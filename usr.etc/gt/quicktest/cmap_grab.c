#ifndef lint
static char sccsid[] = "@(#)cmap_grab.c  1.1 94/10/31 SMI";
#endif

/****
 *
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 *
 *	 @@@   @   @   @@@   @@@@           @@@@  @@@@    @@@   @@@@   
 *	@      @@ @@  @   @  @   @         @      @   @  @   @  @   @  
 *	@      @ @ @  @@@@@  @@@@          @ @@@  @@@@   @@@@@  @@@@   
 *	@      @ @ @  @   @  @             @   @  @  @   @   @  @   @  
 *	 @@@   @ @ @  @   @  @      @@@@@   @@@@  @   @  @   @  @@@@   
 *
 *	Shared colormap synchronization routines.
 *
 *	Client side
 *
 *	Ed Falk, 18 Jan 90
 *
 * Functions:
 *
 *    Cmap_Grab *
 *    wx_cm_grab(devfd, cookie)
 *		int		devfd ;
 *		unsigned long	cookie ;
 *
 *	Grab a colormap.  'cookie' is the handle returned by
 *	XGrabColormap.  'devfd' is the file descriptor of the frame buffer
 *	if any, -1 otherwise.  If you specify -1, wx_cm_grab will open
 *	the frame buffer.  The frame buffer fd may be inquired from
 *	the returned Cmap_Grab structure.
 *
 *	Returns a pointer to the Cmap_Grab structure on success,
 *	NULL on failure.
 *
 *
 *    int
 *    wx_cm_ungrab(cginfo,cflag)
 *		Cmap_Grab	*cginfo ;
 *		int		cflag ;
 *
 *	Release a colormap.  All resources allocated by wx_cm_grab are
 *	freed.  The application should call XUnGrabColormap after calling
 *	wx_cm_ungrab() so that the server may free the colormap info page
 *	at the other end.
 *
 *	if cflag is nonzero, the framebuffr fd described in the info page
 *	is also closed.  The info page is invalid after this call and
 *	references to it will probably result in a SIGSEGV.
 *
 *	Returns 0 on success, -1 on failure, although there's nothing for
 *	the calling program to actually do if there's been a failure.
 *
 *
 *
 *    int
 *    wx_cm_get(cginfo,index,count, red,green,blue)
 *		Cmap_Grab	*cginfo ;
 *		int		index, count ;
 *		u_char		*red, *green, *blue ;
 *
 *	Read colormap values and return them to the application.
 *
 *
 *
 *    int
 *    wx_cm_put(cginfo,index,count, red,green,blue)
 *		Cmap_Grab	*cginfo ;
 *		int		index, count ;
 *		u_char		*red, *green, *blue ;
 *
 *	Write colormap to hardware if colormap is installed, otherwise
 *	save them in shared memory.
 *
 ****/

static	int		_cmapgrab_initialized = 0 ;

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <errno.h>

#ifdef SVR4
#include <sys/fbio.h>
#define WIOC ('w' << 8)
#define WINSETCMS (WIOC|34)
#else
#include <sun/fbio.h>
#endif

#if	!defined(FBIOPUTCMAPI)  &&  defined(FB_CLUTPOST)
#define	FBIOPUTCMAPI	FB_CLUTPOST
#define	fbcmap_i	fb_clut
#endif

#include "cmap_grab.h"

/* externs */
extern	char	*malloc(),*valloc();
extern	int	errno ;
extern	char	*sys_errlist[] ;

#define	GRABFILE	"/tmp/cm"
#define	NDIGITS		8

#define	MINSHMEMSIZE	(8*1024)
#define	CG_PAGESZ	(8*1024)

static	Cmap_Grab	*errret() ;
static	int		sunwindows_open();
static	void		sunwindows_close();

static	Cmap_Grab	*grabbed_list = NULL ;



/*ARGSUSED*/
Cmap_Grab *
wx_cm_grab(fd, cookie)
	int	fd ;
	u_long	cookie ;
{
register Grabbedcmap	*infop, tmp ;
register Cmap_Grab	*cmap_grab ;
	char		fn[sizeof(GRABFILE)+NDIGITS+1];
	char		*lock, *unlock ;
	int		lockfd,			/* lock device */
			devfd,			/* framebuffer */
			infofd,			/* shared memory file */
			sunwindows_def_fd,	/* /dev/winXX fd */
			sunwindows_inst_fd ;	/* /dev/winXX fd */
	int		locktype = 0 ;		/* use /dev/winlock */
	int		cflag = 0 ;		/* close devfd */
	int		filesuffix;
	int		cmaplen ;
	int		filelen ;
	int		ok;

	/* first and foremost, is this a nested grab? */

	for(cmap_grab = grabbed_list;
	    cmap_grab != NULL;
	    cmap_grab = cmap_grab->cm_next)
	  if( cmap_grab->cm_info->cm_cookie == cookie ) {
	    ++cmap_grab->cm_grab_count ;
	    return cmap_grab ;
	  }

	if( (cmap_grab = (Cmap_Grab *)calloc(1,sizeof(Cmap_Grab))) == NULL )
	    return errret("malloc failed", fn) ;

	cmap_grab->cm_cinfofd =
	cmap_grab->cm_cdevfd =
	cmap_grab->cm_clockfd =
	cmap_grab->cm_csunwindows_def_fd =
	cmap_grab->cm_csunwindows_inst_fd = -1 ;
	cmap_grab->cm_grab_count = 1 ;
	cmap_grab->cm_next = grabbed_list ;
	grabbed_list = cmap_grab ;

	/* open the file.  Read in enough data to find out long it is. */

	sprintf(fn, "%s%08x", GRABFILE, cookie) ;

        if ((infofd = open(fn,O_RDWR,0666))<0) {
	    cleanup_lockpages(cmap_grab,0,0,0) ;
	    return errret("can't open info page, ", fn) ;
	}
	cmap_grab->cm_cinfofd = infofd ;

	if( read(infofd, &tmp, sizeof(tmp)) != sizeof(tmp) )
	{
	    cleanup_lockpages(cmap_grab,0,0,0) ;
	    return errret("info page too short", fn) ;
	}

	if( tmp.cm_magic != CMMAGIC )
	{
	    cleanup_lockpages(cmap_grab,0,0,0) ;
	    return errret("invalid magic number", fn) ;
	}

	cmaplen = tmp.cm_reallen ;

	filelen = tmp.cm_shadowofs + 3*cmaplen*sizeof(short) + 4*cmaplen;

	/* map the colormap info area */

        infop = (Grabbedcmap *)mmap(0,
			filelen,
			PROT_READ|PROT_WRITE,
			MAP_SHARED,
			infofd,
			(off_t)0);

	if (infop == (Grabbedcmap *)-1)
	{
	    cleanup_lockpages(cmap_grab,0,filelen,0) ;
	    return errret("can't map info page", fn) ;
	}
	cmap_grab->cm_info = infop ;


	/* Open framebuffer if not provided by caller */

	if( fd == -1 )
	{
	  devfd = open(infop->cm_devname, O_RDWR,0666) ;
	  if( devfd < 0 )
	  {
	    fprintf(stderr, "wx_cm_grab: cannot open %s, %s\n",
		infop->cm_devname, sys_errlist[errno]) ;
	    cleanup_lockpages(cmap_grab,0,filelen,0) ;
	    return NULL ;
	  }
	  cflag = 1 ;
	}
	else
	  devfd = fd ;

	cmap_grab->cm_cdevfd = devfd ;


	locktype = infop->cm_version >= 1 ? infop->cm_locktype : CM_LOCKDEV ;

	switch( locktype )
	{
	  case CM_LOCKDEV:
	    /* if lockdevice explicitly specified by server, we need to
	     * open it here, even if the calling routine has opened the
	     * framebuffer for us.  Otherwise, we use the framebuffer device.
	     */

	    if( infop->cm_lockdevname[0] != '\0' )
	    {
	      lockfd = open(infop->cm_lockdevname, O_RDWR,0666) ;
	      if( lockfd < 0 )
	      {
		fprintf(stderr, "wx_cm_grab: cannot open %s, %s\n",
		    infop->cm_lockdevname, sys_errlist[errno]) ;
		cleanup_lockpages(cmap_grab,cflag,filelen,locktype) ;
		return NULL ;
	      }
	    }
	    else
	      lockfd = devfd ;

	    cmap_grab->cm_clockfd = lockfd ;


	    /* map the lock page */

	    lock = (char *)mmap(0, 
		    CM_PAGESZ, 
		    PROT_READ|PROT_WRITE, 
		    MAP_SHARED,
		    lockfd, (off_t)cookie);

	    if (lock == (char *)-1)
	    {
		cleanup_lockpages(cmap_grab,cflag,filelen,0) ;
		return errret("can't map lock page",tmp.cm_devname) ;
	    }
	    cmap_grab->cm_clockp = (u_long *) lock ;

	    /* map the unlock page */

	    unlock = (char *) mmap(0,
		      CM_PAGESZ, 
		      PROT_READ|PROT_WRITE, 
		      MAP_SHARED,
		      lockfd,(off_t)cookie) ;

	    if( unlock == (char *)-1 )
	    {
		cleanup_lockpages(cmap_grab,cflag,filelen,0) ;
		return errret("can't map unlock page", tmp.cm_devname) ;
	    }
	    cmap_grab->cm_cunlockp = (u_long *) unlock ;
	    break ;

	  case CM_WINLOCK:
	    if( winlockat(cookie, &lock, &unlock) != 0 ) {
	      cleanup_lockpages(cmap_grab,cflag,filelen,locktype) ;
	      return errret("can't get lock pages", fn) ;
	    }
	    cmap_grab->cm_clockp = (u_long *) lock ;
	    cmap_grab->cm_cunlockp = (u_long *) unlock ;
	    break ;
	}


	/* fill in the misc stuff */

	sunwindows_def_fd = sunwindows_inst_fd = -1;
	ok = 1;
	if (infop->cm_default && (infop->cm_sunwindows_def_devname[0] != '\0'))
	{
		if ((sunwindows_def_fd = sunwindows_open(
					infop->cm_sunwindows_def_devname)) < 0)
			ok = 0;
	}
	cmap_grab->cm_csunwindows_def_fd = sunwindows_def_fd ;
	if (ok && (infop->cm_sunwindows_inst_devname[0] != '\0'))
	{
		if ((sunwindows_inst_fd = sunwindows_open(
					infop->cm_sunwindows_inst_devname)) < 0)
			ok = 0;
	}
	cmap_grab->cm_csunwindows_inst_fd = sunwindows_inst_fd ;

	if(!ok)
	{
	    cleanup_lockpages(cmap_grab,cflag,filelen,locktype) ;
	    return errret("/dev/winxx open failed", fn) ;
	}

	cmap_grab->cm_next = NULL ;
	cmap_grab->cm_client = NULL ;
	cmap_grab->cm_count = -1 ;	/* flag all changes as not seen */
	cmap_grab->cm_lockcount = 0 ;
	cmap_grab->cm_use_new_ioctl = 1 ;	/* try to use new ioctl */

	return(cmap_grab);
}


static int
cleanup_lockpages(cginfo,cflag,filelen,locktype)
	Cmap_Grab	*cginfo ;
	int		cflag, filelen, locktype ;
{
	int	error = 0 ;
	Cmap_Grab	*p1, *p2 ;

	if (cginfo->cm_csunwindows_def_fd >= 0)
	    sunwindows_close(cginfo->cm_csunwindows_def_fd);
	if (cginfo->cm_csunwindows_inst_fd >= 0)
	    sunwindows_close(cginfo->cm_csunwindows_inst_fd);
	switch( locktype )
	{
	  case CM_LOCKDEV:
	    if( cginfo->cm_cunlockp != NULL )
	      error |= munmap(cginfo->cm_cunlockp,CM_PAGESZ) < 0 ;
	    if( cginfo->cm_clockp != NULL )
	      error |= munmap(cginfo->cm_clockp,CM_PAGESZ) < 0 ;
	    if( cginfo->cm_clockfd != -1 &&
		cginfo->cm_clockfd != cginfo->cm_cdevfd )
	      error |= close(cginfo->cm_clockfd) ;
	    break ;

	  case CM_WINLOCK:
	    error |= winlockdt(cginfo->cm_info->cm_cookie,
			cginfo->cm_clockp, cginfo->cm_cunlockp) < 0 ;
	    break ;
	}
	if( cginfo->cm_info != NULL )
	  error |= munmap(cginfo->cm_info, filelen) < 0 ;
	if( cginfo->cm_cinfofd != -1 )
	  error |= close(cginfo->cm_cinfofd) < 0 ;
	if( cflag )
	  error |= close(cginfo->cm_cdevfd) < 0 ;

	for( p1 = grabbed_list, p2 = NULL ;
	     p1 != NULL  &&  p1 != cginfo;
	     p2 = p1, p1=p1->cm_next) ;
	if( p1 == NULL )		/* not found?? */
	  error = 1 ;
	else
	{
	  if( p2 == NULL )
	    grabbed_list = p1->cm_next ;
	  else
	    p2->cm_next = p1->cm_next ;
	}

	error |= free(cginfo) == 0 ;
	return error ? -1 : 0 ;
}

int
wx_cm_ungrab(cginfo, cflag)
	Cmap_Grab	*cginfo ;
	int		cflag ;
{
register Grabbedcmap  *infop = cginfo->cm_info ;
	char	fn[sizeof(GRABFILE)+NDIGITS+1];
	int	cmaplen, filelen ;
	int	locktype ;

	if( --cginfo->cm_grab_count > 0 )
	  return 0 ;

	sprintf(fn, "%s%08x", GRABFILE, infop->cm_cookie) ;

	cmaplen = infop->cm_reallen ;
	filelen = infop->cm_shadowofs + 3*cmaplen*sizeof(short) + 4*cmaplen ;

	return cleanup_lockpages(cginfo,cflag,filelen,
	      infop->cm_version >= 1 ? infop->cm_locktype : CM_LOCKDEV ) ;
}

static Cmap_Grab *
errret(msg,file)
	char	*msg, *file ;
{
	fprintf(stderr, "colormap-grab: %s%s, %s\n",
		msg, file, sys_errlist[errno]) ;
	return NULL ;
}



/*
 * Read colormap from shared memory.
 * Shared memory should always be in sync
 * with server's idea of this X11 colormap's
 * contents.
 */

void
wx_cm_get(cginfo,index,count, red,green,blue)
register Cmap_Grab	*cginfo ;
	int		index, count ;
	u_char		*red, *green, *blue ;
{
register Grabbedcmap	*infop = cginfo->cm_info ;

	wx_cm_lock(cginfo) ;

	/* REMIND: Currently, we don't need to test for any changes in the
	 * shared memory page; but if we did, right here is where we'd do
	 * it.  We compare infop->cm_count against cginfo->cm_count and
	 * react if there was a difference.  After handling the change,
	 * we'd copy infop->cm_count to cginfo->cm_count.
	 */

	if( index+count > infop->cm_reallen )
	  count = infop->cm_reallen - index ;

	if( count > 0 )
	{

	 		/* copy from shared memory */
	    short *shadow ;
	    register short *ip ;
	    register u_char *op ;
	    register int n ;

	    shadow = (short *) ( (u_char *)infop + infop->cm_shadowofs ) ;
	    shadow += index ;
	    for(n=count, op=red, ip=shadow ; --n >= 0 ; *op++ = *ip++>>8 ) ;
	    shadow += infop->cm_reallen ;
	    for(n=count, op=green, ip=shadow ; --n >= 0 ; *op++ = *ip++>>8 ) ;
	    shadow += infop->cm_reallen ;
	    for(n=count, op=blue, ip=shadow ; --n >= 0 ; *op++ = *ip++>>8 ) ;
	}
	wx_cm_unlock(cginfo) ;
}



/* write colormap to shared memory, and to DACS if appropriate. */

void
wx_cm_put(cginfo,index,count, red,green,blue)
register Cmap_Grab	*cginfo ;
	int		index, count ;
	u_char		*red, *green, *blue ;
{
register Grabbedcmap	*infop = cginfo->cm_info ;
	short		*shadow ;
register u_char		*ip ;		/* in pointer */
register short		*op ;		/* out pointer */
register u_char		*fp ;		/* flag pointer */
register int		n ;

	wx_cm_lock(cginfo) ;

	if( index+count > infop->cm_reallen )
	  count = infop->cm_reallen - index ;

	if( count > 0 )
	{
	  if( index < infop->cm_start )
	    infop->cm_start = index ;

	  if( index + count > infop->cm_start + infop->cm_len )
	    infop->cm_len = index + count -infop->cm_start ;

	  /* copy to shared memory shadow of grabbed color map */
	  /* "stutter" the 8-bit values into 16 bits */

	  shadow = (short *) ( (u_char *)infop + infop->cm_shadowofs ) ;
	  shadow += index ;
	  for(n=count, ip=red, op=shadow ; --n >= 0 ; *op++, *ip++)
		*op = *ip | (*ip << 8);
	  shadow += infop->cm_reallen ;
	  for(n=count, ip=green, op=shadow ; --n >= 0 ; *op++, *ip++)
		*op = *ip | (*ip << 8);
	  shadow += infop->cm_reallen ;
	  for(n=count, ip=blue, op=shadow ; --n >= 0 ; *op++, *ip++)
		*op = *ip | (*ip << 8);
	  fp = (u_char *)infop + infop->cm_shadowofs +
		3 * infop->cm_reallen * sizeof(short) + index ;
	  for(n=count ; --n >= 0 ; *fp++ = 1 ) ;

	  if( infop->cm_installed )
	  {
	    u_char *hwshadow;

	    /* copy to shared memory shadow of hardware color map */
	    hwshadow = (u_char *)infop + infop->cm_shadowofs +
		3 * infop->cm_reallen * sizeof(short) +
		infop->cm_reallen + index;
	    for (n=count, ip=red, fp=hwshadow; --n >= 0; *fp++ = *ip++);
	    hwshadow += infop->cm_reallen;
	    for (n=count, ip=green, fp=hwshadow; --n >= 0; *fp++ = *ip++);
	    hwshadow += infop->cm_reallen;
	    for (n=count, ip=blue, fp=hwshadow; --n >= 0; *fp++ = *ip++);
	    hwshadow -= index + 2 * infop->cm_reallen;

	    switch(infop->cm_load_method)
	    {
	    case SUNWINDOWS_IOCTL:
		{
		struct cmschange cmschange;

		if( cginfo->cm_csunwindows_def_fd >= 0 )
		    {
		    strcpy(cmschange.cc_cms.cms_name,
			infop->cm_sunwindows_def_cmapname);
		    cmschange.cc_cms.cms_addr = 0;
		    cmschange.cc_cms.cms_size =
			infop->cm_sunwindows_def_cmapsize;
		    cmschange.cc_map.cm_red = hwshadow;
		    cmschange.cc_map.cm_green = hwshadow + infop->cm_reallen;
		    cmschange.cc_map.cm_blue = hwshadow + 2 * infop->cm_reallen;

		    /* HACK ALERT */
		    /* Adjust SUNWINDOWS cms segment 1st colr != last color */
		    if ((cmschange.cc_map.cm_red[0] ==
		      cmschange.cc_map.cm_red[cmschange.cc_cms.cms_size-1]) &&
		      (cmschange.cc_map.cm_green[0] ==
		      cmschange.cc_map.cm_green[cmschange.cc_cms.cms_size-1]) &&
		      (cmschange.cc_map.cm_blue[0] ==
		      cmschange.cc_map.cm_blue[cmschange.cc_cms.cms_size-1]))
			{
			if (cmschange.cc_map.cm_blue[0] > 0)
			    cmschange.cc_map.cm_blue[0]--;
			else
			    cmschange.cc_map.cm_blue[0] = 1;
			}

		    ioctl(cginfo->cm_csunwindows_def_fd, WINSETCMS, &cmschange);
		    }

		if( cginfo->cm_csunwindows_inst_fd >= 0 )
		    {
		    strcpy(cmschange.cc_cms.cms_name,
			infop->cm_sunwindows_inst_cmapname);
		    cmschange.cc_cms.cms_addr = 0;
		    cmschange.cc_cms.cms_size =
			infop->cm_sunwindows_inst_cmapsize;
		    cmschange.cc_map.cm_red = hwshadow;
		    cmschange.cc_map.cm_green = hwshadow + infop->cm_reallen;
		    cmschange.cc_map.cm_blue = hwshadow + 2 * infop->cm_reallen;

		    /* HACK ALERT */
		    /* Adjust SUNWINDOWS cms segment 1st colr != last color */
		    if ((cmschange.cc_map.cm_red[0] ==
		      cmschange.cc_map.cm_red[cmschange.cc_cms.cms_size-1]) &&
		      (cmschange.cc_map.cm_green[0] ==
		      cmschange.cc_map.cm_green[cmschange.cc_cms.cms_size-1]) &&
		      (cmschange.cc_map.cm_blue[0] ==
		      cmschange.cc_map.cm_blue[cmschange.cc_cms.cms_size-1]))
			{
			if (cmschange.cc_map.cm_blue[0] > 0)
			    cmschange.cc_map.cm_blue[0]--;
			else
			    cmschange.cc_map.cm_blue[0] = 1;
			}

		    ioctl(cginfo->cm_csunwindows_inst_fd,WINSETCMS,&cmschange);
		    }
		}
		break;
	    case HW_DEVICE_DIRECT:	/* could have device-specific
					   routines here; just fall
					   through to device ioctl
					   for now */

	    case HW_DEVICE_IOCTL:
	    default:
#ifdef	FBIOPUTCMAPI
		if( cginfo->cm_use_new_ioctl )	/* try new ioctl */
		{
		    struct fbcmap_i cmap;

		    cmap.flags = 0 ;
		    cmap.id = infop->cm_cmapnum ;
		    cmap.index = infop->cm_ioctlbits | index;
		    cmap.count = count;
		    cmap.red   = red;
		    cmap.green = green;
		    cmap.blue  = blue;
		    if( ioctl(cginfo->cm_cdevfd, FBIOPUTCMAPI, &cmap) == 0 )
		      break;
		    else
		      cginfo->cm_use_new_ioctl == 0 ;
		}
#endif	/* FBIOPUTCMAPI */
		{
		    struct fbcmap cmap;

		    cmap.index = infop->cm_ioctlbits | index;
		    cmap.count = count;
		    cmap.red   = red;
		    cmap.green = green;
		    cmap.blue  = blue;
		    ioctl(cginfo->cm_cdevfd, FBIOPUTCMAP, &cmap);
		}
		break;
	    }
	  }

	  /* We've changed the shared memory page, flag this fact to
	   * the server and to any other clients
	   */

	  cginfo->cm_count = ++infop->cm_count ;
	}
	wx_cm_unlock(cginfo) ;
}


static Cmap_Devlist *devlist = NULL;

static	int
sunwindows_open(devname)
char *devname;
{
	Cmap_Devlist *dlist = devlist;
	int fd;

	while (dlist)
		{
		if (strcmp(devname, dlist->devname) == 0)
			{
			dlist->refcnt++;
			return(dlist->fd);
			}
		dlist = dlist->next;
		}
	if ((fd = open(devname, O_RDWR, 0666)) < 0)
		{
		fprintf(stderr, "colormap-grab: open failed %s, %s\n",
		    devname, sys_errlist[errno]) ;
		return(-1);
		}
	if((dlist = (Cmap_Devlist *)malloc(sizeof(Cmap_Devlist))) == NULL )
		{
		close(fd);
		fprintf(stderr, "colormap-grab: malloc failed %s, %s\n",
		    devname, sys_errlist[errno]) ;
		return(-1);
		}
	dlist->next = devlist;
	devlist = dlist;
	dlist->refcnt = 1;
	dlist->fd = fd;
	strcpy(dlist->devname, devname);
	return(fd);
}


static	void
sunwindows_close(fd)
int fd;
{
	Cmap_Devlist *dlist = devlist;
	Cmap_Devlist **dlptr = &devlist;

	while (dlist)
		{
		if (fd == dlist->fd)
			{
			if (--dlist->refcnt == 0)
				{
				close(fd);
				*dlptr = dlist->next;
				free(dlist);
				}
			return;
			}
		dlptr = &dlist->next;
		dlist = dlist->next;
		}
}






/* /dev/winlock management code.  This is a temporary hack so we can
 * develop the new /dev/winlock interface.  Hopefully, this will become
 * part of the C library someday.
 *
 */


#include <sys/types.h>
#include <sys/ioccom.h>

	/* structure for allocating lock contexts.  The identification
	 * should be provided as the offset for mmap(2).  The offset is
	 * the byte-offset relative to the start of the page returned
	 * by mmap(2).
	 */

struct	winlockalloc {
	u_long	sy_key ;	/* user-provided key, if any */
	u_long	sy_ident ;	/* system-provided identification */
	} ;

struct	winlocktimeout {
	u_long	sy_ident ;
	u_int	sy_timeout ;
	int	sy_flags ;
	} ;


#define	WINLOCKALLOC	_IOWR(L, 0, struct winlockalloc)
#define	WINLOCKFREE	_IOW(L, 1, u_long)
#define	WINLOCKSETTIMEOUT	_IOW(L, 2, struct winlocktimeout)
#define	WINLOCKGETTIMEOUT	_IOWR(L, 3, struct winlocktimeout)
#define	WINLOCKDUMP	_IO(L, 4)


/* flag bits */
#define SY_NOTIMEOUT	0x1	/* This client never times out */


#ifndef	GRABPAGEALLOC
#include <sun/fbio.h>
#endif

static	char	*lockdev_name = "/dev/winlock" ;
static	char	*alt_lockdev_name = "/dev/cgsix0" ;

static	int	lock_fd = -1 ;
static	u_long	pagemask ;
static	u_long	pageoffset ;
static	u_long	pagesize ;



	/* return non-zero if fail */

static int
init()
{
	if( lock_fd == -1 ) {
	  if( (lock_fd = open(lockdev_name, O_RDWR, 0)) == -1  &&
	      (lock_fd = open(alt_lockdev_name, O_RDWR, 0)) == -1 )
	    return 1 ;
	  pagesize = getpagesize() ;
	  pageoffset = pagesize - 1 ;
	  pagemask = ~pageoffset ;
	}

	return 0 ;
}


int
winlockat(cookie, lockp, unlockp)
	u_long	cookie ;
	int	**lockp, **unlockp ;
{
	int	ofs ;
	caddr_t	ptr ;

	if( lock_fd == -1  &&  init() )
	{
	  errno = EINVAL ;
	  return -1 ;
	}

	ofs = cookie & pageoffset ;
	cookie &= pagemask ;

	ptr = mmap(0, pagesize, PROT_READ|PROT_WRITE,
		MAP_SHARED, lock_fd, (off_t)cookie) ;
	if( (int)ptr == -1 )
	{
	  errno = ENOSPC ;
	  return -1 ;
	}
	*lockp = (int *) (ptr+ofs) ;

	ptr = mmap(0, pagesize, PROT_READ|PROT_WRITE,
		MAP_SHARED, lock_fd, (off_t)cookie) ;
	if( (int)ptr == -1 )
	{
	  (void) munmap((caddr_t)*lockp, pagesize) ;
	  errno = ENOSPC ;
	  return -1 ;
	}
	*unlockp = (int *) (ptr+ofs) ;

	return 0 ;
}



int
winlockdt(cookie, lockp, unlockp)
	u_long	cookie ;
	int	*lockp, *unlockp ;
{
	caddr_t	ptr ;

	if( lock_fd == -1  &&  init() )
	{
	  errno = EINVAL ;
	  return -1 ;
	}

	ptr = (caddr_t) ((int)lockp & pagemask) ;
	if( munmap(ptr, pagesize) )
	    perror("winlockdt: munmap:");

	ptr = (caddr_t) ((int)unlockp & pagemask) ;
	if( munmap(ptr, pagesize) )
	    perror("winlockdt: munmap:");

	return 0 ;
}
