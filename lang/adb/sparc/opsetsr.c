#ifndef lint
static	char sccsid[] = "@(#)opsetsr.c 1.1 94/10/31 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * This file (opsetsr.c) contains the sparc version of the interface
 * routine "printins" (print instruction -- i.e., the disassembler),
 * which allows the sparc instruction disassembler to fit into adb.
 */

#include "adb.h"
#include "symtab.h"

int	dotinc;
static int	space;

#ifdef	talkative
static char pcrel[] = "[PC-relative]";
static char normallong[] = ":L";
#else	!talkative
static char pcrel[] = "";
static char normallong[] = "";
#endif	!talkative


/*
 * Formerly, the first parameter of printins was here only to keep
 * compatible with the fact that the vax version of printins was used
 * both by adb and sdb.  Now, it is used only in the MC680x0 versions,
 * to tell whether to print the timings or not (format=='i' or 'z').
 */
printins(f, idsp, inst)
{
	char *dres,  *disassemble();

	/*
	 * It is necessary for "disassemble" to know the program counter
	 * for the instruction, so that absolute addresses can be computed
	 * where PC relative displacements exist.  As far as disassemble
	 * is concerned, "dot" is the pc.
	 */
	dres = disassemble( inst, dot );

	printf( "%s", dres );	/* Let format.c cram the newline at the end */
	dotinc = 4;		/* always */
}

long
instfetch(size)
	int size;
{
	long insword;

	insword = chkget(inkdot(dotinc), space);
	dotinc += 4;		/* all instrs 4 bytes long on sr */
	return (insword);
}


char suffix(size)
	register int size;
{

	return((size==1) ? 'b' : (size==2) ? 'w' : (size==4) ? 'l' : '?');
}
