/*	@(#)ir_c.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "ir_misc.h"
# if TARGET == I386
#include "ops.h"
#include "node.h"
#include "symtab.h"
#include "sw.h"
# else
#include "pcc_ops.h"
#include "pcc_node.h"
#include "pcc_symtab.h"
#include "pcc_sw.h"
# endif

typedef enum {
    FOR_VALUE,		/* compile expression for value */
    FOR_ADDRESS,	/* compile address of memory operand */
    FOR_EFFECT,		/* compile expression for effect only */
    FOR_ASSIGN,		/* special cookie for assigning to an IR leaf node */
} COOKIE;

