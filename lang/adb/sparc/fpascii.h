/*      @(#)fpascii.h 1.1 94/10/31 SMI      */

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * fp_reg_address, the address of first word of register image,
 * is now kept in the adb_raddr structure (see sun.h/sparc.h etc).
 */

	/* For sparc, we no-op most of the definitions. */
#define  fpa_disasm 0
#define  fpa_avail  0
#define  mc68881    0

	/* We still need fprtos, doubletos */
void fprtos(/* fpr, s */);		/* see expansion below */
void doubletos (/* d, s */);


/*
 * Description of sparc 128-bit extended floating point
 * (Only 80 of the bits are used right now.)
 * This should be in /usr/include/machine/reg.h
 */
typedef struct {
	unsigned s:1;			/* "Extended-e" */
	unsigned exp:15;
	unsigned unused_reserved: 16;

	unsigned j:1;			/* "Extended-f" */
	unsigned f_msb:31;

	unsigned f_lsb:32;		/* "Extended-f-low" */

	unsigned reserved_unused: 32;	/* "Extended-u" */
} ext_fp;
