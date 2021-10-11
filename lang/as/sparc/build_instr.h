/*	@(#)build_instr.h	1.1	10/31/94	*/

/* structs.h must be included BEFORE this file */

#define MASK_RS1 (MASK(5) << 14)

/* macros for building instructions from their constituent fields */
#define MAKEINSTR_1(op,disp30)			\
	( (INSTR)    (  (    (op) << 30)|	\
			((disp30)      )	\
	)	     )

#define MAKEINSTR_2A(op,rd,op2,disp22)		\
	( (INSTR)    (  (    (op) << 30)|	\
			(    (rd) << 25)|	\
			(   (op2) << 22)|	\
			((disp22)      )	\
	)	     )

#define MAKEINSTR_2B(op,a,cond,op2,disp22)		\
	( (INSTR)    (  (    (op) << 30)|	\
			(     (a) << 29)|	\
			(  (cond) << 25)|	\
			(   (op2) << 22)|	\
			((disp22)      )	\
	)	     )

#define MAKEINSTR_3(op,rd,op3,rs1,imm,low13)	\
	( (INSTR)    (  ( (op) << 30)	|	\
			( (rd) << 25)	|	\
			((op3) << 19)	|	\
			((rs1) << 14)	|	\
			((imm) << 13)	|	\
			((low13)    )		\
	)	     )

		/* OPF may also be an ASI in MAKEINSTR_3REG */
#define MAKEINSTR_3REG(op,rd,op3,rs1,opf,rs2)	\
	 ( MAKEINSTR_3(op,rd,op3,rs1,0, (((opf)<<5)|(rs2)) ) )

#define MAKEINSTR_3IMM(op,rd,op3,rs1,simm13)	\
	 ( MAKEINSTR_3(op,rd,op3,rs1,1, ((simm13)&MASK(13)) ) )
