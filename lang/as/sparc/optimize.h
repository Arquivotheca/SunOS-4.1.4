/*	@(#)optimize.h	1.1	10/31/94	*/

/************* THIS SHOULD BE CHANGED TO MORE MEANINGFUL NAMES ***************/

/* Indexes into do_opt[] array... */
/* When changing these, make corresponding changes in
 * disassemble.c:print_optimization_name() .
 */

/* Note that the Quntus Prolog compiler currently (July 1987) depends on
 * being able to run the assembler/optimizer with the -O~Q~R options.
 */
			/* OPT_NONE is here to aid debugging */
#define OPT_NONE	 (OptimizationNumber)('A'-'A')

#define OPT_DEADCODE	 (OptimizationNumber)('B'-'A')
#define OPT_DEADLOOP	 (OptimizationNumber)('C'-'A')
#define OPT_BR2BR	 (OptimizationNumber)('D'-'A')
#define OPT_TRIVBR	 (OptimizationNumber)('E'-'A')
#define OPT_PROLOGUE	 (OptimizationNumber)('F'-'A')
#define OPT_MOVEBRT	 (OptimizationNumber)('G'-'A')
#define OPT_NOPS	 (OptimizationNumber)('H'-'A')
#define OPT_BRARBR1	 (OptimizationNumber)('I'-'A')
#define OPT_BRARBR2	 (OptimizationNumber)('J'-'A')
#define OPT_BRARBR3	 (OptimizationNumber)('K'-'A')
#define OPT_BRSOONER	 (OptimizationNumber)('L'-'A')
#define OPT_LOOPINV	 (OptimizationNumber)('M'-'A')
#define OPT_EPILOGUE	 (OptimizationNumber)('N'-'A')
#define OPT_LEAF	 (OptimizationNumber)('O'-'A')
#define OPT_SIMP_ANNUL	 (OptimizationNumber)('P'-'A')
#define OPT_DEADARITH	 (OptimizationNumber)('Q'-'A')	/* Quintus needs this */
#define OPT_CONSTPROP	 (OptimizationNumber)('R'-'A')	/* Quintus needs this */
#define OPT_SCHEDULING	 (OptimizationNumber)('S'-'A')
#define OPT_COALESCE_RS1 (OptimizationNumber)('T'-'A')
#define OPT_COALESCE_RS2 (OptimizationNumber)('U'-'A')
	/* OPT_N_OPTTYPES = #of opt types, i.e. size of array needed */
#define OPT_N_OPTTYPES	(OptimizationNumber)('Z'-'A'+1)
extern Bool do_opt[OPT_N_OPTTYPES];	/* each optimization =TRUE if enabled */

extern Bool optimizer_debug;	/* TRUE if optimizer debug output is on */

extern Bool	optmization_options();
extern void	optimize_segment();

#ifdef DEBUG
extern void	opt_dump_nodes();
extern Bool	EMPTY_nodes_were_deleted;
#endif /*DEBUG*/
