static char sccs_id[] = "@(#)scan_symbols.c	1.1\t10/31/94";

#include <stdio.h>
#include "structs.h"
#include "sym.h"
#include "bitmacros.h"
#include "errors.h"
#include "globals.h"

#ifdef DEBUG
Bool	did_scan_symbols_after_input = FALSE;
#endif


/* scan_symbols_after_input() makes a pass through the symbol table after
 * input is complete (and all symbols definitions should have been seen once),
 * noting any anomalies found.
 */
void
scan_symbols_after_input()
{
	register struct symbol *symp;

	for ( symp = sym_first();   symp != NULL;    symp = sym_next() )
	{
		if ( ! BIT_IS_ON(symp->s_attr, SA_DEFN_SEEN) )
		{
			/* definition for symbol was not seen in this module */

			if ( BIT_IS_ON(symp->s_attr, SA_LOCAL) )
			{
				/* local symbol was referenced but not defined*/
				error(ERR_LOC_NDEF, NULL, 0,*(symp->s_symname));
			}
			else if ( make_all_undef_global )
			{
				/* If the "make_all_undef_global" option is on,
				 * make Global (externally-defined) any symbol
				 * which is undefined (i.e. not defined in this
				 * module), and is not a "local" symbol.
				 */
				SET_BIT(symp->s_attr, SA_GLOBAL);
			}
			else if ( BIT_IS_OFF(symp->s_attr, SA_GLOBAL) )
			{
				/* Non-"local" symbol referenced, but not
				 * defined in this module nor declared Global.
				 * Complain about a reference to a never-
				 * defined symbol.
				 */
				error(ERR_SYM_NDEF, NULL, 0, symp->s_symname);
			}
		}
	}

#ifdef DEBUG
	did_scan_symbols_after_input = TRUE;
#endif
}


/* scan_symbols_after_assy() makes a pass through the symbol table after
 * assembly has been done (and all symbols should be completely defined),
 * noting any anomalies found.
 */
void
scan_symbols_after_assy()
{
#ifdef DEBUG
	register struct symbol *symp;

	for ( symp = sym_first();   symp != NULL;    symp = sym_next() )
	{
		if ( ((symp->s_attr & (SA_DEFN_SEEN|SA_DEFINED)) ==SA_DEFN_SEEN)
					||
		     ((symp->s_attr & (SA_DEFN_SEEN|SA_DEFINED)) ==SA_DEFINED)
		   )
		{
			internal_error(
				"scan_symbols_after_assy(): DEFNs out of sync for \"%s\"",
				symp->s_symname);
		}
	}
#endif
}
