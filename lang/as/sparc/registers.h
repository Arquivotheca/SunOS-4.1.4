/*	@(#)registers.h	1.1	94/10/31 SMI */
/*
 * This file contains stuff to deal with the names of registers.
 *
 * The data types Regno and register_set must, unforunately, be
 * contained in structs.h, because they are used in the Node 
 * and operand structs there, and we don't want to require the
 * inclusion of this file everywhere Node is referenced.
 * THIS IS AN UNHAPPPY COMPROMISE.
 */

/* #define'd names which begin with '_' are not intended to be referenced
 * outside of this header file.
 */

/*----------------------------------------------------------------------------
	Count of registers to keep track of for Live/Dead info:

		# of
		reg's	register(s)
		-----	-----------------
		 32	GP 0-31
		 32	FP 0-31
		  1	Y
		  1	WIM
		  1	PSR
		  1	TBR
		  1	FSR
		  1	FQ
		  1  	CSR
		  1	CQ
		  1	ICC's, as a unit
		  1	FCC, as a unit
		  1	Memory
		 ---	
		 75 (total)
		 ^----------- _NB_ALL_RES is #define'd to this # in "structs.h"

	later:	+32	CP 0-31		(NOT included (yet)!)
 ----------------------------------------------------------------------------*/

   /* basic register numbering */
#define _RGP(n)	(n)
#define _RFP(n)	(n)
#define _RY	0
#define _RWIM	1
#define _RPSR	2
#define _RTBR	3
#define _RFSR	4	/* Floating-point Status Register	*/
#define _RFQ	5	/* Floating-point Queue			*/
#define _RCSR	6	/* Coprocessor Status Register		*/
#define _RCQ	7	/* Coprocessor Queue			*/
#define _RICC	8	/* Integer Condition Code		*/
#define _RFCC	9	/* Floating-point Condition Code	*/
#define _RMEMORY 10	/* this is needed for reg read/write/use info */

	/* register numbers (values to put in a Regno).
	 * these values are assumed by indices into the n_regs field in
	 * "struct node", in its related macros, and in get_reg_group().
	 */
#define    _GPREGS   (0*32)	/* base# for general-purpose registers	*/
#define    _FPREGS   (1*32)	/* base# for floating-point registers	*/
#define    _MISCREGS (2*32)	/* base# for misc. registers		*/
#define    _CPREGS   (3*32)	/* base# for coprocessor registers	*/
/***#define    _NONE_REG ???	/* base# for the "NONE" register	*/
#define RN_GP(n)   ((Regno)(_GPREGS+(n)))
#define RN_FP(n)   ((Regno)(_FPREGS+(n)))
#define RN_Y	   ((Regno)(_MISCREGS+_RY      ))
#define RN_WIM     ((Regno)(_MISCREGS+_RWIM    ))
#define RN_PSR     ((Regno)(_MISCREGS+_RPSR    ))
#define RN_TBR     ((Regno)(_MISCREGS+_RTBR    ))
#define RN_FSR     ((Regno)(_MISCREGS+_RFSR    ))
#define RN_FQ      ((Regno)(_MISCREGS+_RFQ     ))
#define RN_CSR     ((Regno)(_MISCREGS+_RCSR    ))
#define RN_CQ      ((Regno)(_MISCREGS+_RCQ     ))
#define RN_ICC     ((Regno)(_MISCREGS+_RICC    ))
#define RN_FCC     ((Regno)(_MISCREGS+_RFCC    ))
#define RN_MEMORY  ((Regno)(_MISCREGS+_RMEMORY ))
#define RN_CP(n)   ((Regno)(_CPREGS+(n)))
#define RN_NONE	   ((Regno)0) /* to assign a "null" value to a Regno */

	/* 5 bits are needed, as there are a maximum of 32 registers of
	 * any one type.
	 */
#define _RAW_REGNO_BITS_NEEDED  5
	/* RAW_REGNO() is used to extract GP or FP reg # */
#define RAW_REGNO(regno) ( (int) ((regno) & MASK(_RAW_REGNO_BITS_NEEDED)) )

#define RN_GP_GBL(n) RN_GP( 0+(n))	/* the Global registers */
#define RN_GP_OUT(n) RN_GP( 8+(n))	/* the Out    registers */
#define RN_GP_LCL(n) RN_GP(16+(n))	/* the Local  registers */
#define RN_GP_IN(n)  RN_GP(24+(n))	/* the In     registers */

	/*
	 * The names of some well-known registers, for convenience.
	 */
#define RN_G0	RN_GP_GBL(0)	/* the zero register & first Global */
#define RN_O0	RN_GP_OUT(0)	/* the first Out register */
#define RN_L0	RN_GP_LCL(0)	/* the first Local register */
#define RN_I0	RN_GP_IN(0)	/* the first In register */
#define RN_SP	RN_GP(14)	/* the Stack Pointer */
#define RN_FPTR	RN_GP(30)	/* the Frame Pointer */
#define RN_RETPTR  RN_GP(31)	/* the return address and last GP register */
#define RN_CALLPTR RN_GP(15)	/* caller's return address */


/* The following are convenience macros for use during the Optimization pass */
#define IS_A_GP_REG(regno)   ( ((regno) >= RN_GP(0)) && ((regno) <= RN_GP(31)) )
#define IS_GP_GBL_REG(regno) ( ((regno) >= RN_GP_GBL(0)) && \
				((regno) <= RN_GP_GBL(7)) )
#define IS_GP_IN_REG(regno)  ( ((regno) >= RN_GP_IN(0)) && \
				((regno) <= RN_GP_IN(7)) )
#define IS_GP_OUT_REG(regno) ( ((regno) >= RN_GP_OUT(0)) && \
				((regno) <= RN_GP_OUT(7)) )
#define IS_GP_LCL_REG(regno) ( ((regno) >= RN_GP_LCL(0)) && \
				((regno) <= RN_GP_LCL(7)) )
#define IS_AN_FP_REG(regno)  ( ((regno) >= RN_FP(0)) && ((regno) <= RN_FP(31)) )

#define IS_Y_REG(regno)		( (regno) == RN_Y )
#define IS_WIM_REG(regno)	( (regno) == RN_WIM )
#define IS_PSR_REG(regno)	( (regno) == RN_PSR )
#define IS_TBR_REG(regno)	( (regno) == RN_TBR )
#define IS_FSR_REG(regno)	( (regno) == RN_FSR )
#define IS_FQ_REG(regno)	( (regno) == RN_FQ  )
#define IS_CSR_REG(regno)	( (regno) == RN_CSR )
#define IS_CQ_REG(regno)	( (regno) == RN_CQ  )
#define IS_A_CP_REG(regno)  ( ((regno) >= RN_CP(0)) && ((regno) <= RN_CP(31)) )
#define IS_MEM_REG(regno)	( (regno) == RN_MEMORY  )
#define IS_CSR_OR_FSR(regno)    ( IS_CSR_REG(regno) || IS_FSR_REG(regno) )


/*---------------------------------------------------------------------------*/

/*
 * Functions for dealing with register groups or classes.
 */

	/* The following produce STATIC bit masks for register-groups.
	 * The function get_reg_group(regno) may be used to get a register's
	 * group DYNAMICALLY.
	 */
#define RM_GP	   ( (unsigned long int) ( 1 << (((_GPREGS)  >>5)    ) ) )
#define RM_FP	   ( (unsigned long int) ( 1 << (((_FPREGS)  >>5)    ) ) )
#define RM_CP	   ( (unsigned long int) ( 1 << (((_CPREGS)  >>5)    ) ) )
#define RM_misc(n) ( (unsigned long int) ( 1 << (((_MISCREGS)>>5)+(n)) ) )
#define RM_Y	   RM_misc(_RY)
#define RM_WIM	   RM_misc(_RWIM)
#define RM_PSR	   RM_misc(_RPSR)
#define RM_TBR	   RM_misc(_RTBR)
#define RM_FSR	   RM_misc(_RFSR)
#define RM_FQ	   RM_misc(_RFQ)
#define RM_CSR	   RM_misc(_RCSR)
#define RM_CQ	   RM_misc(_RCQ)
#if ((_MISCREGS)>>5) > 30
	@error!@
#endif
#define RM_OTHER   ( (unsigned long int) ( 1 << 31) )

/*---------------------------------------------------------------------------*/

/*
 * Functions for manipulating the register_set types of the Node structure.
 */

	/*
	 * access to whole sets
	 */
#define _REG_read	0
#define _REG_written	1
#define _REG_live	2
#define _REG_nflags	3	/* 3 flags per reg: Read, Written, Live */

#define REGS_READ( nodep )	((nodep)->n_regs[_REG_read])
#define REGS_WRITTEN( nodep )	((nodep)->n_regs[_REG_written])
#define REGS_LIVE( nodep )	((nodep)->n_regs[_REG_live])

	/*
	 * the IN function for register_set's
	 */
#define	_REGset_in( set, regno ) \
	(((set).r[(regno)>>_RAW_REGNO_BITS_NEEDED] & (1<<((regno)&MASK(_RAW_REGNO_BITS_NEEDED)))) != 0)
	/* REG_IS_READ:
	 *	is register "regno" READ    by the instr'n in "nodep"?
	 */
#define REG_IS_READ(nodep, regno)	_REGset_in(REGS_READ(nodep), regno )
	/* REG_IS_WRITTEN:
	 *	is register "regno" WRITTEN by the instr'n in "nodep"?
	 */
#define REG_IS_WRITTEN(nodep, regno)	_REGset_in(REGS_WRITTEN(nodep), regno )
	/* REG_IS_LIVE   :
	 *	is register "regno" LIVE    in the instr'n in "nodep"?
	 */
#define REG_IS_LIVE(nodep, regno)	_REGset_in(REGS_LIVE(nodep), regno )

