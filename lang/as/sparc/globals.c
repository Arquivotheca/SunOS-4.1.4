static char sccs_id[] = "@(#)globals.c	1.1\t10/31/94";
#include <stdio.h>	/* for NULL */
#include "structs.h"
#include "globals.h"

char	**cpp_argv;		/* vector of ptrs to args for cpp.	*/
int	cpp_argc = 0;		/* count of entries in "cpp_argv".	*/

LINENO input_line_no;		/* current input line #		*/
char  *input_filename;		/* current input file name; global
				 *	copy of "curr_flp->fl_fnamep".
				 */

char	*pgm_name;		/* name by which program was invoked*/

Bool	debug = FALSE;		/* TRUE if overall debugging turned on*/

Options	final_options;		/* options in effect after last
				 * input file has been processed
				 */

Bool	do_disassembly = FALSE;	/* TRUE = perform disassembly		*/

Bool	make_data_readonly = FALSE;/* TRUE=put Data in a readonly segment*/

Bool	picflag = FALSE;	/* TRUE = generate pic */

int	optimization_level = 0;	/* Optimization level (0 by default)	*/

char	*objfilename="a.out";	/* the output object-file name;
				 * "a.out" by default
				 */

#ifdef CHIPFABS
   int	iu_fabno =IU_FABNO+0;	/* the SPARC  IU chip fab# to assemble for */
   int	fpu_fabno=FPU_FABNO+0;	/* the SPARC FPU chip fab# to assemble for */
#endif /*CHIPFABS*/

AsmPass	pass = PASS_NONE;	/* the current pass of the assembler	*/
				/*	(see globals.h for values)	*/

Bool make_all_undef_global = TRUE;/* option to make all undef'd symbols global*/

/* ASSY_MODE_BUILD_LIST if to build list of nodes during input;
 * ASSY_MODE_QUICK if to emit each node immediately, without putting it
 * on a list.
 */
AssyMode assembly_mode = ASSY_MODE_UNKNOWN;
