#ifndef lint
static char sccsid[] = "@(#)pr_make.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright 1986, 1987 by Sun Microsystems, Inc.
 */

/*
 * Pixrect make/unmake utilities
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sun/fbio.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_make.h>

/* fake out _MAP_NEW if compiling on 3.x or whatever */
#ifndef	_MAP_NEW
#define	_MAP_NEW	0x80000000
#endif
#define GP_MAJOR 32
/* Round value up to specified granularity -- gran must be a power of two */
#undef roundup
#define	roundup(val, gran)	((val) + (gran) - 1 & ~((gran) - 1))

#define	ALLOC(type)		((type *) calloc(1, sizeof (type)))

extern char *calloc(), *valloc();
extern caddr_t mmap();

Pixrect *
pr_makefromfd(fd, size, depth, devdata, curdd,
	mmapbytes, privdatabytes, mmapoffset)
	int fd;
	struct pr_size size;
	int depth;
	struct pr_devdata **devdata, **curdd;
	int mmapbytes, privdatabytes, mmapoffset;
{
	return pr_makefromfd_2(fd, size, depth, devdata, curdd,
		privdatabytes, mmapbytes, mmapoffset, 0, 0);
}

Pixrect *
pr_makefromfd_2(fd, size, depth, devdata, curdd, privdatabytes,
	mmapbytes, mmapoffset, mmapbytes2, mmapoffset2)
	int fd;
	struct pr_size size;
	int depth;
	struct pr_devdata **devdata, **curdd;
	int privdatabytes, mmapbytes, mmapoffset, mmapbytes2, mmapoffset2;
{
	Pixrect *pr = 0;
	register struct	pr_devdata *dd = 0;
	struct stat statbuf;
	struct fbinfo finfo;
	int pagesize;

	/* 
	 * check args
	 * stat frame buffer to get device type
	 * allocate and initialize pixrect and private data 
	 */
	if (fd < 0 || devdata == 0 || curdd == 0 ||
		fstat(fd, &statbuf) < 0 ||
		!(pr = ALLOC(Pixrect)) || 
		!(pr->pr_data = 
			(caddr_t) calloc(1, (unsigned) privdatabytes)))
		goto bad;

	pr->pr_size = size;
	pr->pr_depth = depth;

        if (major(statbuf.st_rdev) == GP_MAJOR &&
            ioctl(fd, FBIOGINFO, &finfo) >= 0)
          statbuf.st_rdev = makedev(major(statbuf.st_rdev), finfo.fb_unit);
	/* look for a devdata structure for this device */
	for (dd = *devdata; dd; dd = dd->next)
		if (dd->rdev == statbuf.st_rdev && dd->count++ > 0) {
			*curdd = dd;

			return pr;
		}

	/* didn't find devdata, make a new one */
	if (!(dd = ALLOC(struct pr_devdata)))
		goto bad;

	/* caller will close fd, so dup it (dumb) */
	if ((dd->fd = dup(fd)) < 0) 
		goto bad;

	pagesize = getpagesize();

	mmapbytes = roundup(mmapbytes, pagesize);
	if (mmapoffset != roundup(mmapoffset, pagesize))
		goto bad;

	if (mmapbytes2) {
		mmapbytes2 = roundup(mmapbytes2, pagesize);
		if (mmapoffset2 != roundup(mmapoffset2, pagesize))
		goto bad;
	}

	/* try new style mmap */
	if ((dd->va = mmap((caddr_t) 0, dd->bytes = mmapbytes, 
		PROT_READ | PROT_WRITE, MAP_SHARED | _MAP_NEW, 
		dd->fd, mmapoffset)) != (caddr_t) -1) {

		/* map second piece if any */
		if (mmapbytes2 &&
			(dd->va2 = mmap((caddr_t) 0, dd->bytes2 = mmapbytes2, 
			PROT_READ | PROT_WRITE, MAP_SHARED | _MAP_NEW, 
			dd->fd, mmapoffset2)) == (caddr_t) -1) {
			(void) munmap(dd->va, dd->bytes);
			goto bad;
		}
	}
	/* new wave mmap failed, try old fashioned (valloc) method */
	else {
		caddr_t mapit();

		/* prevent munmap on unmake (swap hole!) */
		dd->bytes = 0;

		/* valloc/map each piece */
		if (!(dd->va = mapit(dd->fd, mmapbytes, mmapoffset)) ||
			mmapbytes2 && 
			!(dd->va2 = mapit(dd->fd, mmapbytes2, mmapoffset2)))
			goto bad;
	}

	/* fill in new devdata and link into head of list */
	dd->rdev = statbuf.st_rdev;
	dd->count = 1;
	dd->next = *devdata ? (*devdata)->next : 0;
	*devdata = dd;
	*curdd = dd;

	return pr;


	/* error occurred -- clean up */
bad:
	if (pr) {
		/* free pixrect and possibly pixrect data */
		if (pr->pr_data)
			free((char *) pr->pr_data);
		free((char *) pr);

		/* free devdata, close fd */
		if (dd) {
			if (dd->fd >= 0) 
				(void) close(dd->fd);
			free((char *) dd);
		}
	}

	*curdd = 0;
	return 0;
}

static caddr_t
mapit(fd, bytes, offset)
	int fd, bytes, offset;
{
	register caddr_t p;

	if (!(p = (caddr_t) valloc((unsigned) bytes)))
		(void) fprintf(stderr, 
			"pr_open: out of swap space (needed %dK)\n",
			(unsigned) bytes >> 10);
	else if (mmap(p, bytes, PROT_READ | PROT_WRITE, MAP_SHARED, 
		fd, offset)) {
		free((char *) p);
		p = 0;
	}

	return p;
}

pr_unmakefromfd(fd, devdata)
	int fd;
	struct pr_devdata **devdata;
{
	register struct	pr_devdata *dd, *ddprev = 0;
	struct fbinfo finfo;
	struct stat statbuf;

	/* get the device number */
	if (fstat(fd, &statbuf) < 0)
		return PIX_ERR;

        if (major(statbuf.st_rdev) == GP_MAJOR &&
            ioctl(fd, FBIOGINFO, &finfo) >= 0)
          statbuf.st_rdev = makedev(major(statbuf.st_rdev), finfo.fb_unit);

	/* look for the relevant devdata structure */
	for (dd = *devdata; dd; ddprev = dd, dd = dd->next) 
		/* found matching device number? */
		if (statbuf.st_rdev == dd->rdev) {
			/*
			 * If the reference count goes to zero, we would 
			 * like to unmap the frame buffer, close its fd, 
			 * and release the virtual space.
			 * 
			 * Unfortunately, we can't do that with the old
			 * VM system because the mmap call probably 
			 * released the swap space for the region the
			 * frame buffer was mapped over.
			 */
			if (dd->bytes && --dd->count <= 0) {
				(void) munmap(dd->va, dd->bytes);
				if (dd->bytes2)
					(void) munmap(dd->va2, dd->bytes2);
				(void) close(dd->fd);

				/* unlink devdata */
				if (ddprev == 0) 
					*devdata = dd->next;
				else
					ddprev->next = dd->next;

				free((char *) dd);
			}

			return 0;
		}

	/* couldn't find the devdata */
	return PIX_ERR;
}
