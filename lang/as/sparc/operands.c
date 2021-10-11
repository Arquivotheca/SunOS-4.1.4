static char sccs_id[] = "@(#)operands.c	1.1\t10/31/94";
#include <stdio.h>
#include "structs.h"
#include "errors.h"
#include "globals.h"
#include "alloc.h"

/**********************  n e w _ o p e r a n d s  ****************************/

struct operands *
new_operands()
{
	register struct operands *operp;
	static struct operands op;

	operp = &op;

	/** if (assembly_mode == ASSY_MODE_BUILD_LIST)
	/** {
	/**	operp = (struct operands *)as_malloc(sizeof(struct operands));
	/** }
	/** else
	/** {
	/** 	static struct operands op;
	/** 	operp = &op;
	/** }
	 **/

	bzero(operp, sizeof(struct operands));

	return operp;
}


/*********************  f r e e _ o p e r a n d s  ***************************/

void
free_operands(operp)
	register struct operands *operp;
{
	/** if (assembly_mode == ASSY_MODE_BUILD_LIST)	as_free(operp); **/
}
