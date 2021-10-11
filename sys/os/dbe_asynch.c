/*	@(#)dbe_asynch.c 1.1 94/10/31 SMI */

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/ioctl.h>
#include <sys/file.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/stat.h>
#include <sys/vnode.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/page.h>

#include <specfs/snode.h>
#include <sys/conf.h>
#include <sys/buf.h>

#include <sys/asynch.h>
#include <sys/dbe_asynch.h>

#define ASSERTIONS
#define AUIODEBUG  1

#ifdef ASSERTIONS
#define assert(ex)    {if (!(ex)){(void)printf("Assertion failed: file \"%s\", line %d\n", __FILE__, __LINE__);}}
#else
#define assert(ex)
#endif ASSERTIONS

/*
 * dbe_asynch.c
 *
 * Asynchronous disk I/O support intended for DBMS vendors. It works
 * with raw disk partitions only.
 *
 * An I/O request (called through aioread()/aiowrite() kernel calls) is inserted
 * directly into the device driver queue.  After the completion of the
 * physical data transfer, the interrupt handler calls dbe_aiodone() and 
 * the requested is put on the `completed' queue. The process polls
 * for the completion of the operation with the aiowait() kernel call.
 * 
 * Entry points:
 *   Kernel calls:
 *
 *   Other:
 *	dbe_arw()       - Initiate asynchronous i/o operation.
 *	dbe_aiodone()   - Called from biodone when an operation has completed.
 *	dbe_aio_comp_check() - Called from aiowait() to check i/o completion.
 *	dbe_aio_comp_reap()  - Called from aiowait() to return i/o completion.
 *	dbe_astop()     - Called on process exit,exec to stop i/o(s).
 *
 * Implementation limitations:
 *  	- Up to AUION i/os can be in the system at any one time.
 *  	  Up to AUIO_PROC_N UNIX process may use dbasynch at any one time.  
 *	  If the limits of dbe_asynch are execeeded, then dbe_arw() returns 
 *	  a special status indicating that the i/o should be performed by
 *	  the ASYNCHIO and LWP facility.
 *
 *  	- dbe_asynchio is dependent on the ASYNCHIO option.
 *
 *  	- a process can use dbe_asynchio and ASYNCHIO simultaneously.
 *
 *	- when a process exits, the process waits till all initiated
 *	  asynchronous I/O completes.
 *
 *	- when a process calls exec, the process waits till all initiated
 *	  asynchronous I/O completes.
 */

#ifdef AUIODEBUG
int auiodebug = 0;
#define AUIOPRINTF if (auiodebug > 0) printf
#endif
	
/* DBE asynchio is enabled if dbe_asynchio_enabled is set to 1 */
int dbe_asynchio_enabled = 1;

#define AUION   	500

/* Maximum number of processes allowed to use DBE asynchio concurrentcly */
#define AUIO_PROC_N	10		

	static struct auio auio[AUION];

/* Number of I/O that are queued on some device driver. */
static int dbe_aiodriver_n;			

/* Number of completed I/O in the completion queue. */
static int dbe_aiodone_n;			     

/* Index of highest slot used in dbe_aio_info[] */
static int dbe_aio_info_n;			     

static struct dbe_aio_info	dbe_aio_info[AUIO_PROC_N];

/* List of available auio blocks */
static struct auio *auio_avail;

/* Statistics */
static int auio_readcnt, auio_writecnt, auio_awaitcnt;		     
/*
 * DOKV counters.
 *
 * System writes: writes_dokv
 * System reads:  reads__dokv
 */
long reads__dokv = 0;
long writes_dokv = 0;
long aiowaitdokv = 0;

static faultcode_t dai_lockpages(/* dai, a, len */);
static faultcode_t dai_unlockpages(/* dai, a, len */);
static void daipglocksalloc(/* dai, incr */);

/* auio_init() - called once to initialize data structures */
static auio_init()
{
	register int	i;
	static int		auio_initialized = 0;
	
	if (auio_initialized)
		return;
	
	/* Create the list of available auio blocks */
	for (i = 0; i < AUION - 1; i++) {
		auio[i].auio_next = &auio[i + 1];
	}
	
	auio[AUION - 1].auio_next = NULL;
	
	auio_avail = &auio[0];
	
	auio_initialized = 1;
}

#define AUIO_LOOKUP		0		/* Lookup info for proc */
#define AUIO_LOOKUP_ADD_IT	1		/* Lookup & add if not found */

struct dbe_aio_info*
auio_lookup_proc(p, add_it)
	register struct proc		*p;
	int				add_it;	/* !0 means add if not found */
{
	struct dbe_aio_info		*dai;
	register int			i;
	int				found_it;

	/* Must be called at splbio() or higher */

	/*
	 * The # of elements are few (say <= 10) so the search is simple.
	 */
	found_it = 0;
	dai = (struct dbe_aio_info *)NULL;

	for (i = 0; i < dbe_aio_info_n; i++) {
		if (dbe_aio_info[i].dai_procp == p) {
			/*
			 *  Got it.
			 */
			found_it = 1;
			dai = &dbe_aio_info[i];
			break;
		} else if (add_it && dai == NULL &&
			dbe_aio_info[i].dai_flags == DAI_AVAIL) {
			/* 
			 * Remember earliest available slot
			 */
			dai = &dbe_aio_info[i];
		}
	}

	if (!found_it) {
		if (add_it) {
			if (dai == NULL) {
				/* Allocate a new slot */
				if ((dbe_aio_info_n + 1) >= AUIO_PROC_N) {
				    return (NULL);
				} else {
				    dai = &dbe_aio_info[ dbe_aio_info_n++ ];
				}
			}
			dai->dai_procp = p;
			dai->dai_pid = p->p_pid;
			dai->dai_comp_count = 0;
			dai->dai_pending_count = 0;
			dai->dai_comp_first = (struct auio *)NULL;
			dai->dai_comp_last = (struct auio *)NULL;
		}
	} else {
		/* 
		 *  found one, make sure the info is still valid
		 */
		if (dai->dai_pid != p->p_pid) {
			/* 
			 *  Must be an old entry, fill in correct info.
			 */
			assert(dai->dai_flags == DAI_AVAIL);
			dai->dai_pid = p->p_pid;
#ifdef AUIODEBUG
			assert(dai->dai_comp_count == 0);
			assert(dai->dai_pending_count == 0);
			assert(dai->dai_comp_first == NULL);
			assert(dai->dai_comp_last == NULL);
			assert(dai->dai_pglocklast == 0);
#endif AUIODEBUG
		}
	}

	return (dai);
}

dbe_arw(rw)
	enum uio_rw rw;
{
	register struct dbe_aio_info *dai;
	register struct file *fp;
	register struct vnode *vp;
	int			error;
	register struct auio *pauio;
	int			s;
	
	register struct a {
		int	fdes;
		char	*cbuf;
		u_int	bufs;
		off_t	offset;
		int	whence;
		aio_result_t *result;
	} *uap = (struct a *)u.u_ap;
	
	if (!dbe_asynchio_enabled) {
		return (DBE_AIO_UNAVAIL);		/* not enabled now */
	}

	auio_init();
	
	GETF(fp, uap->fdes);
        if ((fp->f_flag & (rw == UIO_READ ? FREAD : FWRITE)) == 0) {
                u.u_error = EBADF;
		return (DBE_AIO_ERROR);
        }
	/*
	 * Only try to do DBE asynch i/o for character special files.
	 */
	if (fp->f_type != DTYPE_VNODE ||
	    (vp = (struct vnode *)fp->f_data)->v_type != VCHR) {
		return (DBE_AIO_UNAVAIL);		
	}

	/*
	 *  Looks good so far, so keep simple stats on DBE i/o's
	 */
	if (rw == UIO_READ) {
		auio_readcnt++;
		reads__dokv++;
	} else {
		auio_writecnt++;
		writes_dokv++;
	}

	/*
	 * Check to make sure the result pointer is good.
	 */
	if (fuword((caddr_t)uap->result) == -1) {
		u.u_error = EFAULT;
		return (DBE_AIO_ERROR);
	}

	s = splbio();
	/*
	 * Get a per-process completion structure for the current user.
	 */
	if ((dai= auio_lookup_proc(u.u_procp, AUIO_LOOKUP_ADD_IT)) == NULL) {
		(void)splx(s);
		return (DBE_AIO_UNAVAIL);		
	}
	dai->dai_flags |= DAI_BUSY;		/* Mark it as busy */
	(void)splx(s);

#ifdef AUIODEBUG
	AUIOPRINTF("dbe_arw(%d,%d,%d,%d,%d,%d)\n", 
		   uap->fdes,
		   uap->cbuf,
		   uap->bufs,
		   uap->offset,
		   uap->whence,
		   uap->result);
#endif
	

	/*
	 * If write, make sure filesystem is writable.
	 * XXX Is it necessary for raw disk I/O?
	 */
	
	if ((rw == UIO_WRITE) ) {
		if (isrofile(vp)) {
			s = splbio();
			if (dai->dai_pending_count + dai->dai_comp_count == 0) {
				dai->dai_flags &= ~DAI_BUSY;
			}
			(void)splx(s);
			u.u_error = EROFS;
			return (DBE_AIO_ERROR);
		}
	}
	
	
#ifdef AUIODEBUG
	AUIOPRINTF("device %d/%d\n", major(vp->v_rdev), minor(vp->v_rdev));
#endif  
	
	/*
	 * Find first available auio block.
	 */
	s = splbio();
	pauio = auio_avail;
	if (pauio != NULL) {
		auio_avail = pauio->auio_next;
		dbe_aiodriver_n++;	
		dai->dai_pending_count++;
	} else {
		/* None available */
		if (dai->dai_pending_count + dai->dai_comp_count == 0) {
			dai->dai_flags &= ~DAI_BUSY;
		}
		(void)splx(s);
		return (DBE_AIO_UNAVAIL);
	}
	(void)splx(s);
	
	pauio->auio_base = uap->cbuf;
	pauio->auio_len = uap->bufs;
	pauio->auio_result = uap->result;
	pauio->auio_nbytes = 0;
	pauio->auio_error = 0;
	pauio->auio_dai = dai;
/*	pauio->auio_offset = uap->offset;  */

	/*
	 * Calculate offset from offset and whence.
	 */
        switch (uap->whence) {
        case L_INCR:
                pauio->auio_offset = fp->f_offset += uap->offset;
                break;
        case L_XTND: {
                struct vattr vattr;

                u.u_error = VOP_GETATTR((struct vnode *)fp->f_data, &vattr,
                    u.u_cred);
                if (u.u_error)
                        return;
                pauio->auio_offset = uap->offset + vattr.va_size;
                break;
        }
        case L_SET:
                pauio->auio_offset = uap->offset;
                break;
        default:
                return (EINVAL);
        }

	pauio->auio_flags = AUIO_BUSY;
	
	error = doaio(vp, pauio, rw);
	
	/* Put auio block on auio_avail list */
	if (error != 0) {

		s = splbio();
		dbe_aiodriver_n--;		
		dai->dai_pending_count--;
		pauio->auio_next = auio_avail;
		auio_avail = pauio;
		pauio->auio_flags = AUIO_AVAIL;
		if (dai->dai_pending_count + dai->dai_comp_count == 0) {
			dai->dai_flags &= ~DAI_BUSY;
		}
		(void)splx(s);

		/*
		 *  If operation not supported by DBE, go try regular aio.
		 */
		if (error == EOPNOTSUPP) {
			return (DBE_AIO_UNAVAIL);		
		}
	}
	
	u.u_error = error;
	return (DBE_AIO_SUCCESS);
}


/*ARGSUSED*/
int doaio(vp, pauio, rw)
	struct vnode *vp;
	register struct auio *pauio;
	enum uio_rw rw;
{
	register struct snode *sp;
	dev_t 	dev;
	extern int  mem_no;
	int		brw;
	int		(*strategy)();
	int   	(*_auio_getstrategy())();
	
	sp = VTOS(vp);
	dev = (dev_t)sp->s_dev;
	if (rw != UIO_READ && rw != UIO_WRITE)
		panic("spec_rdwr");
	
	smark(sp, SACC);
	
	if (rw == UIO_WRITE) 
		smark(sp, SUPD|SCHG);
	
	brw = (rw == UIO_WRITE) ? B_WRITE : B_READ;
	
	if (vp->v_type == VCHR) {
		
		if (cdevsw[major(dev)].d_str) {
			return (EOPNOTSUPP);	/* It's a stream, not a disk */
		}
		
		if (strategy = _auio_getstrategy(major(dev))) 
			return (aphysio(strategy, dev, pauio, brw));
		else
			return (EOPNOTSUPP);
	}  
	
	return (EOPNOTSUPP);
}

aphysio(strat, dev, pauio, rw)
	int		(*strat)();
	dev_t dev;
	struct auio *pauio;
	int		rw;			     /* B_READ or B_WRITE */
{
	extern int maxphys;
	struct proc *p = u.u_procp;
	register struct buf *bp = &pauio->auio_buf;
	faultcode_t fault_err;
	char *a;
	int s, error = 0;

	/*
	 * Check minimum alignment and block size.
	 */
	if (pauio->auio_offset & (DEV_BSIZE-1)) {
		return (EINVAL);
	} else if (pauio->auio_len & (DEV_BSIZE - 1)) {
		return (EINVAL);
	}

	if (useracc(pauio->auio_base, (u_int)pauio->auio_len,
		    rw == B_READ ? B_WRITE : B_READ) == NULL)
		return (EFAULT);
	
	if (pauio->auio_len > maxphys) {
		return (EOPNOTSUPP);		/* Too big for DBE to handle */
	}

	bp->b_error = 0;
	bp->b_proc = p;
	bp->b_un.b_addr = pauio->auio_base;
	
	bp->b_flags = B_BUSY | B_PHYS | rw | B_ASYNC;
	bp->b_dev = dev;
	bp->b_blkno = btodb(pauio->auio_offset);
	
	bp->b_bcount = pauio->auio_len;

	/* minphys(bp); */
	
	a = bp->b_un.b_addr;

#ifdef notdef	
	fault_err = as_fault(p->p_as, a, (u_int)pauio->auio_len, F_SOFTLOCK,
			     rw == B_READ? S_WRITE : S_READ);
#endif notdef
	fault_err = dai_lockpages(pauio->auio_dai, a, (u_int)pauio->auio_len);

	if (fault_err != 0) {
		/*
		 * Even though the range of addresses were valid
		 * and had the correct permissions, we couldn't
		 * lock down all the pages for the access we needed.
		 * (e.g. we needed to allocate filesystem blocks
		 * for rw == B_READ but the file system was full).
		 */
		if (FC_CODE(fault_err) == FC_OBJERR)
			error = FC_ERRNO(fault_err);
		else
			error = EFAULT;
		bp->b_flags |= B_ERROR;
		bp->b_error = error;
		s = splbio();
		if (bp->b_flags & B_WANTED)
			wakeup((caddr_t)bp);
		(void) splx(s);
		bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS);
		return (error);
	}
	if (buscheck(bp) < 0  || ((int )bp->b_un.b_addr & 0x03)) {
		/*
		 * The io was not requested across legal pages, or not
		 * aligned mod 4.
		 */
		bp->b_flags |= B_ERROR;
		bp->b_error = error = EFAULT;

#ifdef notdef
		if (as_fault(p->p_as, a, (u_int)pauio->auio_len, F_SOFTUNLOCK,
			     bp->b_flags & B_READ == B_READ? S_WRITE : S_READ) != 0)
			panic("aphysio unlock");
#endif notdef
		(void) dai_unlockpages(pauio->auio_dai, a, (u_int)pauio->auio_len);
	} else {

		/* Handle transfer of 0 chars as a special case */
		if (bp->b_bcount == 0) {
			s = splbio();
			dbe_aiodone(bp);  /* Fake interrupt callback function */
			(void)splx(s);
		}
		
		/*
		 *  Keep from trying to swap this process while we have
		 *  memory locked down--we could panic otherwise.
		 *  This process isn't swappable until it's memory is unlocked
		 *  by dbe_aio_comp_reap().
		 */
		s = splbio();
		p->p_flag |= SPHYSIO;
		(void)splx(s);

		error = 0;
		(void) (*strat)(bp);
	}
	
	return (error);
}

dbe_aio_comp_check(p, dpp)
	struct proc			*p;
	struct dbe_aio_info		**dpp;
{
	register struct dbe_aio_info	*dai;
	int				status;
	int				s;
	
	auio_init();

	s = splbio();
	if (dbe_aiodone_n + dbe_aiodriver_n < 1) { 
		status = DBE_AIO_NONE_OUTSTANDING;

	} else if ((dai = auio_lookup_proc(p, AUIO_LOOKUP)) == NULL) {
		status = DBE_AIO_NONE_OUTSTANDING;

	} else if (dai->dai_comp_count > 0) {
		status = DBE_AIO_COMPLETE;

	} else if (dai->dai_pending_count > 0) {
		status = DBE_AIO_PENDING;

	} else {
		status = DBE_AIO_NONE_OUTSTANDING;
	}
	(void)splx(s);
	if (dpp != NULL) {
		*dpp = dai;			/* return pointer to caller */
	}

	return (status);
}

caddr_t
dbe_aio_comp_reap(dai)
	register struct dbe_aio_info	*dai;
{
	struct proc			*p = u.u_procp;
	register struct buf 		*bp;
	char				*a;
	int				i;
	int				s;
	register struct auio  		*pauio;
	int				found_one;
	aio_result_t			result;
	label_t				lqsave;
	caddr_t				uresultp;	/* in user space */
	
	auio_init();
	
	s = splbio();

#ifdef AUIODEBUG
	AUIOPRINTF("dbe_aio_comp_reap called for pid= %d, comp_count=%d\n", dai->dai_procp->p_pid, dai->dai_comp_count);
#endif
	assert(dai->dai_procp == p);
	assert(dai->dai_flags & DAI_BUSY);
	assert(dai->dai_comp_count > 0);

	/* 
	 * Get the first auio block from the auio_comp list.
	 */
	assert(dai->dai_comp_first != NULL);
	pauio = dai->dai_comp_first;
	dai->dai_comp_first = pauio->auio_next;
	if (dai->dai_comp_first != NULL) 
		dai->dai_comp_first->auio_prev = NULL;
	else
		dai->dai_comp_last = NULL;
	(void) splx(s);

	
	bp = &pauio->auio_buf;
	a = bp->b_un.b_addr;

#ifdef	notdef
	if (as_fault(p->p_as, a, (u_int)pauio->auio_len, F_SOFTUNLOCK,
		     bp->b_flags & B_READ == B_READ? S_WRITE : S_READ) != 0)
		panic("dbe_aio_reap unlock");
#endif	notdef
	(void) dai_unlockpages(dai, a, (u_int)pauio->auio_len);

	result.aio_return = pauio->auio_len - bp->b_resid;
	result.aio_errno = (bp->b_flags & B_ERROR) ? EIO : 0;
	
	uresultp = (caddr_t)pauio->auio_result;

	bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS);
	
	s = splbio();
	/* Put the auio block on auio_avail list. */
	pauio->auio_next = auio_avail;
	auio_avail = pauio;
	
	pauio->auio_flags = AUIO_AVAIL;	     /* make auio block available */

	dbe_aiodone_n--;			    
	dai->dai_comp_count--;
	
	if (!any_aio(u.u_procp, dai)) {
		dai->dai_flags &= ~DAI_BUSY;
		/*
		 *  XXX -- make process swappable again.
		 */
		p->p_flag &= ~SPHYSIO;
	}
	(void)splx(s);

	u.u_error = copyout((caddr_t)&result, uresultp, sizeof(result));
	return (uresultp);
}

int
any_aio(p, dai)
        struct proc *p;
        struct dbe_aio_info *dai;
{
        if (p->p_threadcnt <= 1)
                if ((dai == NULL) && (dai = auio_lookup_proc(p, AUIO_LOOKUP)) == NULL)
                   return (0);
                else if ((dai->dai_comp_count + dai->dai_pending_count) == 0)
                        return (0);
 
        return (1);
}        
 
/* Called from biodone() in vfs_bio.c */
dbe_aiodone(bp)
	struct buf 		*bp;
{
	struct dbe_aio_info	*dai;
	struct auio 		*pauio = (struct auio *) bp;
	int  			s;
	
	s = splbio();	

#ifdef AUIODEBUG
	AUIOPRINTF("dbe_aiodone called for bp= 0x%x, iopid= %d, chan=0x%x, cpid=%d\n", bp, bp->b_proc->p_pid, (long)&bp->b_proc->p_aio_count, u.u_procp->p_pid);
#endif
	pauio->auio_flags |= AUIO_DONE;
	
	/*
	 * Get per-process completion list header.
	 */
	if ((dai = auio_lookup_proc(bp->b_proc, AUIO_LOOKUP)) == NULL) {
		panic("dbe_aiodone: can't find proc for this i/o");
	}

#ifdef AUIODEBUG
	assert(dai->dai_flags & DAI_BUSY);
#endif

	/*
	 * Put auio on the list of completed I/O.
	 */
	if (dai->dai_comp_first == NULL) {
		assert(dai->dai_comp_last == NULL);
		
		pauio->auio_next = pauio->auio_prev = NULL;
		dai->dai_comp_first = dai->dai_comp_last = pauio;
	}
	else {
		pauio->auio_prev = dai->dai_comp_last;
		pauio->auio_next = NULL;
		dai->dai_comp_last->auio_next = pauio;
		dai->dai_comp_last = pauio;
	}
	dai->dai_comp_count++;			
	dai->dai_pending_count--;			

	dbe_aiodone_n++;			
	dbe_aiodriver_n--;

	wakeup((caddr_t) &dai->dai_procp->p_aio_count);
	psignal(dai->dai_procp, SIGIO);
	(void)splx(s);
}

#ifdef TRACE
int dai_sleep;
#endif
/*
 *  Called from:  exit() in kern_exit.c 
 *                execve() in kern_exec.c 
 *                close() in kern_descrip.c 
 */
void
dbe_astop(fd, res)
	int fd;	
	struct aio_result_t *res;
{
	struct proc	*p = u.u_procp;
	struct dbe_aio_info	*dai;
	int done = 0;
	int s;
	int status;
	caddr_t	comp;
	struct timeval	tv;
	
#ifdef AUIODEBUG
	AUIOPRINTF("dbe_astop called %2d, %2d pid=%d\n", fd, res, p->p_pid);
#endif
	/*  Wait for completed I/O 
	 *
	 *  We must reap the completions if we are called from exec() or exit()
	 *  (i.e., fd == AIO_ALL).  But, if called from close()  (i.e.,
	 *  fd != AIO_ALL), then we just wait for all pending I/O's for this
	 *  process to complete, the user process will still need to call 
	 *  aiowait() to dequeue the completions.
	 */
	/* XXX -- Do we need a timeout?  What do we do if we timeout? */

	do {
		s = splbio();
		switch (status = dbe_aio_comp_check(p, &dai)) {
		case DBE_AIO_COMPLETE:
			if (fd == ALL_AIO) {

                            while (dai->dai_pending_count > 0) {
#ifdef TRACE
				dai_sleep++;
				printf("dbe_astop: dai_pending_count = %d \n", dai->dai_pending_count);
#endif
			        sleep((caddr_t)&p->p_aio_count, PZERO|PCATCH);

			    }

			    /*(void) splx(s);
			    s = -1;*/
			    if ((comp = dbe_aio_comp_reap(dai)) == (caddr_t)res)
				    done = 1;
			} else {
			    /*
			     * Called from close().
			     * Wait for all pending i/o's to complete.
			     * XXX -- is this too much of a performance hit?
			     */
			    while (dai->dai_pending_count > 0) {
				sleep((caddr_t)&p->p_aio_count, PZERO|PCATCH);
			    }
			    done = 1;
			}
			break;
		case DBE_AIO_PENDING:
			sleep((caddr_t)&p->p_aio_count, PZERO|PCATCH);
			break;
		case DBE_AIO_NONE_OUTSTANDING:
			done = 1;
			break;
		default:
			panic("dbe_astop: bad status from dbe_aio_comp_check");
		}
		if (s != -1)
			(void) splx(s);
	} while (!done);

}

/* 
 * Get from the major device number of a character device to the corresponding
 * device strategy function.
 */
typedef int (*stratfunc_t)();
#define INITSTRATEGY  ((stratfunc_t)-1)
static stratfunc_t *strattable;

static int (*_auio_getstrategy(chrmajor))()
	int		chrmajor;
{
	register int	i;
	int		(*strategy)();
	int		(*strat)();

	if (chrmajor < 0 || chrmajor >= nchrdev)
		return (stratfunc_t)NULL;

	/*
	 * Treat the id device driver special.
	 */
	if (strat = id_get_strategy(chrmajor))
		return(strat);

	/*
	 * Allocate and initialized the strategy table. Do it only once.
	 */
	if (strattable == NULL) {
		strattable = (stratfunc_t *)
			kmem_alloc(nchrdev * sizeof(strattable[0]));

		for (i = 0; i < nchrdev; i++)
			strattable[i] = INITSTRATEGY;
	}
	
	if (strattable[chrmajor] == INITSTRATEGY) {
		/* 
		 * We haven't filled the entry for this chrmajor yet.
		 * Search bdevsw for an entry with d_open that matches
		 * cdevsw[chrmajor].d_open.
		 * 
		 * NOTE: this doesn't work for id.c. We treat it as a 
		 *	special case above.
		 */
		strattable[chrmajor] = (stratfunc_t)NULL;
		for (i = 0; i < nblkdev; i++) {
			if (cdevsw[chrmajor].d_open == bdevsw[i].d_open) {
				strattable[chrmajor] = bdevsw[i].d_strategy;
				break;
			}
		}
	}

	return (strattable[chrmajor]); 
}    

/*
 * The stuff below was added to prevent deadlock on p_keepcnt. If a process
 * is doing asynchronous I/O on buffers that are the same page it is possible
 * that with the "help" of the pageout daemon the process may get into
 * a state where it is hanging in as_fault waiting for a page p_keepcnt to drop
 * to zero. However, the page is 'kept' by another I/O request (issued
 * by the same process) and the process will hang forever. This can happen
 * only on 4/260 where the variable nopagereclaim is set to one. There is
 * a race condition between the first I/O and the page daemon for the 
 * page.
 *
 * Since we don't want to mess with the VM system, we will prevent the
 * deadlock by never as_fault'ing on pages (p_keepcnt > 0) that are
 * already locked by another I/O. The DBE async module keeps track 
 * of locked pages (a separate list for each process). Each locked down
 * page has its keepcount. If the page is already locked, we just increment
 * the keepcount. We release the page (as_fault SOFTUNLOCK) when the keepcount
 * drops to zero.
 *	
 * XXX - we will assume that all pages are lock R/W. This makes sense
 *       for all situations that we care about.
 */

static faultcode_t
dai_lockpages(dai, addr, len)
	struct dbe_aio_info *dai;
	addr_t		addr;
	u_int		len;
{
	addr_t		a, ba, ea;
	int		i;
	int		freeind;
	struct daipglock *daipglockp;
	int		err;

	ba = (addr_t)((u_int)addr & PAGEMASK);
	ea = (addr_t)(roundup((u_int)addr + len, PAGESIZE));

	for (a = ba; a < ea; a += PAGESIZE) {
		
		freeind = -1;		/* get first free slot */

		for (i = 0; i < dai->dai_pglocklast; i++) {
			daipglockp = &dai->dai_pglocks[i];
			if (a == daipglockp->dl_addr) {
				daipglockp->dl_keepcnt++;
				goto nextpage;
			} else {
				/*
				 * Remember first free slot.
				 */
				if (freeind == -1 &&
				    daipglockp->dl_addr == AVAILDLADDR)
					freeind = i;
			}
		}

		/*
		 * Page is not in the table. Insert it in a free slot.
		 */
		if (freeind == -1) {
			if (dai->dai_pglocklast == dai->dai_pglocksize) {
				/*
				 * Reallocate the page lock table.
				 */
				daipglocksalloc(dai, DAIPGLOCKINCR);
			}
			freeind = dai->dai_pglocklast++;
		}

		daipglockp = &dai->dai_pglocks[freeind];
		daipglockp->dl_addr = a;
		daipglockp->dl_keepcnt = 1;

		/*printf("LOCK PAGE %x\n", daipglockp->dl_addr);*/
		err = as_fault(dai->dai_procp->p_as,  daipglockp->dl_addr, 
		    PAGESIZE, F_SOFTLOCK, S_WRITE);
		if (err) {
			dai_unlockpages(dai, ba, a - ba);
			return (err);
		}

	nextpage: 
		continue;
	}

	return (faultcode_t)0;
}

#ifdef TRACE
struct daipglock *daipglockpp;
addr_t dai_dl_addr;
addr_t bba, eea;
int    dai_keepcnt;

faultcode_t dai_result;
int dai_result_nomap;
int dai_i;
#endif TRACE

static faultcode_t
dai_unlockpages(dai, addr, len)
	struct dbe_aio_info *dai;
	addr_t		addr;
	u_int		len;
{
	struct daipglock *daipglockp;
	int		i;
	addr_t		ba, ea;

#ifdef TRACE
	bba = ba = (addr_t)((u_int)addr & PAGEMASK);
	eea = ea = (addr_t)(roundup((u_int)addr + len, PAGESIZE));
#else
	ba = (addr_t)((u_int)addr & PAGEMASK);
	ea = (addr_t)(roundup((u_int)addr + len, PAGESIZE));
#endif	
	for (i = 0; i < dai->dai_pglocklast; i++) {
#ifdef TRACE
		daipglockpp = daipglockp = &dai->dai_pglocks[i];
		dai_dl_addr = daipglockp->dl_addr;
		dai_keepcnt = daipglockp->dl_keepcnt;
#else
		daipglockp = &dai->dai_pglocks[i];
#endif
		if (daipglockp->dl_addr >= ba && daipglockp->dl_addr < ea) {
			if (--(daipglockp->dl_keepcnt) == 0) {
				/*printf("UNLOCK PAGE %x\n", daipglockp->dl_addr);*/
#ifdef TRACE
				dai_i = i;
				dai_result = as_fault(dai->dai_procp->p_as,  
				    daipglockp->dl_addr, PAGESIZE, F_SOFTUNLOCK,
				    S_WRITE);
				if (dai_result != (faultcode_t)0 ){
				   if (dai_result == FC_NOMAP)
				       dai_result_nomap++;
				    else
					panic("as_fault(F_SOFTUNLOCK)");
				}
#else
				if (as_fault(dai->dai_procp->p_as,  
				    daipglockp->dl_addr, PAGESIZE, F_SOFTUNLOCK,
				    S_WRITE) != (faultcode_t)0) {
					panic("as_fault(F_SOFTUNLOCK)");
				}
#endif
				daipglockp->dl_addr = AVAILDLADDR;
			}
		}
	}

	/*
	 * Adjust dai->dai_pglocklast
	 */
	for (i = dai->dai_pglocklast - 1; i >= 0; i--) {
		daipglockp = &dai->dai_pglocks[i];
		if (daipglockp->dl_addr == AVAILDLADDR)
			dai->dai_pglocklast = i;
		else
			break;
	}
}

static void
daipglocksalloc(dai, incr)
	struct dbe_aio_info	*dai;
	int			incr;
{
	int			oldsize;

	if (dai->dai_pglocks == NULL) {
		dai->dai_pglocks = 
			(struct daipglock *)kmem_alloc(incr * 
			    sizeof(dai->dai_pglocks[0]), 0);
		dai->dai_pglocksize = incr;
		dai->dai_pglocklast = 0;
	} else {
		oldsize = dai->dai_pglocksize;
		dai->dai_pglocks = (struct daipglock *)
			kmem_resize((char *)dai->dai_pglocks, 0, 
			    sizeof(dai->dai_pglocks[0]) * (oldsize + incr), 
			    oldsize);

		dai->dai_pglocksize += incr;
	}
}

#ifdef	sun4m
/*
 * A hack for aiowait to probe if there is any I/O on the completion queue
 * before acquiring the kernel lock.
 *
 * Note that this function is called outside of the kernel lock.
 *
 * XXX - #ifdef sun4m code shouldn't be in sys/os. SunDBE can get away
 *	 with this.
 */
int
dbe_probeawait(ptv)
	struct timeval			*ptv;
{
        struct dbe_aio_info             *dai;
        register int                    i;
        int                             foundit = 0;
	struct proc			*p = masterprocp;
	static int			errcnt;
	struct timeval			atv;

	aiowaitdokv++;
	if (p->p_aio_count > 0)
		return 1;		/* a generic async I/O has completed */
 
        if (ptv) {
                if ((u.u_error = copyin((caddr_t)ptv, (caddr_t)&atv, sizeof (atv))))
                        return 1;	/* let normal syscall handle EFAULT */
		if (atv.tv_usec != 0 || atv.tv_sec != 0)
			return 1;	/* let normal syscall handle non zero timeout */
	} else
		return 1;		/* let normal syscall handle infinite timeout */
	
	/* 
 	 * We come here only if aiowait is called with timeout == {0, 0}.
	 */
        for (i = 0; i < dbe_aio_info_n; i++) {
                if (dbe_aio_info[i].dai_procp == p) {
                        /*
                         *  Got it.
                         */
                        foundit = 1;
                        dai = &dbe_aio_info[i];
                        break;
                } 
        }

	return (foundit && dai->dai_comp_count > 0); 
}
#endif	sun4m

