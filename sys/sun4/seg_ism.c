#ifndef lint
static	char sccsid[] = "@(#)seg_ism.c 1.1 94/10/31 SMI";
#endif

/* 
 * seg_ism.c - 'intimate shared memory' segment driver
 */

#include <sys/param.h>
#include <sys/vm.h>
#include <sys/user.h>
#include <sys/mman.h>
#include <sys/debug.h>

#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/cpu.h>

#include <vm/hat.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/anon.h>
#include <vm/rm.h>
#include <vm/page.h>

#include <sun/seg_kmem.h>
#include <sun4/seg_ism.h>

#define	DEBUG				     /* XXX */

#define SEGISMKADDR	(addr_t)0x0	

static int ismmem_pct = 90;		/* max percent memory allowed for ISM */
static int ismmem_used;			/* # of pages currently used for ISM */
extern int maxmem;

#define OK_ALLOCATE(npages) ((ismmem_used + (npages)) *  100 < maxmem * ismmem_pct)

/********** segismu driver **************************************************/

/* Private segment functions */
static int	segismu_dup(/* seg, newsegp */);
static int	segismu_unmap(/* seg, addr, len */);
static int	segismu_free(/* seg */);
static faultcode_t	segismu_fault(/* seg, addr, len, type, rw */);
static int	segismu_setprot(/* seg, addr, size, prot */);
static int	segismu_checkprot(/* seg, addr, size, prot */);
static u_int	segismu_swapout(/* seg */);

static int	segismu_badop();

struct	seg_ops segismu_ops = {
	segismu_dup,		
	segismu_unmap,		
	segismu_free,		
	segismu_fault,
	segismu_badop,			     /* faulta */
	segismu_badop,			     /* unload */
	segismu_setprot,
	segismu_checkprot,
	segismu_badop,			     /* kluster */
	segismu_swapout,
	segismu_badop,			     /* sync */
	segismu_badop,			     /* incore */
};


int segismu_create(seg, argsp)
	struct seg *seg;
	caddr_t argsp;
{
	struct segismu_data 	*dp;

	dp = (struct segismu_data *)kmem_zalloc(sizeof(struct segismu_data));
	*dp = *(struct segismu_data *)argsp; /* structure assignment */

	seg->s_data = (caddr_t)dp;	     /* private data */
	seg->s_ops = &segismu_ops;

	dp->siu_shmism_data->refcnt++;

	/* Map 'intimately' */
	hat_intimate_map(seg, as_segat(dp->siu_shmism_data->as, SEGISMKADDR)); 

	return 0;
}


static int segismu_dup(seg, newseg)
	struct seg	*seg, *newseg;
{
	struct segismu_data 	*dp;

	dp = (struct segismu_data *)kmem_zalloc(sizeof(struct segismu_data));
	*dp = *(struct segismu_data *)seg->s_data;

	newseg->s_data = (caddr_t)dp;	     /* private data */
	newseg->s_ops = &segismu_ops;

	dp->siu_shmism_data->refcnt++;

	/* Map 'intimately' */
	hat_intimate_map(newseg, as_segat(dp->siu_shmism_data->as, SEGISMKADDR)); 

	return 0;
}

static int segismu_unmap(seg, addr, len)
	struct seg	*seg;
{
	hat_intimate_unmap(seg);
	seg_free(seg);
	return 0;
}

static int segismu_free(seg)
	struct seg	*seg;
{
	struct segismu_data 	*dp;

	dp = (struct segismu_data *)seg->s_data;

	/* Destroy shmism_data structure if this was the last reference. */
	if (--(dp->siu_shmism_data->refcnt) == 0)
		ismdestroy(dp->siu_shmism_data);		     

	kmem_free((caddr_t)seg->s_data, sizeof(struct segismu_data));
	return 0;
}

static faultcode_t segismu_fault(seg, addr, len, type, rw)
	struct seg		*seg;
	addr_t			addr;
	u_int			len;
	enum fault_type 	type;
	enum seg_rw 		rw;
{
	struct seg		*segismk;
	struct segismu_data *	dp;
	struct pte		pte;

	switch (type) {
	case F_SOFTUNLOCK:
		return (0);
	case F_SOFTLOCK:
                mmu_getpte(addr, &pte);
                if (pte.pg_v)
                        return (0);
		/* else fall through */
	case F_INVAL:
		/* This fault happens after a process was swapped out */
/*
		printf("segismu_fault F_INVAL addr %x len %d\n",
		    addr, len); 
		printf("ctx %x, reg %x, seg %x, pte %x\n",
			map_getctx(), map_getrgnmap(), map_getsgmap(),
			map_getpgmap());
*/
		dp = (struct segismu_data *)seg->s_data;
		segismk = as_segat(dp->siu_shmism_data->as, SEGISMKADDR);
		hat_intimate_map(seg, segismk);	/* link pseudo pmegs */
		return (0);
	case F_PROT:
	default:
		printf("segismu_fault F_PROT called\n");   
		return (FC_NOMAP);	     /* XXX should not happen */
	}
	/* NOTREACHED */
}

static u_int	segismu_swapout(seg)
	struct seg	*seg;
{
	/* 
	 * XXX we will allow swapping later. segismu_fault will map the
	 * segment in again.
	 */
	/*
	printf("segismu_swapout called\n");
	*/
	return (0);			     /* no pages actually freed */
}

static int segismu_setprot(seg, addr, len, prot)
	struct seg *seg;
	addr_t addr;
	u_int len, prot;
{
	struct as	*saveas = mmu_getctx()->c_as;
	addr_t		eaddr;
	u_int		pprot;

	/*
	printf("segismu_setprot called\n");
	*/

	if (prot == 0)			     /* XXX */
		return (-1);

	hat_setup(seg->s_as);
	pprot = hat_vtop_prot(prot);

	for (eaddr = addr + len; addr < eaddr; addr += PAGESIZE) {
		struct pte tpte;

		mmu_getpte(addr, &tpte);
		tpte.pg_prot = pprot;
		tpte.pg_nc = 0;		/* XXX should use hat layer */
		mmu_setpte(addr, tpte); 
	}

 out:
	hat_setup(saveas);
	return (0);
}

static int segismu_checkprot(seg, addr, size, prot)
{
	/* XXX - We created the ISM segment with PROT_ALL */
	return 0;
}


static int segismu_badop()
{
	panic("segismu_badop");
	/*NOTREACHED*/
}


/***************** segismk driver *******************************************/
/* Note: sun/seg_kmem.c was the used as the sample. */

static void segismk_free_pages(/* seg, addr, len */);

/* Private segment functions */
static int	segismk_unmap(/* seg, addr, len */);
static int	segismk_free(/* seg */);
static int	segismk_setprot(/* seg, addr, size, prot */);

static int	segismk_badop();

struct	seg_ops segismk_ops = {
	segismk_badop,		
	segismk_unmap,		
	segismk_free,		
	segismk_badop,			     /* fault */
	segismk_badop,			     /* faulta */
	segismk_badop,			     /* unload */
	segismk_setprot,		    
	segismk_badop,			     /* checkprot */
	segismk_badop,			     /* kluster */
	(u_int (*)()) NULL,		     /* swapout */
	segismk_badop,			     /* sync */
	segismk_badop,			     /* incore */
};

int segismk_create(seg, argsp)
	struct seg *seg;
	caddr_t argsp;
{
	addr_t			addr = seg->s_base;
	u_int			len = seg->s_size;
	int			error;
	struct as		*saveas = mmu_getctx()->c_as;

	hat_setup(seg->s_as);

	error = segismk_allocate_pages(seg, addr, len);

	if (error)
		goto out;

	seg->s_ops = &segismk_ops;
	seg->s_data = (char *)(1);
	
	error = 0;
	bzero(addr, len);		     /* Clear the pages */
 out:
	hat_ism_lockpmgs(seg->s_as);	     /* lock all pmegs */
	hat_setup(saveas);
	return error;
}

static int segismk_unmap(seg, addr, len)
	struct seg	*seg;
{
	seg_free(seg);

	return 0;
}

static int segismk_free(seg)
	struct seg	*seg;
{
	struct as	*saveas = mmu_getctx()->c_as;

	hat_setup(seg->s_as);

	hat_ism_unlockpmgs(seg->s_as);	     /* unlock all pmegs */

	segismk_free_pages(seg, seg->s_base, seg->s_size);
	hat_setup(saveas);

	return 0;
}

static int segismk_setprot(seg, addr, len, prot)
	struct seg *seg;
	addr_t addr;
	u_int len, prot;
{
	printf("segismk_setprot called\n"); 	
	return (-1);
}

static int segismk_badop()
{

	panic("segismk_badop");
	/*NOTREACHED*/
}


static int segismk_allocate_pages(seg, addr, len)
	struct seg *seg;
	addr_t addr;
	u_int len;
{
	struct page 	*pp;
	struct pte 	tpte;
	addr_t		v;

	/*
	 * We allow up to ismmem_pct percent of maxmem physical memory
	 * pages to be consumed by ISM segments.
	 */
	if (!OK_ALLOCATE(btopr(len)))
		return (ENOMEM);

	for (v = addr; v < addr + len; v += PAGESIZE) {
 
		/* 
		 * We are allocating one page at a time now. We may optimize
		 * this by allocating pages in chunks. However, we may not
		 * ask for all the pages at once because of 
		 * the 'max_page_get' limit.
		 */
		pp = rm_allocpage(seg, v, PAGESIZE, 1); /* canwait == 1 */
		hat_setup(seg->s_as);     /* rm_allocpage() may switch to process context */

		if (pp == (struct page *)NULL) {
			(void)segismk_free_pages(seg, addr, (u_int)(v - addr));
			return (ENOMEM);
		}

		hat_mempte(pp, PROT_ALL /*& ~PROT_USER*/, &tpte);
		hat_pteload(seg, v, pp, tpte, PTELD_LOCK);
		page_sub(&pp, pp);
	}

	ismmem_used += btopr(len);
	return (0);
}

static void segismk_free_pages(seg, addr, len)
	register struct seg *seg;
	addr_t addr;
	u_int len;
{
	struct page *pp;
	struct pte tpte;

	ismmem_used -= btopr(len);

	for (; (int)len > 0; len -= PAGESIZE, addr += PAGESIZE) {
		mmu_getpte(addr, &tpte);
		pp = page_numtookpp(tpte.pg_pfnum);
		if (tpte.pg_type != OBMEM || pp == NULL)
			panic("segismk_free_pages");
		hat_unlock(seg, addr);
		hat_unload(seg, addr, PAGESIZE);
		PAGE_RELE(pp);		     /* this should free the page */
	}

}

struct shmism_data *ismcreate(size)
{
	int			error;
	struct shmism_data 	*dt;
	struct as		*as;

	as = as_alloc();
	if (error = as_map(as, SEGISMKADDR, size, segismk_create, (caddr_t)0)) {
		as_free(as);
		u.u_error = error;
		return NULL;
	}

	dt = (struct shmism_data *) kmem_zalloc(sizeof (struct shmism_data));
	dt->refcnt = 1;
	dt->size   = size;
	dt->anon   = NULL;
	dt->as	   = as;

	printf("ISM segment created size 0x%x (=%d) bytes allocated.\n", 
	       dt->size, dt->size);	

	return dt;
}


void ismdestroy(dt)
	struct shmism_data	*dt;
{
	printf("ISM segment destroyed size 0x%x (=%d) bytes freed.\n", 
	       dt->size, dt->size);	

	ASSERT(dt->refcnt == 0);
	as_unmap(dt->as, SEGISMKADDR, dt->size);
	as_free(dt->as);
	kmem_free((caddr_t)dt, sizeof(struct shmism_data));
}

struct shmism_data *ismlookup(as, addr)
	struct as *as;
	addr_t addr;
{
	register struct seg *seg;

	/* Find an address segment that contains this address */
	if ((seg = as_segat(as, addr)) == NULL)
		return (NULL);

	/* Make sure it is a segismu segment */
	if (seg->s_ops != &segismu_ops)
		return (NULL);

	return ((struct shmism_data *)seg->s_data);
}


int ismalign_p(addr)
	addr_t	addr;
{
	int	align;

#if defined(sun4)
	/* Check alignment */
	switch (cpu) {
	case CPU_SUN4_470:		     /* SMEG align (16Mbytes) */
		align = SMGRPSIZE;
		break;
	case CPU_SUN4_110:		     /* PMEG align (256k boundary) */
	case CPU_SUN4_330:
	case CPU_SUN4_260:
	default:
		align = PMGRPSIZE;
		break;
	}
#endif sun4

#if defined(sun4c)
	align = PMGRPSIZE;
#endif sun4

	return (((int)addr & (align - 1)) == 0);
}

