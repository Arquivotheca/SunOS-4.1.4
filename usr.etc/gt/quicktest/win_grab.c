#ifndef lint
static char sccsid[] = "@(#)win_grab.c  1.1 94/10/31 SMI";
#endif
/****
 *
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 *
 *	@   @   @@@   @   @          @@@@  @@@@    @@@   @@@@
 *	@   @    @    @@  @         @      @   @  @   @  @   @
 *	@ @ @    @    @ @ @         @ @@@  @@@@   @@@@@  @@@@
 *	@ @ @    @    @  @@         @   @  @  @   @   @  @   @
 *	 @ @    @@@   @   @  @@@@@   @@@@  @   @  @   @  @@@@
 *
 *	Shared window synchronization routines
 *
 *	Code shamelessly stolen from server shapes code.
 *
 * Functions:
 *
 *    WXCLIENT *
 *    wx_grab(devfd,cookie)
 *		int		fd ;
 *		unsigned long	cookie ;
 *
 *	Grab a window.  'cookie' is the window-info handle returned by
 *	XGrabWindow.  'devfd' is the file descriptor of the frame buffer
 *	if known, -1 otherwise.  If you specify -1, wx_grab will open the
 *	frame buffer.  The frame buffer fd may be inquired from the returned
 *	WXINFO structure via the wx_devfd(WXINFO) macro.
 *
 *	Returns a pointer to the WXINFO structure on success, NULL on failure.
 *
 *
 *    WXCLIENT *
 *    wx_win_grab(dpy, win, devfd)
 *		Display	*dpy ;
 *		Window	win ;
 *		int	devfd;
 *
 *	Grab a window.  Simply calls XGrabWindow and then wx_grab().
 *
 *
 *    wx_ungrab(infop, cflag)
 *		WXINFO	*infop ;
 *		int	cflag ;
 *
 *	Ungrab a window.  All resources allocated by wx_grab are freed.
 *	If 'cflag' is nonzero, the framebuffer fd described in the info
 *	page is also closed.  The info page is invalid after this call
 *	and references to it will probably result in a SIGSEGV.
 *
 *	The application should call XUnGrabWindow(dpy,win) after
 *	calling wx_ungrab() so that the server may free the window-info
 *	page at the other end.
 *
 ****/

#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <errno.h>
#include <X11/Xlib.h>

#include "win_grab.h"

#ifndef	GRABPAGEALLOC
#define GRABPAGEALLOC   _IOR(F, 10, caddr_t)
#define GRABPAGEFREE    _IOW(F, 11, caddr_t)
#define GRABATTACH      _IOW(F, 12, caddr_t)
#endif	GRABPAGEALLOC

#define	GRABFILE	"/tmp/wg"
#define	CURGFILE	"/tmp/curg"
#define	NDIGITS		8

#define	MINSHMEMSIZE	(8*1024)
#define WX_PAGESZ       (8*1024)

#ifdef DEBUG
extern	int	sys_nerr, errno ;
extern	char	*sys_errlist[] ;
#endif

/******************************************
 *
 * wx_win_grab:
 *
 *	Grab a window and create shared memory file for window
 *	information map to lock page
 *
 *	arguments:
 *
 *	Display	*dpy;	INPUT
 *		Xwindows display
 *
 *	Window	win;	INPUT
 *		Window to grab.
 *
 *	int	devfd;	INPUT
 *		file descriptor of graphics device
 *
 *	returns a user virtual address for the window info structure.
 *	returns NULL if anything goes awry.
 *
 *	'devfd' is the file descriptor of the frame buffer, if known,
 *	-1 otherwise.  If you specify -1, wx_grab will open the
 *	frame buffer.  The frame buffer fd may be inquired from the returned
 *	WXINFO structure via the wx_devfd(WXINFO) macro.
 *
 *****************************************/

WXCLIENT *
wx_win_grab(dpy, win, devfd)
	Display	*dpy ;
	Window	win ;
	int	devfd;
{
	int	version ;	/* protocol version */
	u_long	cookie ;
	WXCLIENT *infop ;

	if( !getenv("XGL_NO_WINLOCK")  &&
	    (cookie = XGrabWindowNew(dpy, win, 1)) != 0 )
	    version = 1 ;
	else if( (cookie = XGrabWindowNew(dpy, win, 0)) != 0 )
	    version = 0 ;
	else
	    return NULL ;

	if( (infop = wx_grab(devfd,cookie)) == NULL )
	    XUnGrabWindowNew(dpy,win,version) ;
	else
	    infop->w_proto_vers = version ;

	return infop ;
}


/******************************************
 *
 * wx_grab:
 *
 *	create shared memory file for window information
 *	map to lock page
 *
 *	arguments:
 *
 *	int	devfd;	INPUT
 *		file descriptor of graphics device
 *
 *	unsigned long	cookie;	INPUT
 *		magic cookie supplied by the server
 *
 *	returns a user virtual address for the window info structure.
 *	returns NULL if anything goes awry.
 *
 *	'devfd' is the file descriptor of the frame buffer, if known,
 *	-1 otherwise.  If you specify -1, wx_grab will open the
 *	frame buffer.  The frame buffer fd may be inquired from the returned
 *	WXINFO structure via the wx_devfd(WXINFO) macro.
 *
 *****************************************/

WXCLIENT *
wx_grab(devfd,cookie)
	int		devfd;
	unsigned long	cookie ;
{
register WXINFO	*infop;
register WXCLIENT *clientp ;

	int	lockfd ;
	WX_LOCKP_T	lockp, unlockp ;
	char	*lock ;
	char	filename[sizeof(GRABFILE)+NDIGITS+1];
	int	filefd;
	int	locktype ;

	if( (clientp = (WXCLIENT *) malloc(sizeof(WXCLIENT))) == NULL )
	    return NULL ;

	sprintf(filename, "%s%08x",GRABFILE,cookie);

        if ((filefd = open(filename,O_RDWR,0666))<0) {
	    return((WXCLIENT *)NULL);
	}

	/* map the wx_winfo area */
        infop = (WXINFO *)mmap(0,
			    MINSHMEMSIZE,
			    PROT_READ|PROT_WRITE,
			    MAP_SHARED,
			    filefd,
			    (off_t)0);

	if (infop == (WXINFO *)-1) {
	    close(filefd);
	    free(clientp) ;
	    return((WXCLIENT *)NULL);
	}

	clientp->w_info = infop ;

	/* open the frame buffer if not already opened by client */
	if( devfd >= 0 )
	  lockfd = devfd ;
	else
	{
	  lockfd = open(infop->w_devname, O_RDWR,0666) ;
	  if( lockfd < 0 )
	  {
#ifdef DEBUG
	    fprintf(stderr, "wx_grab: cannot open %s, %s\n",
		infop->w_devname, sys_errlist[errno]) ;
#endif
	    munmap(infop, MINSHMEMSIZE) ;
	    close(filefd) ;
	    free(clientp) ;
	    return NULL ;
	  }
	}
	clientp->w_cdevfd = lockfd ;

	locktype = infop->w_version >= 2 ? infop->w_locktype : WG_LOCKDEV ;
	switch( locktype )
	{
	  case WG_LOCKDEV:
	    /* map the lock page */
	    lockp = (WX_LOCKP_T)mmap(0,
			WX_PAGESZ,
			PROT_READ|PROT_WRITE,
			MAP_SHARED,
			lockfd,(off_t)infop->w_cookie);

	    if (lockp == (WX_LOCKP_T)-1) {
#ifdef DEBUG
		fprintf(stderr, "wx_grab: cannot map lock page, %s\n",
		    sys_errlist[errno]) ;
#endif
		munmap(infop, MINSHMEMSIZE) ;
		close(filefd);
		if( devfd < 0 )
		  close(lockfd) ;
		free(clientp) ;
		return(NULL);
	    }

	    /* map the unlock page */
	    unlockp = (WX_LOCKP_T)mmap(0,
			WX_PAGESZ,
			PROT_READ|PROT_WRITE,
			MAP_SHARED,
			lockfd,(off_t)infop->w_cookie) ;

	    if(unlockp == (WX_LOCKP_T)-1) {
#ifdef DEBUG
		fprintf(stderr, "wx_grab: cannot map unlock page\n") ;
#endif
		munmap(lockp, WX_PAGESZ) ;
		munmap(infop, MINSHMEMSIZE) ;
		close(filefd);
		if( devfd < 0 )
		  close(lockfd) ;
		free(clientp) ;
		return(NULL);
	    }
	    break ;

	  case WG_WINLOCK:
	    if( winlockat(infop->w_cookie, &lockp, &unlockp) != 0 )
		return(NULL);
	    break ;
	}

	clientp->w_clockp = lockp ;
	clientp->w_cunlockp = unlockp ;

	/* cursor grabber stuff */
	if (infop->w_version >= 2  &&  infop->c_sinfo) {
	    char    cfn[sizeof(CURGFILE)+NDIGITS+1];
	    int	    c_cfd;
	    
	    strcpy(cfn,CURGFILE);
	    sprintf(cfn+sizeof(CURGFILE)-1,"%08x", infop->c_scr_addr);
	    
	    /* open the shared cursor page */
	    if ((c_cfd = open(cfn, O_RDWR,0666))<0) {
#ifdef DEBUG
		fprintf(stderr, "wx_grab: cannot open cursor grabber page\n") ;
#endif		
		release_lockpages(devfd, clientp);
		return((WXCLIENT *)NULL);
	    }
	    clientp->c_cfd = c_cfd;

	    /* Map it */
	    clientp->c_cinfo = (CURGINFO *)mmap(0,
			MINSHMEMSIZE,	
			PROT_READ|PROT_WRITE,
			MAP_SHARED,
			c_cfd,
			(off_t)0);
			
	    if (clientp->c_cinfo == (CURGINFO *)-1) {
#ifdef DEBUG
		fprintf(stderr, "wx_grab: cannot map cursor page, %s\n",
		    sys_errlist[errno]) ;
#endif
		close(c_cfd);
		release_lockpages(devfd, clientp);
		return(NULL);
	    }
	    /* check to see if you have a good magic number */
	    if ((clientp->c_cinfo)->c_magic != CURG_MAGIC) {
#ifdef DEBUG
		fprintf(stderr, "wx_grab: wrong cursor page mapped, %s\n",
		    sys_errlist[errno]) ;
#endif		
		munmap(clientp->c_cinfo, MINSHMEMSIZE) ;
		close(c_cfd);
		release_lockpages(devfd, clientp);
		return(NULL);
	    }
	}
	else
	    clientp->c_cinfo = NULL ;

	/* success, fill out rest of structure */
	clientp->w_version = infop->w_version ;
	clientp->w_next = NULL ;
	clientp->w_client = 0 ;
	clientp->w_clip_seq = 0 ;
	clientp->w_extnd_seq = 0 ;
	clientp->w_cinfofd = filefd ;
	clientp->w_crefcnt = 0;
	if (infop->w_version != 0)
	  clientp->w_cclipptr = (short *)((char *)infop+infop->u.vn.w_clipoff);

#ifdef DEBUG
	wx_dump(clientp) ;
#endif

	return(clientp);
}




static
release_lockpages(devfd, clientp)
	int		devfd ;
	WXCLIENT	*clientp ;
{
	int		lockfd = clientp->w_cdevfd ;
	int		infofd = clientp->w_cinfofd ;
	WXINFO		*infop = clientp->w_info ;
	WX_LOCKP_T	lockp = clientp->w_clockp ;
	WX_LOCKP_T	unlockp = clientp->w_cunlockp ;
	int		locktype ;

	locktype = infop->w_version >= 2 ? infop->w_locktype : WG_LOCKDEV ;
	switch( infop->w_locktype )
	{
	  case WG_LOCKDEV:
	    munmap(lockp, WX_PAGESZ) ;
	    munmap(unlockp, WX_PAGESZ) ;
	    if( devfd < 0 )
		 close(lockfd) ;
	    break ;

	  case WG_WINLOCK:
	    (void) winlockdt(infop->w_cookie, lockp, unlockp) ;
	    break ;
	}
	munmap(infop, MINSHMEMSIZE) ;
	close(infofd);
	free(clientp) ;
}



wx_win_ungrab(clientp, cflag, dpy, win)
	WXCLIENT	*clientp ;
	int	cflag ;
	Display	*dpy ;
	Window	win ;
{
	int	version = clientp->w_proto_vers ;

	wx_ungrab(clientp, cflag) ;
	XUnGrabWindowNew(dpy,win,version) ;
}




wx_ungrab(clientp, cflag)
	WXCLIENT	*clientp ;
	int	cflag ;
{
	WXINFO	*infop = clientp->w_info ;
	int	infofd, devfd , c_cfd;

	infofd = clientp->w_cinfofd ;
	devfd = clientp->w_cdevfd ;

	if ((infop->w_version != 0) &&
	    (clientp->w_cclipptr !=
	    ((short *)((char *)infop+infop->u.vn.w_clipoff))))
		munmap(clientp->w_cclipptr, clientp->w_ccliplen);
	
	/* Cursor grabber stuff */
	if (clientp->c_cinfo) {
	    c_cfd = clientp->c_cfd;
	    munmap(clientp->c_cinfo, MINSHMEMSIZE) ;
	    close(c_cfd);
	}

	release_lockpages( cflag ? -1 : 0, clientp ) ;
}

#define	ROUNDUP(i)	(((i)+WX_PAGESZ-1)&~(WX_PAGESZ-1))

short *
wx_clipinfo(clientp)
	WXCLIENT	*clientp ;
{
    WXINFO	*infop = clientp->w_info ;
    int		cliplen;
    short	*clipptr;
    short	*cmclip;

    cmclip = (short *)((char *)infop+infop->u.vn.w_clipoff);

    while(1) {
	if (infop->w_flag & WEXTEND) { /* server has an extended mapping */
	    if (clientp->w_cclipptr == cmclip) { /* ...and we don't. */
		cliplen = infop->w_scliplen;
		WX_UNLOCK(clientp);
		clipptr = (short *)mmap(NULL,
				cliplen,
				PROT_READ|PROT_WRITE,
				MAP_SHARED,
				clientp->w_cinfofd,
				ROUNDUP(infop->u.vn.w_clipoff+sizeof(short)));
		WX_LOCK(clientp);
		if ((int)clipptr != -1) {
		    clientp->w_ccliplen = cliplen;
		    clientp->w_cclipptr = clipptr;
		}
		continue;  /* at while */
	    }
	    if (clientp->w_ccliplen != infop->w_scliplen) {
				/* ...and we do... but the wrong size. */
		cliplen = infop->w_scliplen;
		WX_UNLOCK(clientp);
		munmap(clientp->w_cclipptr,clientp->w_ccliplen);
		clipptr = (short *)mmap(NULL,
				cliplen,
				PROT_READ|PROT_WRITE,
				MAP_SHARED,
				clientp->w_cinfofd,
				ROUNDUP(infop->u.vn.w_clipoff+sizeof(short)));
		WX_LOCK(clientp);
		if ((int)clipptr == -1)
		    clientp->w_cclipptr = cmclip;
		else {
		    clientp->w_ccliplen = cliplen;
		    clientp->w_cclipptr = clipptr;
		}
		continue;  /* at while */
	    }
	} else { 	/* server doesn't have an extended mapping */
            if (clientp->w_cclipptr != cmclip) { /* ...and we do. */
                WX_UNLOCK(clientp);
                munmap(clientp->w_cclipptr,clientp->w_ccliplen);
                WX_LOCK(clientp);
		clientp->w_cclipptr = cmclip;
		continue;  /* at while */
	    }
	    else {	/* ...nor do we */
		if (wx_shape_flags(infop) & SH_RECT_FLAG)
		  return((short *)&((struct class_SHAPE_vn *)((char *)(infop) +
			  (infop)->u.vn.w_shapeoff))->SHAPE_YMIN);
		/* else fall through and return clientp->w_cclipptr */
	    }
	}
	break;
    }
    return(clientp->w_cclipptr);
}


#ifdef	DEBUG
wx_dump(clientp)
	WXCLIENT	*clientp ;
{
register WXINFO	*infop = clientp->w_info ;

	printf("client page is %x\n", clientp) ;
	printf("  w_info = %x\n", clientp->w_info) ;
	printf("  w_version = %d\n", clientp->w_version) ;
	printf("  w_next = %x\n", clientp->w_next) ;
	printf("  w_client = %x\n", clientp->w_client) ;
	printf("  w_clip_seq = %d\n", clientp->w_clip_seq) ;
	printf("  w_extnd_seq = %d\n", clientp->w_extnd_seq) ;
	printf("  w_cinfofd = %d\n", clientp->w_cinfofd) ;
	printf("  w_cdevfd = %d\n", clientp->w_cdevfd) ;
	printf("  w_crefcnt = %d\n", clientp->w_crefcnt) ;
	printf("  w_clockp = %x\n", clientp->w_clockp) ;
	printf("  w_cunlockp = %x\n", clientp->w_cunlockp) ;
	printf("  w_cclipptr = %x\n", clientp->w_cclipptr) ;
	printf("  w_ccliplen = %d\n", clientp->w_ccliplen) ;

	printf("info page is %x\n", infop) ;
	printf("  w_flag = %x\n", infop->w_flag) ;
	printf("  w_magic = %x\n", infop->w_magic) ;
	printf("  w_version = %d\n", infop->w_version) ;
	printf("  w_cunlockp = %x\n", infop->w_cunlockp) ;
	printf("  w_devname = %s\n", infop->w_devname) ;
	printf("  w_cookie = %x\n", infop->w_cookie) ;
	printf("  w_clipoff = %x\n", infop->u.vn.w_clipoff) ;
	printf("  w_scliplen = %d\n", infop->w_scliplen) ;
	printf("  w_org = %d,(%f,%f)\n",
		infop->w_org.t, infop->w_org.x/65536., infop->w_org.y/65536.) ;
	printf("  w_dim = %d,(%f,%f)\n",
		infop->w_dim.t, infop->w_dim.x/65536., infop->w_dim.y/65536.) ;
	printf("  &w_shape_hdr = %x\n", &infop->w_shape_hdr) ;
	printf("  &w_shape = %x\n", &infop->w_shape) ;
	printf("  w_shape.SHAPE_FLAGS = %x\n", infop->w_shape.SHAPE_FLAGS) ;
	printf("  w_shape.SHAPE_YMIN = %d\n", infop->w_shape.SHAPE_YMIN) ;
	printf("  w_shape.SHAPE_YMAX = %d\n", infop->w_shape.SHAPE_YMAX) ;
	printf("  w_shape.SHAPE_XMIN = %d\n", infop->w_shape.SHAPE_XMIN) ;
	printf("  w_shape.SHAPE_XMAX = %d\n", infop->w_shape.SHAPE_XMAX) ;
	printf("  w_shape.SHAPE_X_EOL = %x\n", infop->w_shape.SHAPE_X_EOL) ;
	printf("  w_shape.SHAPE_Y_EOL = %x\n", infop->w_shape.u.SHAPE_Y_EOL) ;
	printf("  &w_cliparray = %x\n", &infop->w_cliparray) ;
}
#endif	DEBUG
