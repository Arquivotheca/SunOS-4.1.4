#ifndef lint
static	char sccsid[] = "@(#)_Q_qtoi.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include "_Qquad.h"
#include "_Qglobals.h"

extern _Q_set_exception();

int
_Q_qtoi(x)
	QUAD x;
{
	unpacked	px;
	int		i;
	enum fp_direction_type saved_fp_direction = fp_direction;

	_fp_current_exceptions = 0;
	fp_direction = fp_tozero;
	_fp_unpack(&px,&x,fp_op_extended);
	_fp_pack(&px,&i,fp_op_integer);
	_Q_set_exception(_fp_current_exceptions);
	fp_direction = saved_fp_direction;
	return i;
}
