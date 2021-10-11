/*	@(#)gtseg.h	1.1 94/10/31	*/

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#ifndef _gtseg_h
#define _gtseg_h

/*
 *  direct-port segment private data
 */
struct seggt_data {
	u_int			flag;
	struct gt_cntxt		*cntxtp;	/* pointer to context	*/
	struct seggt_data	*link;		/* shared_list		*/
	dev_t			dev;
	u_int			offset;		/* beginning offset	*/
};

/*
 * seggt_data flags
 */
#define	SEG_SHARED	0x0001
#define SEG_PRIVATE	0x0002
#define SEG_UVCR	0x0004
#define SEG_SVCR	0x0008
#define SEG_LOCK	0x0010

#define SEG_VCR		(SEG_UVCR | SEG_SVCR)


/*
 * accelerator-port segment private data
 */
struct seggta_data {
	caddr_t		 baseaddr;		/* virtual address base	*/
};


/*
 * segment-create args are in fact the segment private data
 */
#define seggt_crargs	seggt_data

int seggt_create();
int seggta_create();


/*
 * Accelerator-port structures
 */

#define GT_PAGESIZE 		4096	/* bytes per gt mmu page	*/
#define GT_PAGESHIFT		  12	/* shift count for gt mmu	*/

#define GT_ENTRIES_PER_LVL1	1024	/* PTPs per level 1 table	*/
#define GT_ENTRIES_PER_LVL2	1024	/* PTEs per level 2 table	*/

/*
 * GT_PAGE_RATIO:		gt pages per host page
 */
#define GT_PAGE_RATIO		(MMU_PAGESIZE / GT_PAGESIZE)
#define GT_LVL2SIZE		(GT_ENTRIES_PER_LVL2 * GT_PAGESIZE)


/*
 * gt page table pointer
 */
struct gt_ptp {
	u_int	ptp_base:20;		/* level 2 table base address	*/
	u_int	ptp_cnt:11;		/* number of valid lvl2 ptrs	*/
	u_int	ptp_v:1;		/* valid bit			*/
};


/*
 * gt page table entry
 */
struct gt_pte {
	u_int	pte_ppn:20;	/* physical page number		*/
	u_int	:8;
	u_int	pte_u:1;	/* pte has been used		*/
	u_int	pte_r:1;	/* referenced			*/
	u_int	pte_m:1;	/* modified			*/
	u_int	pte_v:1;	/* valid			*/
};


/*
 * Level 1 page table
 * XXX: replace the virtual pointers to the level 2 table
 * with code that does the kptov transformation?
 */
struct gt_lvl1 {
	struct gt_ptp	gt_ptp[GT_ENTRIES_PER_LVL1];
	struct gt_lvl2 *gt_lvl2[GT_ENTRIES_PER_LVL1];
};


/*
 * Level 2 page table
 */
struct gt_lvl2 {
	struct gt_pte	 gt_pte[GT_ENTRIES_PER_LVL2];
};


#endif	_gtseg_h
