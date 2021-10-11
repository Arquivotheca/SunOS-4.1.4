static char sccs_id[] = "@(#)set_instruction.c	1.1\t10/31/94";

#include <stdio.h>	/* for "NULL" */
#include "structs.h"
#include "set_instruction.h"

SET_INSTR_VARIATION
minimal_set_instruction(symp, offset)
	register struct symbol *symp;
	register long int offset;
{
	/* From looking at a SET pseudo-instruction's operand, this routine
	 * decides on the minimum # (and type) of instructions necessary to
	 * generate.
	 */

	if ( (symp == NULL) && (offset >= -0x1000) && (offset <= 0x0fff) )
				         /*-4096*/              /*4095*/
	{
		/* The source operand is an absolute value,
		 * within the signed 13-bit Immediate range,
		 * so can use a single instruction:
		 *	MOV const13,regno1
		 * (actually "or  %0,const13,regno1")
		 */
		return SET_JUST_MOV;
	}
	else if ( (symp == NULL) && ((offset & MASK(10)) == 0) )
	{
		/* The source operand is an absolute value,
		 * and the 10 low-order bits are zero,
		 * so can use a single instruction:
		 *      SETHI  %hi(VALUE),regno1
		 */
		return SET_JUST_SETHI;
	}
	else
	{
		/* Must expand "set" into 2 instructions; a SETHI plus an OR. */
		return SET_SETHI_AND_OR;
	}
}
