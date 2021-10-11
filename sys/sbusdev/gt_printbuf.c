#ifndef lint
static char sccsid[] = "@(#)gt_printbuf.c  1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 *  GT graphics accelerator print buffer definition
 */

#include "gtmcb.h"

/*
 * The GT printbuffer is set to HKKVS_PRINT_STRING_SIZE unless the
 * symbol GT_DIAG is defined.  In that case the printbuffer will
 * set to 132 Kbytes.
 */
#ifndef GT_DIAG
#define GT_PRINT_BUF_SIZE	((HKKVS_PRINT_STRING_SIZE) / sizeof (int))
#else GT_DIAG
#define	GT_PRINT_BUF_SIZE	(0x21000 / sizeof (int))
#endif GT_DIAG

unsigned int	gt_printbuffer[GT_PRINT_BUF_SIZE];
unsigned int	gt_printbuffer_size = sizeof(gt_printbuffer);
