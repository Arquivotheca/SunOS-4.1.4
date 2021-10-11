#ifndef lint
static char sccsid[] = "@(#)hk_load_hdl.c  1.1 94/10/31 SMI";
#endif

/*
 ***********************************************************************
 *
 * hk_load_hdl.c
 *
 * @(#)hk_load_hdl.c 13.1 91/07/22 18:30:08
 *
 * Copyright (c) 1991 by Sun Microsystems, Inc.
 *
 * This subroutine loads a relocatable Hawk Display List (.hdl) file
 * into VM, relocating it by the specified amount.
 *
 * 12-Mar-90 Kevin C. Rushforth	  Initial version created.
 * 25-Jun-90 Scott Johnston	  Modified to accept code from hlink
 * 24-Jul-90 Kevin C. Rushforth	  Check for major/minor version mismatch
 * 28-Apr-91 Kevin C. Rushforth	  Make versions 9.2-9.4 equivalent.
 *
 ***********************************************************************
 */

#include <stdio.h>
#include "hk_header.h"
#include "hasm_hdl.h"

/*
 *======================================================================
 *
 * Procedure Hierarchy:
 *
 *	hk_read_hdl_header
 *	hk_load_hdl_file
 *
 *======================================================================
 */

/* Define the following version number range as equivalent */
#define EQUIV_START	0x0902
#define EQUIV_END	0x0904

/* Set hk_version equal to FCS version */
#define HK_VERSION      0x0904  /* Current version */
short hk_version = HK_VERSION;  /* Global variable containing version */

/*
 *----------------------------------------------------------------------
 *
 * hk_read_hdl_header
 *
 *	Routine to read the header of the .hdl file and verify the
 *	no unresolved references exist.
 *
 * RETURN VALUE:
 *
 *	dl_size = size of display list from header.  A value of -1
 *	    indicates a bad file header
 *
 *----------------------------------------------------------------------
 */

int
hk_read_hdl_header(fp)
    FILE *fp;			/* Pointer to .hdl file */
{
    hk_dl_hd header;		/* The header */
    int dl_size;		/* Size (in bytes) of display list */
    int unresolved;		/* Flag indicating unresolved refs */

/* Get the hawk file header and verify that it is correct */
    if (fread(&header, sizeof(header), 1, fp) != 1) {
	perror("hk_load_hdl: Unable to read header");
	return (-1);
    }

    if (header.magic != HK_MAGIC_NUMBER) {
	fprintf(stderr,
	    "hk_load_hdl: Invalid magic number (0x%x) should be (0x%x)\n",
	    header.magic, HK_MAGIC_NUMBER);
	return (-1);
    }

    if (header.type != HK_HDL_TYPE) {
	fprintf(stderr,
	    "hk_load_hdl: Invalid type field (0x%x) should be (0x%x)\n",
	    header.type, HK_HDL_TYPE);
	return (-1);
    }

    if (header.version != hk_version) {
	/* Version mismatch -- check to see if the major number has changed */
	if ((header.version & 0xFF00) != (hk_version & 0xFF00)) {
	    fprintf(stderr,
		"hk_load_hdl: Error, version %.4X file with version %.4X library.\n",
		header.version, hk_version);
	    return (-1);
	}
	else {
	    /* Check to see whether versions are equivalent */
	    if ((header.version < EQUIV_START) || (header.version > EQUIV_END)
		|| (hk_version < EQUIV_START) || (hk_version > EQUIV_END))
		fprintf(stderr,
		    "hk_load_hdl: Warning, version %.4X file with version %.4X library.\n",
		    header.version, hk_version);
	}
    }

/* Read the .hdl header */
    if (fread(&dl_size, sizeof(dl_size), 1, fp) != 1) {
	perror("hk_load_hdl: Unable to read .hdl header");
	return (-1);
    }

    if (fread(&unresolved, sizeof(unresolved), 1, fp) != 1) {
	perror("hk_load_hdl: Unable to read .hdl header");
	return (-1);
    }

    if (unresolved) {
	fprintf(stderr,
	    "hk_load_hdl: cannot load file with unresolved references\n");
	return (-1);
    }

    return (dl_size);
} /* End of hk_read_hdl_header */



/*
 *----------------------------------------------------------------------
 *
 * hk_load_hdl_file
 *
 *	Routine to load the specified .hdl file into virtual memory,
 *	at the specified address, relocating it by the specified amount.
 *	The relocation offset is usually the same as the load address.
 *	The display list header must have been previously read in using
 *	hk_read_hdl_header().
 *
 * RETURN VALUE:
 *
 *	 0 = Success
 *	-1 = Failure
 *
 *----------------------------------------------------------------------
 */

int
hk_load_hdl_file(fp, dl_size, loadaddr, reloc)
    FILE *fp;			/* Pointer to .hdl file */
    int dl_size;		/* Size (in bytes) of display list */
    unsigned *loadaddr;		/* Starting address to load .hdl file into */
    unsigned *reloc;		/* Offset to use for relocation */
{
    register int offset;	/* Integer offset to use for relocation */
    int table_type;		/* Type of entry (symbol or patch) */
    int name_size;		/* Size (in bytes) of symbol name */
    int patch_addr;		/* Address to patch */
    char dummy[MAX_NAME_SIZE+1]; /* For reading in symbols */

    int reloc_table_size;	/* Size (in bytes) of relocation table */
    int symbol_table_size;	/* Size (in bytes) of symbol table */
    FILE *dmp = NULL;		/* Pointer to debug dump file */
    int i = 0;			/* Iterative counter */

#if 0
    dmp = fopen("hlink_loader.dmp", "w");               /* For debug out */
#endif

/* Get reloc_table_size and symbol_table_size vars */
    if (fread(&reloc_table_size, sizeof(reloc_table_size), 1, fp) != 1) {
	perror("hk_load_hdl_file: Unable to read reloc_table_size");
	return (-1);
    }

#if 0
    if (dmp) {
	fprintf(dmp, "HLINK_LOADER.DMP\n");
	fprintf(dmp, "\n");
	fprintf(dmp, "Input File:\n");
	fprintf(dmp, "Size (in bytes) of display list:\t%5d\n",
	     dl_size);
	fprintf(dmp, "Size (in bytes) of reloc table:\t\t%5d\n", reloc_table_size);
    }
#endif

    if (fread(&symbol_table_size, sizeof(symbol_table_size), 1, fp) != 1) {
	perror("hk_load_hdl_file: Unable to read symbol_table_size");
	return (-1);
    }

#if 0
    if (dmp) {
	fprintf(dmp, "Size (in bytes) of symbol table:\t%5d\n",
	    symbol_table_size);
	fprintf(dmp, "Relocation Table:\n"); 
    }
#endif

/* Load the file into VM */
    if (fread(loadaddr, 1, dl_size, fp) != dl_size) {
	perror("hk_load_hdl_file: Unable to read display list");
	return (-1);
    }

/* Read and process relocation information */
    offset = (int) reloc;
    for (i = 0; i < reloc_table_size; i+= sizeof(int))
    	if (fread(&patch_addr, sizeof(patch_addr), 1, fp) == 1) {
	    patch_addr >>= 2;
	    loadaddr[patch_addr] += offset;
#if 0
	    if (dmp) 
	    	fprintf(dmp, "%5x\n", patch_addr << 2);
#endif
	}
	else {
	    perror("hk_load_hdl_file:  Unable to read reloc_table");
	    return(-1);
     	}

#if 0
    if (dmp)
	fclose(dmp);
#endif

    /* ALL DONE */
    return (0);
} /* End of hk_load_hdl_file */

/* End of hk_load_hdl.c */
