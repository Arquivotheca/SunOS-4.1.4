static char sccs_id[] = "@(#)opt_unique_label.c	1.1\t10/31/94";
/*
 * This file contains code associated with creation of unique label names for
 * the peephole optimizer.
 */

#include <stdio.h>	/* for NULL */
#include "structs.h"
#include "actions.h"
#include "opcode_ptrs.h"
#include "opt_utilities.h"

/* Note: Peephole-optimizer-unique labels traditionally begin with "LY".
 * Also note that if "LY" is changed, be sure to make corresponding changes in
 * label_name[], is_unique_optimizer_label(), and unique_optimizer_label().
 */
static char label_name[10] = "LY0000000";
static unsigned int label_count = 1;

Bool
is_unique_optimizer_label(name)
	char *name;
{
	register char *cp;

	/* To look like a unique optimizer label, it has to start with "LY",
	 * and the remaining characters must all be decimal digits.
	 */

	if (strncmp(name, "LY", 2) != 0)	return FALSE;

	for (cp = name+2;   (*cp != '\0');   cp++)
	{
		if ( ! ((*cp >= '0') && (*cp <= '9')) )	return FALSE;
	}

	return TRUE;
}

static char *
unique_optimizer_label()
{
	sprintf(&label_name[2], "%u", label_count);
	label_count++;

	return &label_name[0];
}


Node *
unique_optimizer_label_node(lineno, optno)
	register LINENO lineno;
	register int	optno;
{
	register char *label_name;
	register Node *np;

	label_name = unique_optimizer_label();
	np = create_label( label_name,
			   opt_new_node(lineno, opcode_ptr[OP_LABEL], optno) );
	
	if (np == NULL)
	{
		internal_error("unique_optimizer_label_node(): can't create new label \"%s\"", label_name);
		/*NOTREACHED*/
	}
	else	np->n_internal = TRUE;

	return np;
}
