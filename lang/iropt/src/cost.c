#ifndef lint
static	char sccsid[] = "@(#)cost.c 1.1 94/10/31 Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include "iropt.h"
#include "cost.h"

#define  INFINITY (0x7fffffff)

/*
 * return the sum of two costs, or INFINITY if
 * an overflow occurs
 */
COST
add_costs(cost1, cost2)
    COST cost1;
    COST cost2;
{
    COST result;
    COST sgn1;
#define  SGN(x) ((x)>>31)

    result = cost1 + cost2;
    sgn1   = SGN(cost1);
    if (sgn1 == SGN(cost2) && sgn1 != SGN(result)) {
	/* overflow occurred */
	return (COST)INFINITY;
    }
    return result;
#undef SGN
}

/*
 * return the product of two costs, or INFINITY if
 * an overflow occurs
 */
COST
mul_costs(cost1, cost2)
    COST cost1;
    COST cost2;
{
    COST result;
    int abs1;

#define ABS(x) ( ((x)<0) ? -(x) : (x) )
    result = (cost1 * cost2);
    abs1 = ABS(cost1);
    if (ABS(result) < abs1 && cost1 != 0 && cost2 != 0) {
	/* overflow occurred */
	return (COST)INFINITY;
    }
    return result;
#undef ABS
}
