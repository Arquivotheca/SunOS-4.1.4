/*	@(#)opcode_ptrs.h	1.1	10/31/94	*/

#define OP_NOP		  0
#define OP_ST		  1
#define OP_STH		  2
#define OP_STB		  3
#define OP_LDF		  4
#define OP_STF		  5
#ifdef SIXTYFOURBIT_CP_REGS
#  define OP_LDF2	  6
#  define OP_STF2	  7
#  define OP_LDF3	  8
#  define OP_STF3	  9
#  define OP_LDC2	 10
#  define OP_STC2	 11
#  define OP_LDC3	 12
#  define OP_STC3	 13
#endif /*SIXTYFOURBIT_CP_REGS*/
#define OP_LDFSR	 14
#define OP_LDFSRA	 15
#define OP_STFSR	 16
#define OP_STFSRA	 17
#define OP_LDC		 18
#define OP_STC		 19
#define OP_LDCSR	 20
#define OP_STCSR	 21
#define OP_LDDF		 22
#define OP_STDF		 23
#define OP_LDDC		 24
#define OP_STDC		 25
#define OP_STDFQ	 26
#define OP_STDCQ	 27
#define OP_ADD		 28
#define OP_ADDCC	 29
#define OP_SUB		 30
#define OP_SUBCC	 31
#define OP_ANDCC	 32
#define OP_OR		 33
#define OP_ANDN		 34
#define OP_XOR		 35
#define OP_XNOR		 36
#define OP_SETHI	 37
#define OP_CALL		 38
#define OP_JMPL		 39
#define OP_WRY		 40
#define OP_WRPSR	 41
#define OP_WRWIM	 42
#define OP_WRTBR	 43
#define OP_RDY		 44
#define OP_RDPSR	 45
#define OP_RDWIM	 46
#define OP_RDTBR	 47
#define OP_UNIMP	 48

#define OP_PSEUDO_ALIGN	 49
#define OP_PSEUDO_SKIP	 50
#define OP_PSEUDO_SEG	 51
#define OP_PSEUDO_BOF	 52
#define OP_PSEUDO_CSWITCH  53

#define OP_BN		 54
#define OP_BE		 55
#define OP_BZ		 56
#define OP_BLE		 57
#define OP_BL		 58
#define OP_BLEU		 59
#define OP_BCS		 60
#define OP_BNEG		 61
#define OP_BVS		 62
#define OP_BNE		 63
#define OP_BNZ		 64
#define OP_BG		 65
#define OP_BGE		 66
#define OP_BGU		 67
#define OP_BCC		 68
#define OP_BPOS		 69
#define OP_BVC		 70
#define OP_BA		 71

#define OP_FBN		 72
#define OP_FBU		 73
#define OP_FBG		 74
#define OP_FBUG		 75
#define OP_FBL		 76
#define OP_FBUL		 77
#define OP_FBLG		 78
#define OP_FBNE		 79
#define OP_FBNZ		 80
#define OP_FBE		 81
#define OP_FBZ		 82
#define OP_FBUE		 83
#define OP_FBGE		 84
#define OP_FBUGE	 85
#define OP_FBLE		 86
#define OP_FBULE	 87
#define OP_FBO		 88
#define OP_FBA		 89

#define OP_CBN		 90
#define OP_CB3		 91
#define OP_CB2		 92
#define OP_CB23		 93
#define OP_CB1		 94
#define OP_CB13		 95
#define OP_CB12		 96
#define OP_CB123	 97
#define OP_CB0		 98
#define OP_CB03		 99
#define OP_CB02		100
#define OP_CB023	101
#define OP_CB01		102
#define OP_CB013	103
#define OP_CB012	104
#define OP_CBA		105

#define OP_RET		106
#define OP_RETL		107

#define OP_EQUATE	108
#define OP_LABEL	109
#define OP_MARKER	110
#define OP_COMMENT	111
#define OP_SYM_REF	112

#define OP_ORCC		113
#define OP_JMP		114

#ifdef CHIPFABS	/* only for fab#1 IU chip */
# define OP_FMOVS	115
# define OP_LDFA	116
# define OP_STFA	117
# define N_OPs		118
#else /*CHIPFABS*/
# define N_OPs		115
#endif /*CHIPFABS*/

extern struct opcode *(opcode_ptr[N_OPs]);

extern void init_opcode_pointers();
