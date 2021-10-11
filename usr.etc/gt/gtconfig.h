/*	@(#)gtconfig.h	1.1 94/10/31 */

/*
 * Copyright (c) 1990 by Sun Microsystem, Inc.
 */

#define FBNAME     "/dev/gt"

#define WCS_DATE	0
#define WCS_VERSION 	1
#define EXEC_VERSION 	2
#define KERNEL_MCB_PTR 	3

/* SU addresses */
#define SU_CSR_ADDR		0x00080000	/* SU processor CSR */

#define SU_DSP_RUN_SHIFT	0		/* Shift value for run bits */
#define SU_DSP_RUN_MASK		0x00000007	/* Run bits for 3 DSPs */
#define SU_DSP_RUN_0		0x00000001	/* Run bit for DSP 0 */
#define SU_DSP_RUN_1		0x00000002	/* Run bit for DSP 1 */
#define SU_DSP_RUN_2		0x00000004	/* Run bit for DSP 2 */
#define SU_DSP_DISABLE_SHIFT	3		/* Shift value for disable */
#define SU_DSP_DISABLE_MASK	0x00000038	/* Disable bits for 3 DSPs */
#define SU_DSP_DISABLE_0	0x00000008	/* Disable bit for DSP 0 */
#define SU_DSP_DISABLE_1	0x00000010	/* Disable bit for DSP 1 */
#define SU_DSP_DISABLE_2	0x00000020	/* Disable bit for DSP 2 */
#define SU_INPUT_DATA_READY	0x00000800	/* Data ready at input FIFO */
#define SU_OUTPUT_DATA_READY	0x00001000	/* Data ready at output FIFO */
#define SU_FIFO_NOT_FULL	0x00002000	/* SU fifo NOT full */
#define SU_FIFO_FULL_INT_ENA	0x00004000	/* SU full interrupt enable */


/* Edge Walker addresses */

#define EW_CSR_ADDR		0x00080010	/* EW CSR */
#define EW_CSR_AA		0x00000001	/* Antialiasing on */
#define EW_CSR_SF		0x00000002	/* Semaphore flag */
#define EW_CSR_RESET		0x00000004	/* Hold EW in reset state */
#define EW_CSR_HELD		0x00000008	/* EW Held by SI */
#define EW_CSR_ODR		0x00000010	/* Output data ready */
#define EW_CSR_AD		0x00000020	/* Accepting data */
#define EW_CSR_ISE		0x00000040	/* Input sequence error */
#define EW_CSR_CFIFO		0x00000F80	/* Command FIFO */
#define EW_CSR_ISE_INT_ENA	0x40000000	/* Enable interrupt for ISE */


/* Span Interpolator addresses */

#define SI_CSR1_ADDR		0x00080014	/* SI CSR 1 */
#define SI_CSR1_SF		0x00000001	/* Semaphore flag */
#define SI_CSR1_ICM		0x00000002	/* Indexed color mode */
#define SI_CSR1_RESET		0x00000004	/* Hold SI in reset state */
#define SI_CSR1_HELD		0x00000008	/* SI Held by PBM */
#define SI_CSR1_AD		0x00000020	/* Accepting data */
#define SI_CSR1_ISE		0x00000040	/* Input sequence error */
/* Ignore other fields here */
#define SI_CSR1_XMSIP_AF	0x10000000	/* X MSIP almost full */
#define SI_CSR1_ZMSIP_AF	0x20000000	/* Z MSIP almost full */
#define SI_CSR1_ISE_INT_ENA	0x40000000	/* Enable interrupt for ISE */

#define SI_CSR2_ADDR		0x00080018	/* SI CSR 2 */
#define SI_CSR2_EF		0x0000001F	/* Endpoint Filter (diag) */
#define SI_CSR2_LWF		0x000007E0	/* Line Width Filter (diag) */
#define SI_CSR2_F_ENA		0x80000000	/* Enable normal filtering */


/* Pixel Bus Mux addresses */

#define PBM_FEFBAS_ADDR		0x0008001c	/* FE FB address space */
#define PBM_FECSR_ADDR		0x00080020	/* FE PBM CSR */

#define HKPBM_FECS_AFBSS	0x00000001	/* FE state set 1	*/

#define HKAS_IMAGE_DEPTH	0x00000007	/* Pixel image/depth	*/
#define HKAS_DEPTH		0x00000005	/* Pixel depth		*/
#define HKAS_IMAGE		0x00000004	/* Pixel image		*/
/* See hk_fb_regs.h HKPBM_FECS for these bit definitions */

#define PBM_HFBAS_ADDR		0x00090000	/* Host FB address space */
#define PBM_HCSR_ADDR		0x00090004	/* Host PBM CSR */

/* Global types */

typedef struct fbinit_type fbinit_type;
struct fbinit_type {
    unsigned offset;		/* MS+/Sbus offset of a reg */
    unsigned ppdata;		/* data sent to PP */
};


#define	MS_OFFSET_MASK	0x00ffffff

#define HKPBM_HCS_HSRP             0x00000010      /* Host stalling RP */
#define HKPBM_FECS_SILE            0x00000080      /* SI loopback enable */



/*
 * GT addressability
 */
struct	gt_ctrl {
	int		flags;
	int		fd;			/* file descriptor for GT   */
	u_char 		*baseaddr;		/* base addr of dev	    */
	rp_host		*rp_host_regs;		/* ptr to RP host ctrl regs */
	fbi_reg		*fbi_regs;		/* ptr to FBI ctrl regs	    */
	fbi_pfgo	*fbi_pfgo_regs;		/* ptr to PF ctrl regs	    */
	fe_ctrl_sp	*fe_ctrl_regs;		/* ptr to FE ctrl regs	    */
	fe_i860_sp	*fe_i860_ctrl_regs;	/* ptr to i860 ctrl regs    */
	fbo_screen_ctrl	*fbo_screen_ctrl_regs;	/* ptr to FBO ctrl regs	    */
};
