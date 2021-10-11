#ifndef _HASM_HDL_
#define _HASM_HDL_

/*
 ****************************************************************
 *
 * hasm_hdl.h
 *
 *      @(#)hasm_hdl.h  1.1 94/10/31  SMI
 *
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 *
 *	Hawk display list (hdl) definitions.
 *
 * This header file contains the definition of the .hdl file
 * used by hasm, hlink, and the hdl_loader routine.
 *
 * 12-Mar-90 Kevin C. Rushforth	  Initial version created.
 * 14-Mar-90 Kevin C. Rushforth	  Removed byte vs. word flag.
 *				  Added maximum name size
 * 20-Jun-90 Scott Johnston	  Added relocation structures
 *
 ****************************************************************
 */

#define HASH_SIZE	1024		/* Hash table size (a power of 2) */
#define HASH_PRIME	17		/* An arbitrary prime number */

#define MAX_NAME_SIZE	64		/* Maximum name size */

/* Flags for linker input */
#define TYPE_PATCH 	0		/* Patch entry */
#define TYPE_SYMBOL 	1		/* Symbol entry */

/* Global types */

typedef struct symbol symbol;
typedef struct patch patch;
typedef struct rval rval;
typedef struct reloc reloc;

struct patch {
    patch *next;			/* Pointer to next location to patch */
    int addr;				/* Address to patch */
    int relative;			/* Flag indicating relative patch */
};

struct symbol {
    symbol *next;			/* Pointer to next label */
    char *name;				/* Label name */
    int val;				/* Value of label */
    short defined;			/* Flag indicating defined label */
    short global;			/* Global flag */
    patch *p;				/* Pointer to patched data */
};

struct rval {
    int val;				/* Value returned */
    symbol *sptr;			/* Pointer to label (if any) */
};

struct reloc {
    int addr;				/* Address to be relocated */
    reloc *next;			/* Pointer to next reloc ite */
};


#endif _HASM_HDL_

/* End of hasm_hdl.h */
