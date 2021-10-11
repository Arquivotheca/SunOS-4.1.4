#ifndef lint
static char sccsid[] = "@(#)hk_error.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * hk_error.c
 *
 * @(#)hk_error.c 13.1 91/07/22 18:29:40
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 * Take a runtime error code and return a pointer to a string
 * describing that error.
 *
 * 19-Apr-90 Kevin C. Rushforth	  Initial version created.
 * 30-Jul-90 Kevin C. Rushforth	  Include gtmcb.h
 * 08-Oct-90 Vic Tolomei	  Add LOTS of missing errors from gtmcb.h
 *				  and put them in order
 * 06-Dec-90 Kevin C. Rushforth	  Rearranged routines and global symbols.
 * 25-Jan-91 Josh Lee		  Added new errors.
 * 23-Apr-91 Kevin C. Rushforth	  Ditto.
 * 25-Jun-91 Kevin C. Rushforth	  Ditto.
 *
 ***********************************************************************
 */

#include <sbusdev/gtmcb.h>

/*
 * Global variables -- these really belong in a "util" or "globals"
 * file, but this was the most convenient place to put them.
 */

int hk_phigs_debugger = 0;		/* Flag for the PHIGS debugger */
int hk_fixed_mmap = 0;			/* Force the mmap to a fixed vaddr */



char *
hk_error_string(error)
    unsigned error;
{
    char *err_string;

    switch (error) {
    case HKERR_ILLEGAL_OP_CODE:
	err_string = "Illegal op-code";
	break;
    case HKERR_ILLEGAL_ATTRIBUTE:
	err_string = "Illegal attribute";
	break;
    case HKERR_LIGHT_OVERFLOW:
	err_string = "Light overflow";
	break;
    case HKERR_MCP_OVERFLOW:
	err_string = "MCP overflow";
	break;
    case HKERR_JMPL_IMMEDIATE:
	err_string = "JMPL immediate";
	break;
    case HKERR_JMPR_ABSOLUTE:
	err_string = "JMPR absolute";
	break;
    case HKERR_JPS_IMMEDIATE:
	err_string = "JPS immediate";
	break;
    case HKERR_ST_IMMEDIATE:
	err_string = "ST immediate";
	break;
    case HKERR_STU_IMMEDIATE:
	err_string = "STU immediate";
	break;
    case HKERR_STD_IMMEDIATE:
	err_string = "STD immediate";
	break;
    case HKERR_PARA_ORDER_OVER_6:
	err_string = "Parametric order over 6";
	break;
    case HKERR_KNOT_INDEX_OUTARANGE:
	err_string = "Knot index out of range";
	break;
    case HKERR_NURB_ORDER_OVER_10:
	err_string = "Nurb order over 10";
	break;
    case HKERR_NURB_DATA_ERROR:
	err_string = "Nurb data error";
	break;
    case HKERR_NO_MARKER_TABLE:
	err_string = "No marker table present";
	break;
    case HKERR_NO_FONT_TABLE:
	err_string = "No font table present";
	break;
    case HKERR_NO_NORMAL_IN_TRIANGLE:
	err_string = "No normal in triangle";
	break;
    case HKERR_TWO_NORMALS_OR_COLORS:
	err_string = "Two normals or colors";
	break;
    case HKERR_SI32_IMMEDIATE:
	err_string = "Save_image_32 immediate";
	break;
    case HKERR_SI8_IMMEDIATE:
	err_string = "Save_image_8 immediate";
	break;
    case HKERR_SS_ACC_IMMEDIATE:
	err_string = "Stochastic instruction in immediate mode";
	break;
    case HKERR_STACK_UNDERFLOW:
	err_string = "Traversal stack underflowed";
	break;
    case HKERR_LOAD_CONTEXT_IMMEDIATE:
	err_string = "Load context immediate";
	break;
    case HKERR_LDSTU_IMMEDIATE:
	err_string = "LDSTU immediate";
	break;
    case HKERR_SWAP_IMMEDIATE:
	err_string = "SWAP immediate";
	break;
    case HKERR_MIA_BUS_TIME_OUT:
	err_string = "MIA bus timeout";
	break;
    case HKERR_MIA_BUS_ERR:
	err_string = "MIA bus error";
	break;
    case HKERR_MIA_SIZE:
	err_string = "MIA size error";
	break;
    case HKERR_LOCAL_BUS_TIME_OUT:
	err_string = "Local bus timeout";
	break;
    case HKERR_DIAG_ESC_SMALL:
	err_string = "DIAG_ESC i860 program too small";
	break;
    case HKERR_DIAG_ESC_MAGIC:
	err_string = "DIAG_ESC i860 program invalid magic number";
	break;
    case HKERR_DIAG_ESC_LARGE:
	err_string = "DIAG_ESC i860 program too large";
	break;
    case HKERR_DIAG_ESC_CHECKSUM:
	err_string = "DIAG_ESC i860 program illegal checksum";
	break;
    case HKERR_SU_SYNC:
	err_string = "SU sync error";
	break;
    case HKERR_EW_SYNC:
	err_string = "EW sync error";
	break;
    case HKERR_SI_SYNC:
	err_string = "SI sync error";
	break;
    case HKERR_BAD_MCB_MAGIC:
	err_string = "Bad magic number in MCB";
	break;
    case HKERR_BAD_MCB_VERSION:
	err_string = "Version number mismatch in MCB";
	break;
    case HKERR_BAD_CTX_MAGIC:
	err_string = "Bad magic number in CTX";
	break;
    case HKERR_BAD_CTX_VERSION:
	err_string = "Version number mismatch in CTX";
	break;
    case HKERR_ASSERTION_FAILED:
	err_string = "Front end code assertion check failed";
	break;
    case HKERR_NULL_POINTER:
        err_string = "Illegal NULL pointer";
	break;
    case HKERR_BAD_POINTER:
        err_string = "Invalid pointer";
	break;
    case HKERR_CLUT_INDEX:
        err_string = "CLUT index out of range";
	break;
    case HKERR_SCRATCH_BUFFER:
        err_string = "Illegal or NULL hk_scratch_buffer";
	break;
    case HKERR_SINGULAR_CMT:
        err_string = "Composite modeling transform is singular";
	break;
    case HKERR_BAD_GEOM_FORMAT:
        err_string = "Illegal hk_*_geom_format";
	break;
    case HKERR_BAD_NORMAL:
	err_string = "Illegal normal for MCP";
	break;
    case HKERR_WLUT_INDEX:
	err_string = "Illegal WLUT index";
	break;
    case HKERR_BAD_COUNT:
	err_string = "Illegal count field";
	break;
    case HKERR_RP_SEMAPHORE:
	err_string = "RP semaphore not seen";
	break;
    case HKERR_OTHER:
	err_string = "Unspecified error";
	break;
    default:
	err_string = "Unknown";
    }

    return (err_string);
} /* End of hk_error_string */

/* End of hk_error.c */
