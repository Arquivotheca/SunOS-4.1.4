/*	@(#)globals.h	1.1	10/31/94	*/
	/* structs.h must be included BEFORE this file */

extern	char	**cpp_argv;		/* vector of ptrs to args for cpp.*/
extern	int	cpp_argc;		/* count of entries in "cpp_argv" */

extern	LINENO input_line_no;		/* current input line #		*/
extern char  *input_filename;		/* current input file name; global
					 *	copy of "curr_flp->fl_fnamep".
					 */

extern	char	*pgm_name;		/* name by which program was invoked*/

extern	Bool	debug;			/* TRUE if overall debugging turned on*/

extern	Options	final_options;		/* options in effect after last
					 * input file has been processed
					 */

extern	Bool	do_disassembly;		/* TRUE = perform disassembly	      */

extern	Bool	make_data_readonly;	/* TRUE=put Data in a readonly segment*/

extern	Bool	picflag;		/* TRUE = generate pic */

extern	int	optimization_level;	/* Optimization level (0 by default)  */

extern	char	*objfilename;		/* the output object-file name;
					 * "a.out" by default
					 */

#ifdef CHIPFABS
   extern int	iu_fabno ;	/* the SPARC  IU chip fab# to assemble for */
   extern int	fpu_fabno;	/* the SPARC  FPU chip fab# to assemble for */
#  define USE_BRUTE_FORCE_FAB_WORKAROUNDS (assembly_mode == ASSY_MODE_QUICK)
#endif /*CHIPFABS*/

typedef enum
{
	PASS_NONE, PASS_INPUT, PASS_POST_INPUT,
	PASS_OPTIMIZE, PASS_ASSEMBLE, PASS_FINISH
} AsmPass;
extern AsmPass	pass;		/* the current pass of the assembler	*/

extern Bool make_all_undef_global;/* option to make all undef'd symbols global*/

/* The mode of assembly: BUILD_LIST means to build the entire node list, and do
 * all the error-checking we can on it.  QUICK means to speed up the assembly by
 * immediately emitting code for each instruction, and then free()'ing the
 * node.  This reduces memory requirements considerably.  Side effects are that
 * without the node list:
 *	-- we can't do any contextual error checking; therefore the QUICK mode
 *	   should NOT be used on any code except untouched code from the
 *	   compiler.
 *	-- we can't do optimization; therefore the "-O" command-line option is
 *	   incompatible with option(s) which select the QUICK assembly mode.
 */
typedef enum { ASSY_MODE_UNKNOWN, ASSY_MODE_BUILD_LIST, ASSY_MODE_QUICK }
	AssyMode;
extern AssyMode assembly_mode;

/* Track label references by the nodes iff we are building a node list. */
#define TRACKING_LABEL_REFERENCES  (assembly_mode == ASSY_MODE_BUILD_LIST)
