/*	@(#)cost.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifndef __cost__
#define __cost__

/*
 * operand costs are weighted by depth of loop nesting.
 * The definitions in this file support arithmetic operations
 * on weighted costs, with provision for overflow detection.
 * Overflows are handled by saturation, i.e., returning a
 * large number representing "Infinity".
 *
 * NOTE: If (a) 1988 is in the distant past, (b) someone
 * is still maintaining iropt, and (c) you are it, you might
 * improve this interface by rewriting it in C++.
 */

typedef long int COST;	/* costs must be signed */

extern COST add_costs(/*cost1, cost2*/);
extern COST mul_costs(/*cost1, cost2*/);

#endif /* __cost__ */
