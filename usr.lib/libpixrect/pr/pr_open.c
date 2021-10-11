#ifndef lint
static char sccsid[] = "@(#)pr_open.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Pixrect device opening routine.
 */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sun/fbio.h>
#include <pixrect/pixrect.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>


/*
 * pixrect create function vector
 * (defined in pr_makefun.c)
 */
/* extern Pixrect *(*pr_makefun[FBTYPE_LASTPLUSONE])(); */
typedef Pixrect *(*Pixfunc)();
extern Pixfunc pr_makefun[FBTYPE_LASTPLUSONE];
static Pixfunc make_func, shared_make_func();

#ifdef SHLIB
#define SMALL_INTEGER (1 << (sizeof(int) * 8 - 1))
#define MAKE_FUNC_LENGTH 30
#define	IS_VALID_FBTYPE(type)                            \
	( (((unsigned) (type) < FBTYPE_LASTPLUSONE) &&		\
	  ((make_func =  pr_makefun[(type)]) !=  NULL) ) ||       \
	 ((make_func = shared_make_func((type))) !=  NULL) ) 
#else
#define	IS_VALID_FBTYPE(type)                            \
	((unsigned) (type) < FBTYPE_LASTPLUSONE &&               \
	 ((make_func = pr_makefun[(type)]) != 0))
#endif

#define	CAN_SATTR(attr) \
	((attr).owner == 0)

Pixrect *
pr_open(fbname)
	char	*fbname;
{
	register int fd;
	register char *errstr;
	struct pr_size fbsize;
	Pixrect *pr;
	struct fbgattr attr;

	if ((fd = open(fbname, O_RDWR, 0)) < 0) {
		errstr = "open failed for";
		goto bad;
	}

	/* force FBIOGTYPE if FBIOGATTR fails */
	attr.real_type = -1;
	attr.fbtype.fb_type = -1;

	/* get frame buffer attributes */
	if (ioctl(fd, FBIOGATTR, &attr) != -1 && CAN_SATTR(attr)) {
		/*
		 * If we can't use the native mode, pick a suitable
		 * emulation mode and turn auto-initialization on.
		 */
		if (!IS_VALID_FBTYPE(attr.fbtype.fb_type)) {
			register int bestemu, *ep;

			bestemu = -1;
			for (ep = attr.emu_types; 
				ep < &attr.emu_types[FB_ATTR_NEMUTYPES]; 
				ep++) {
				if (*ep == -1) 
					break;
				if (IS_VALID_FBTYPE(*ep) && *ep > bestemu)
					bestemu = *ep;
			}

			/* skip the ioctl if it wouldn't change anything */
			if (bestemu > 0 
				&& (bestemu != attr.sattr.emu_type ||
				!(attr.sattr.flags & FB_ATTR_AUTOINIT))) {

				attr.sattr.flags |= FB_ATTR_AUTOINIT;
				attr.sattr.emu_type = bestemu;
				(void) ioctl(fd, FBIOSATTR, &attr.sattr);
			}
		}
	}

	/*
	 * If using an emulation mode, get the type information for it.
	 */
	if (!IS_VALID_FBTYPE(attr.fbtype.fb_type) &&
		ioctl(fd, FBIOGTYPE, &attr.fbtype) == -1) {
		errstr = "FBIOGTYPE ioctl failed for";
		goto bad;
	}

	if (!IS_VALID_FBTYPE(attr.fbtype.fb_type)) {
		errstr = "no pixrect implemented for";
		goto bad;
	}

	fbsize.x = attr.fbtype.fb_width;
	fbsize.y = attr.fbtype.fb_height;
	if ((pr = (Pixrect *)(*make_func)(fd, fbsize, 
		attr.fbtype.fb_depth, &attr)) == 0) {
		errstr = "pixrect create failed for";
		goto bad;
	}
	close(fd);
	return pr;

bad:
	if (fd >= 0)
		close(fd);
	fprintf(stderr, "pr_open: %s %s\n", errstr, fbname);
	return (Pixrect *) 0;
}


/* 
 * Return the frame buffer type of the device, or -1 if error
 */
pr_getfbtype_from_fd(fd)
	int fd;
{
	struct fbgattr attr;

	/* assume failure */
	attr.fbtype.fb_type = -1;

	/* try to get attributes first */
       	if (ioctl(fd, FBIOGATTR, &attr) == -1 ||
		!IS_VALID_FBTYPE(attr.fbtype.fb_type)) {

		/*
		 * There are two reasons to be here:
		 *
		 * 1. The frame buffer driver doesn't support the
		 *    FBIOGATTR ioctl; we must use FBIOGTYPE to
		 *    find out what it is.
		 *
		 * 2. The frame buffer is new and we weren't linked
		 *    with a pixrect driver for it; we use FBIOGTYPE
		 *    to see if it is emulating something useful.
		 */
		(void) ioctl(fd, FBIOGTYPE, &attr.fbtype);
	}

	return attr.fbtype.fb_type;
}

#ifdef SHLIB

static struct pixlib {
    void	  *shlib_fd;
    struct pixlib *next;
}               pixlib_rec , *pixlib;



/* return the function pointer of the type from the shared module */
static Pixfunc
shared_make_func(type)
    int             type;
{
    char            make_fn[MAKE_FUNC_LENGTH];
    char          *shared_lib_name,
                   *fb_version();
    Pixfunc        generic_make;
    struct pixlib  *p = pixlib,
                   *q = p;
    void	   *dlopen();

    if (type <= 0)
	return NULL;	/* this short circuits many many unecessary directory
				searches */
    sprintf(make_fn, "_fb%d_make", type);

    if (!pixlib) {
	pixlib = (struct pixlib *) malloc(sizeof (struct pixlib));
	p = pixlib;
        q = p;
	bzero(p, sizeof (*p));
    }

    while (p && p->shlib_fd) {
	if (generic_make = (Pixfunc)
	    dlsym(p->shlib_fd, make_fn))
	    return generic_make;
	q = p;
	p = p->next;
    }

    p = q;

    if (p && p->shlib_fd) {
	p->next = (struct pixlib *) malloc(sizeof (struct pixlib));
	if (!p->next)
	    bzero(p, sizeof (*p));
	else
	    p = p->next;
	bzero(p, sizeof (*p));
    }


    if (!(shared_lib_name = fb_version(type)))
      return NULL;
    p->shlib_fd = dlopen(shared_lib_name, 1);

    if (!p->shlib_fd)
	return NULL;


    if (!(generic_make = (Pixrect * (*) ())
	  dlsym(p->shlib_fd, make_fn))) {
	dlclose(p->shlib_fd);
	p->shlib_fd = 0;
	return NULL;
    }

    return generic_make;

}


static char    *
fb_version(type)
    int             type;
{
    /*
     * get the highest minor number for libfb_{fb_type}.so.%d.%d .
     */
    struct
    {
	char           *dir_name;
	char           *lib_name;
	int             sh_major,
	                sh_minor;
    }               shlib_search;

    DIR            *dirp;
    struct dirent  *dp;
    int             lib_length,
                    i,
                    j;
    char           *es,
                   *oes,
                   *full_name,
                   *current,
                   *library_path;
    char           *get_next_name();
    extern char    *getenv();
    char	    shared_lib_name[256];


    if ((library_path = getenv("LD_LIBRARY_PATH")) == NULL)
	library_path = "/usr/lib";
    if (!(es = strdup(library_path)))
	return NULL;

    bzero(&shlib_search, sizeof (shlib_search));
    shlib_search.sh_major = SMALL_INTEGER;
    oes = es;
    sprintf(shared_lib_name, "libfb_%d.so.", type);

    while ((current = get_next_name(&es)) != NULL)
    {
	lib_length = strlen(shared_lib_name);
	dirp = opendir(current);
	while (dp = readdir(dirp))
	{
	    if (dp->d_namlen >= lib_length &&
		!strncmp(dp->d_name, shared_lib_name, lib_length))
	    {
		char           *version_string;

		es = NULL;
		version_string = dp->d_name + lib_length;
		i = strtol(version_string, &version_string, 10);
		version_string++;
		j = strtol(version_string, (char **) NULL, 10);
		if (shlib_search.sh_major < i)
		{
		    shlib_search.sh_major = i;
		    shlib_search.sh_minor = j;
		    shlib_search.dir_name = current;
		    shlib_search.lib_name = dp->d_name;
		}
		else if (shlib_search.sh_major == i &&
			 shlib_search.sh_minor < j)
		{
		    shlib_search.sh_minor = j;
		    shlib_search.dir_name = current;
		    shlib_search.lib_name = dp->d_name;
		}
	    }
	}
	closedir(dirp);
    }
    if (!shlib_search.dir_name)
	return NULL;
    full_name = (char *) malloc(strlen(shlib_search.dir_name) +
		       strlen(shlib_search.lib_name) + 2);

    if (full_name)
    {
	strcpy(full_name, shlib_search.dir_name);
	strcat(full_name, "/");
	strcat(full_name, shlib_search.lib_name);
    }
    free(oes);
    return full_name;
}

/*
 * Extract list of directories needed from colon separated string.
 */
static char    *
get_next_name(list)
    char          **list;
{
    char           *lp = *list;
    char           *cp = *list;;

    if (lp != NULL && *lp != '\0') {
	while (*lp != '\0' && *lp != ':')
	    lp++;
	if (*lp == ':') {
	    *lp = '\0';
	    *list = lp + 1;
	    if (**list == '\0')
		*list = NULL;
	}
	else
	    *list = NULL;
    }
    return (cp);
}

#endif
