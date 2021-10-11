/*	@(#)spt.c  1.1 94/10/31 SMI	*/

#include <sys/param.h>
#include <sys/user.h>
#include <sys/mman.h>

#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h>
#include <vm/page.h>

#include <sun4m/mmu.h>
#include <sun4m/pte.h>
#include <sun4m/spt.h>

#ifdef  MULTIPROCESSOR
#include <machine/async.h>
#include "percpu.h"
#endif  MULTIPROCESSOR

#define	SEGSPTADDR	(addr_t)0x0
#define CHECK_MAXMEM(x) ((spt_used + (x)) * 100 < maxmem * spt_max)

static int      spt_max = 90; /* max percent among maxmem for spt */
static u_int      spt_used;     /* # pages used for spt */
int	spt_num = 6;	/* maximal number of spt segments permitted */
struct spt_data **sptp = NULL;	/* vector of sptp's */
extern int	maxmem;  
extern union ptpe percpu_ptpe[];        /* machdep.c */

int segspt_create(/* seg, argsp */);
int segspt_unmap(/* seg, raddr, ssize */);
int segspt_lockop(/* seg, raddr, ssize, func */);
int segspt_free(/* seg */);
int segspt_badops();
void segspt_free_pages();

struct seg_ops segspt_ops = {
        segspt_badops,
        segspt_unmap,
        segspt_free,
        segspt_badops,
        segspt_badops,
        segspt_badops,
        segspt_badops,
        segspt_badops,
        segspt_badops,
        (u_int (*)()) NULL,     /* swapout */
        segspt_badops,
        segspt_badops,
        segspt_lockop,
        segspt_badops
};

int  segspt_shmdup(/* seg, newseg */);
int  segspt_shmunmap(/* seg, raddr, ssize */);
int  segspt_shmfree(/* seg */);
faultcode_t segspt_shmfault();
int  segspt_shmsetprot();
int  segspt_shmcheckprot();
int  segspt_shmbadops();

struct seg_ops segspt_shmops = {
        segspt_shmdup,
        segspt_shmunmap,
        segspt_shmfree,
        segspt_shmfault,
        segspt_shmbadops,
        segspt_shmbadops,
        segspt_shmsetprot,
        segspt_shmcheckprot,
        segspt_shmbadops,
        (u_int (*)()) NULL,
        segspt_shmbadops,
        segspt_shmbadops,
        segspt_shmbadops,
        segspt_shmbadops
} ;

struct spt_data *
sptcreate(size)
	int size;
{
	int err;
	int i;
	struct spt_data	*spt;
	struct as		*as;

	as = as_alloc(); /* in order to make 0x0 as the start addr */

	/* To get special treatment in hat_setup() for spt 

	as->a_hat.hat_sv_uunix = (struct user *)&segspt_ops; 

	as->a_hat.hat_oncpu = cpuid | 8; */

	spt = (struct spt_data *) kmem_zalloc(sizeof (struct spt_data));
	spt->as = as;

	if (sptp == NULL) {
		/* create sptp area... */
		sptp = (struct spt_data **)
		       kmem_zalloc((sizeof(struct spt_data *) * (spt_num + 1)));
		sptp[0] = (struct spt_data *) spt_num;
	} else {
		/* verify sptp area */
		if (sptp[0] != (struct spt_data *) spt_num) {
			printf("sptcreate: resetting spt_num value\n");
			spt_num = (int) sptp[0];
		}
	}
	/* find free slot */
	for(i = 1; i <= spt_num; i++) {
		if (sptp[i] == NULL)
			break;
	}
	if (i > spt_num) {
		err = EAGAIN;
		goto bad;
	}

	/* insert spt in free slot */
	sptp[i] = spt;

 	if (err = as_map(as, SEGSPTADDR, size, segspt_create, (caddr_t) 0)) {
		sptp[i] = NULL;
		goto bad;
	}

	spt->refcnt = 1;
	spt->size = size;
	spt->anon = NULL;/* mark that shared page table is used */

#ifdef SUNDBE
	printf("ISM segment created size 0x%x (=%d) bytes allocated.\n", 
	       spt->size, spt->size);	
#endif
	return (spt);

 bad:
	as_free(as);
	kmem_free((caddr_t) spt, sizeof (struct spt_data));
	u.u_error = err;
	return NULL;
}

#define L1_FREE 0

void
sptdestroy(spt)
	struct spt_data	*spt;
{
	int s = splvm();
	int i, cnt;
	struct as *as = spt->as;
	struct l1pt *l1pt = as->a_hat.hat_l1pt;
	struct ptp *ptpp;

#ifdef SPT_DEBUG
		printf("hat_oncpu = %d, cpuid = %d \n", 
			   as->a_hat.hat_oncpu, cpuid);
#endif
#ifdef SUNDBE
	printf("ISM segment destroyed size 0x%x (=%d) bytes freed.\n", 
	       spt->size, spt->size);	
#endif
	if (sptp == NULL)
		panic("sptdestroy: no sptp\n");

	/* verify spt_num */
	if (spt_num != (int) sptp[0]) {
		printf("sptdestroy: resetting spt_num value\n");
		spt_num = (int) sptp[0];
	}

	/* remove spt from array */
	for (i = 1, cnt = 0; i <= spt_num; i++) {
		if (sptp[i] == spt)
			sptp[i] = NULL;
		if (sptp[i] == NULL)
			cnt++;
	}

	if (cnt == spt_num) {	/* all entries empty, deallocate array */
		kmem_free((caddr_t) sptp,
			  (sizeof(struct spt_data *) * (spt_num + 1)));
		sptp = NULL;
	}

	/* Map the percpu area for this CPU.
	 *  There is no u area for spt->as.
   	 *  So we have to hat_map_percpu by ourselves which was done in  
	 *  hat_map_percpu(&((struct seguser *)(as->a_hat.hat_sv_uunix))->segu_u);
  	 */	
	s = splvm();
        ptpp = &(l1pt->ptpe[getl1in(VA_PERCPUME)].ptp);
	stphys(va2pa((addr_t)ptpp), percpu_ptpe[cpuid].ptpe_int);
	(void) splx(s);

	as_unmap(as, SEGSPTADDR, spt->size);

	/*as->a_segs = NULL;  supposedly done in seg_free */

 	/* hat_l1free is done in as_free */	

	as_free(as);

	kmem_free((caddr_t)spt, sizeof (struct spt_data));
}
/*
 * called from seg_free().
 * free (i.e., unlock, unmap, return to free list)
 *  all the pages in the given seg.
 */
int
segspt_free(seg)
	struct seg	*seg;
{
	struct as 	*save = u.u_procp->p_as;
	int sx;

#ifdef SPT_DEBUG
printf("segspt_free is called 1 \n");
#endif
	sx = splvm();
	hat_setup(seg->s_as); 

#ifdef SPT_DEBUG
printf("segspt_free is called 2 \n");
#endif

	(void)segspt_free_pages(seg, seg->s_base, seg->s_size);

#ifdef SPT_DEBUG
printf("segspt_free is called 3 \n");
#endif
	hat_setup(save);
	(void)splx(sx);
	
	return 0;
}
/*
 * called from as_ctl(, MC_LOCK,)
 *
 */
int
segspt_lockop(seg, raddr, ssize, func)
	struct seg 	*seg;
	addr_t		raddr;
	u_int		ssize;
	int		func;
{
	/* for spt, as->a_paglck is never set 
	 * so this routine should not be called.
	 */

	printf("segspt_lockop called \n");
}
int
segspt_unmap(seg, raddr, ssize)
	struct seg 	*seg;
	addr_t		raddr;
	u_int		ssize;
{

#ifdef SPT_DEBUG
printf("segspt_unmap is called \n");
#endif

	if (raddr == seg->s_base && ssize == seg->s_size) {
		seg_free(seg);
		return (0);
	}
	else
	   panic("segspt_unmap - unexpected address \n");
}
int
segspt_badops()
{	
	panic("segspt_badops is called");
}
int
segspt_create(seg, argsp)
	struct seg	*seg;
	caddr_t		argsp;
{
	
	int		err;
	struct as 	*saveas = u.u_procp->p_as;
	u_int		len  = seg->s_size;
        addr_t          addr = seg->s_base;
	int		sx;

	sx = splvm();
	hat_setup(seg->s_as); /* get an ctx */

	err = segspt_alloc_map_pages(seg, addr, len); 
	if (err)
	   goto out;

	seg->s_ops = &segspt_ops;
	seg->s_data = (caddr_t) &segspt_ops; /* seg driver is attached */
	bzero(addr, len); /* XXXX */

out:
	hat_setup(saveas);
	(void)splx(sx);

#ifdef SPT_DEBUG
printf("from segspt_create: allocation is done \n");
#endif
	return(err);
}

int
segspt_alloc_map_pages(seg, addr, len)
	struct seg	*seg;
	addr_t		addr;
	u_int		len;
{
	struct page 	*pp;
	addr_t		v;

#ifdef SPT_DEBUG
printf("segspt_alloc_map_pages \n");
#endif
	if (!CHECK_MAXMEM(btopr(len)))
		return (ENOMEM);

	for (v = addr; v < addr + len; v += PAGESIZE) {

		pp = page_get(PAGESIZE, 1); /* get a new page */
		hat_setup(seg->s_as);  /* maybe context has been changed ? */

		if (pp == (struct page *)NULL) {
			printf("page_get gets NULL page");
			(void)segspt_free_pages(seg, addr, (u_int)(v - addr));
			return (ENOMEM);
		}

		hat_memload(seg, v, pp, PROT_ALL, PTELD_LOCK);
		/*page_sub(&pp, pp); */
	}
	spt_used += btopr(len);
	return (0);
}
void
segspt_free_pages(seg, addr, len)
	struct seg 	*seg;
	addr_t		addr;
	u_int		len;
{
	union  ptpe     *ptpe;
	struct page 	*pp;
	int sx;

	spt_used -= btopr(len);

	sx = splhigh();

        for (; (int)len > 0; len -= PAGESIZE, addr += PAGESIZE) {
		
		ptpe = hat_ptefind(seg->s_as, addr); 
		pp = ptetopp(&ptpe->pte);
		/* printf("before pp = %x \n", pp); */
        	if (pte_valid(&ptpe->pte))
			hat_unlock_ptbl(ptetoptbl(&ptpe->pte));
		else
			panic("invalid pte in spt \n");
		
		hat_unload(seg, addr, PAGESIZE);
		/*printf("pp = %x \n", pp);*/
		PAGE_RELE(pp);
	}
	(void)splx(sx);
}

int
segspt_shmattach(seg, argsp)
	struct seg	*seg;
	caddr_t		argsp;
{
	int s;
   	struct spt_data	*spt = (struct spt_data *) argsp;
        struct l1pt *l1pt = spt->as->a_hat.hat_l1pt;
	struct ptp *ptpp;

#ifdef SPT_DEBUG
printf("segspt_shmattach is called \n");
#endif
	seg->s_ops = &segspt_shmops;
	seg->s_data = argsp;

	spt->refcnt++;

	/* make the new u area known to other processor (per cpu area) */

        /*if (spt->as->a_hat.hat_oncpu != (cpuid | 8)) { 
#ifdef SPT_DEBUG
	     printf("hat_oncpu = %d, cpuid = %d \n",
		      spt->as->a_hat.hat_oncpu, cpuid);
#endif
	     hat_map_percpu(&((struct seguser *)(spt->as->a_hat.hat_sv_uunix))->segu_u);
	} */

        s = splvm();
	ptpp = &(l1pt->ptpe[getl1in(VA_PERCPUME)].ptp);
	stphys(va2pa((addr_t)ptpp), percpu_ptpe[cpuid].ptpe_int);
	(void) splx(s);

	share_pagetables(seg, spt->as->a_segs);

	return 0;
}

#define L1PTSHIFT 24

share_pagetables(attachseg, sptseg)
	struct seg	*attachseg;
	struct seg	*sptseg;
{

	struct as  	*as = attachseg->s_as;
	caddr_t		addr = attachseg->s_base;
	u_int		size = attachseg->s_size;
	struct as   	*sptas = sptseg->s_as;
	struct as 	*saveas = u.u_procp->p_as;
        union  ptpe	a_ptpe, s_ptpe;
	union  ptpe   	*ptpa, *ptps;
	unsigned  	l1ptpa, l1ptps;
	int		i;

#ifdef SPT_DEBUG
printf("share_pagetables: as = %x, sptseg->s_as = %x, saveas = %x \n", 
	 as, sptseg->s_as, saveas);
#endif

	hat_setup(as); /* take care of as_dup */

	ptps = &s_ptpe;
	ptpa = &a_ptpe;

	for (l1ptps = va2pa((addr_t) sptas->a_hat.hat_l1pt),
	     i = 0,
	     l1ptpa = (va2pa((addr_t) as->a_hat.hat_l1pt) +
			  getl1in(addr) * sizeof (struct ptp));
	     	i < ((size + L2PTOFFSET) >> L1PTSHIFT);
	     l1ptps += sizeof (struct ptp), 
	     i++,
	     l1ptpa += sizeof (struct ptp) ){

	     	ptps->ptpe_int = ldphys(l1ptps);
	     	if (pte_entrytype(ptps->ptp) == MMU_ET_INVALID) 
			panic("invalid shared memory l1 ptp");

		ptpa->ptpe_int = ldphys(l1ptpa);
		if (pte_entrytype(ptpa->ptp) != MMU_ET_INVALID)
			panic("already allocated shared memory l1 ptp");

		stphys(l1ptpa, ptps->ptpe_int);

/*	ptetoptbl(ptptopte(ptps->PageTablePointer))->lockcnt++;*/

	}			
	hat_setup(saveas);
}
	
struct spt_data *
spt_lookup(as, addr)
	struct as 	*as;
	addr_t		addr;
{
	struct seg 	*seg;

#ifdef SPT_DEBUG
printf("spt_lookup is called \n");
#endif

	if ((seg = as_segat(as, addr)) == NULL)
		return(NULL);
	if (seg->s_ops != &segspt_shmops)
		return(NULL);

	return((struct spt_data *)seg->s_data);
}
int
segspt_shmunmap(seg, raddr, ssize)
	struct seg 	*seg;
	addr_t		raddr;
	u_int		ssize;
{

#ifdef SPT_DEBUG
printf("segspt_shmunmap is called \n");
#endif
	unshare_pagetables(seg, raddr, ssize);
	seg_free(seg);
	return 0;
}

unshare_pagetables(seg, addr, size)
	struct seg 	*seg;
	addr_t		addr;
	u_int		size;
{
	int 		i;
	struct as	*as = seg->s_as;
	unsigned 	l1ptp;
	union ptpe	a_ptp;
       
#ifdef SPT_DEBUG
	union ptpe	*ptpa;
	ptpa = &a_ptp;

printf("unshare_pagetables is called \n");
#endif

	for(i = 0,
	    l1ptp = va2pa((addr_t)as->a_hat.hat_l1pt) +
			  getl1in(addr) * sizeof (struct ptp);
	    i < ((size + L2PTOFFSET) >> L1PTSHIFT);
	    i++,
	    l1ptp += sizeof (struct ptp)) {
#ifdef SPT_DEBUG
		ptpa->ptpe_int = ldphys(l1ptp);
		if (pte_entrytype(ptpa->ptp) == MMU_ET_INVALID)
		    panic("unsharing invalid l1pte");
#endif
		a_ptp.ptp.EntryType = MMU_ET_INVALID;
		stphys(l1ptp, a_ptp.ptpe_int);
/*	ptetoptbl(ptptopte(ptpa->PageTablePointer))->lockcnt--;*/
	}
}
int
segspt_shmfree(seg)
	struct seg 	*seg;
{
	struct spt_data	*spt;

#ifdef SPT_DEBUG
printf("segspt_shmfree is called \n");
#endif

	spt = (struct spt_data *)seg->s_data;
	--(spt->refcnt);
	return 0;
}

int
segspt_shmsetprot()
{
	printf("segspt_shmsetprot is called \n");
}
faultcode_t
segspt_shmfault(seg, addr, len, type, rw)
        struct seg              *seg;
	addr_t                  addr;
	u_int                   len;
	enum fault_type         type;
	enum seg_rw             rw;

{
	struct seg		*sptseg;
	struct spt_data		*spt;
	union  ptpe		*ptpe;
	struct pte		pte;
	struct ctx		*ctx;

#ifdef SPT_DEBUG
	printf("segspt_shmfault: ");
#endif

	switch (type) {
	case F_SOFTUNLOCK:
#ifdef SPT_DEBUG
		printf("UNLOCK called \n");
#endif
		return (0);
	case F_SOFTLOCK:
		return (0); /* Because we know that every shared memory is 
			     *  already locked 
			     *  and called in the same context.
			     */
#ifdef SPT_DEBUG
		ctx = seg->s_as->a_hat.hat_ctx;
		if (ctx->c_num == mmu_getctx()) {
		   *(u_int *)&pte = mmu_probe(addr); /* faster than hat_ptefind */
		   printf("same context - use mmu_probe ");
		   if (pte_valid(&pte)) {
		       printf("pte valid \n");
		       return (0);
	    	   }
		   else
		       printf("invalid pte for mmu_probe \n");
		}
		else {
		  ptpe = hat_ptefind(seg->s_as, addr);
		  if (pte_valid(&ptpe->pte)) {
#ifdef SPT_DEBUG1
		      printf("SOFTLOCK: found addr = %x \n", addr);
#endif
		      return (0);
		  }
		  printf("using hat_ptefind due to different context\n");
		}
#ifdef SPT_DEBUG1
		   printf("SOFTLOCK: not found addr = %x, seg = %x \n",
			    addr, seg);
#endif
#endif
	case F_INVAL:
#ifdef SPT_DEBUG
		printf("INVAL called \n");
#endif
		spt = (struct spt_data *)seg->s_data;
		sptseg = as_segat(spt->as, SEGSPTADDR);
		share_pagetables(seg, sptseg);
		return(0);
	case F_PROT:
#ifdef SPT_DEBUG
		printf("PROT called");
#endif
	default:
#ifdef SPT_DEBUG
		printf("- reaching default \n");
#endif
		return(FC_NOMAP);
	}
}
int
segspt_shmbadops()
{
	panic("segspt_shmbadops is called \n");
}

/*
 * duplicate the shared page tables
 */

int
segspt_shmdup(seg, newseg)
	struct seg	*seg, *newseg;
{
        struct spt_data *spt = (struct spt_data *)seg->s_data;

#ifdef SPT_DEBUG
printf("segspt_shmdup is called \n");
#endif

	newseg->s_data = (caddr_t) spt;
	newseg->s_ops = &segspt_shmops;

	spt->refcnt++;
/*	share_pagetables(newseg, spt->as->a_segs);*/
	return 0;
}
int
segspt_shmcheckprot(seg, addr, size, prot)
{
	return 0;
}
