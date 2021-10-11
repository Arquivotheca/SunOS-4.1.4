static char sccs_id[] = "@(#)errors.c	1.1\t10/31/94";

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include "structs.h"
#include "globals.h"

/*
 *  routines to write error messages and warnings.
 */

/*
 * "messages" contains both errors and warnings.
 *
 * NOTE:  the #define's in "error.h" MUST correspond with the order of the
 *	  messages in this array!
 */

static char *(messages[]) =
{
	/* 0*/	"",				/* <unused> */
	/* 1*/	"statement syntax",
	/* 2*/	"invalid character (0x%02x)",
	/* 3*/	"unknown opcode \"%s\"",
	/* 4*/	"invalid digit in radix %d",
	/* 5*/	"attempt to redefine reserved symbol",
	/* 6*/	"unterminated string",
	/* 7*/	"missing '('",
	/* 8*/	"missing ')'",
	/* 9*/	"redefinition of symbol \"%s\"",
	/*10*/	"expression must evaluate to a nonrelocatable (absolute) value",
	/*11*/	"alternate space index must be >0 and <255",
	/*12*/	"constant value must be between -4096 and 4095",
	/*13*/	"constant value must be between -0x00400000 and 0x003fffff",/*22 bits*/
	/*14*/	"addressing syntax",
	/*15*/	"division by zero",
	/*16*/	"invalid register",
	/*17*/	"%s in delay slot (follows CTI)",
	/*18*/	"\"%%lo\" invalid in SETHI instruction",
	/*19*/	"quoted-string operand required",
	/*20*/	"location counter not on %s boundary",
	/*21*/	"invalid number of operands",
	/*22*/	"invalid operand",
	/*23*/	"value does not fit in %d bits",
	/*24*/	"invalid alignment boundary",
	/*25*/	"displacement is not a WORD displacement",
	/*26*/	"word displacement will not fit in 22 bits",
	/*27*/	"cannot make local symbol global",
	/*28*/	"missing definition for local symbol \"%c\"",
	/*29*/	"symbol \"%s\" referenced but not defined",
	/*30*/	"first operand must be a register for comparison with non-zero constant",
	/*31*/	"no input filename given",
	/*32*/	"cannot open input file \"%s\"",
	/*33*/	"unknown \"%%\"-symbol",
	/*34*/	"unknown option \'%s\'",
	/*35*/	"invalid use of local symbol",
	/*36*/	"must assemble this file with -P option",
	/*37*/	"invalid use of %%hi or %%lo operator",
	/*38*/	"invalid operand of %%ad",
	/*39*/	"questionable use of relocatable value",
	/*40*/	"(perhaps the input has already been optimized?)",
	/*41*/	"cannot open output file \"%s\"",
	/*42*/	"unrecognized floating-point number \"0f%s\"",
	/*43*/	"symbol \"%s\" appears in \".globl\" after its definition",
	/*44*/	"invalid operand value \"%s\"",
	/*45*/	"%sfeature not yet supported",
	/*46*/	"Bfcc preceeded by floating-point instruction; NOP inserted",
	/*47*/	"must use \"clr\"",
	/*48*/	"even-numbered register required",
	/*49*/	"must use register 15 (%%o7)",
	/*50*/	"expression syntax",
	/*51*/	"last CTI in segment has empty delay slot",
	/*52*/	"invalid (misaligned) register",
	/*53*/	"RETT not preceeded by JMP",
	/*54*/	"out-register count %d is not between 0 and 6",
	/*55*/	"can't compute value of an expression involving an external symbol",
	/*56*/	"can't compute difference between symbols in different segments",
	/*57*/	"syntax no longer supported",
	/*58*/	"can't do arithmetic involving %%hi/%%lo of a relocatable symbol",
	/*59*/	"write error on output file \"%s\"",
	/*60*/	"\"%s\" option incompatible with \"%s\" option",
	/*61*/	"stack alignment problem; second operand is not a multiple of %d",
	/*62*/	"cannot use \"-O\" option with hand-written ass'y language code",
	/*63*/	"\"jmpl\" 3rd operand invalid when destination register is %%g0",

#ifdef SUN3OBJ
	/*64*/	"segment \"%s\" not among those currently supported",
#else /*SUN3OBJ*/
        /*64*/  "",
#endif /*SUN3OBJ*/

#ifdef CHIPFABS
	/*65*/	"constant value must be between -0x00800000 and 0x007fffff",/*23 bits*/
#else /*CHIPFABS*/
	/*65*/	"",
#endif /*CHIPFABS*/
	/*66*/  "3 instructions required between ldfsr and next FBfcc; nops inserted",
	/*67*/  "illegal \"boundary\" register pair",
	/*68*/  "optimizing code with stab pseudo-instr; stabs will be deleted",
	/*69*/  "constant value must be between 0 and 511",
	/*70*/  "-R conflicts with .stab directive, -R turned off",
	NULL
};

int error_count;		/* Number of errors detected */


errors_init()
{
	error_count = 0;

#ifdef DEBUG
	/* Check the ASSUMPTION that pointers and ints are the same size.
	 * Sometimes pointers are passed to the error routines, to go along
	 * with "%s" formats, but the routines expect, and pass on to
	 * fprintf(), an "int" arg.  If pointers and ints are not the same
	 * size, this kludge will fail.
	 */
	if ( sizeof(int) != sizeof(char *) )	internal_error("errors_init(): sizeof(ptr) != sizeof(int); \"intarg\" argument to error routines will fail");
#endif
}

static char *
filename_for_messages(sp)
	register char *sp;
{
	/* Make the input filename more legible, for diagnostic messages.
	 * (The "real" name of "-" is still referred to by "...->fl_fnamep").
	 */

	if ( strcmp(sp, "-") == 0 )	return "<stdin>";
	else	return sp;
}


static void
print_err_msg(type, code, filename, lineno, intarg1, intarg2)
	char   *type;
	int	code;
	char   *filename;	/* input file error occurred in	  */
	LINENO	lineno;		/* input line # error occurred on */
	int	intarg1;
	int	intarg2;
{
	fputs(pgm_name, stderr);
	fputs(": ",     stderr);
	if (filename != NULL)
	{
		putc('"',       stderr);
		fputs(filename_for_messages(filename),  stderr);
		putc('"',       stderr);
		if (lineno > 0)	fprintf(stderr,", line %d", lineno);
		fputs(": ",     stderr);
	}
	fputs(type, stderr);
	fputs(": ", stderr);
	fprintf(stderr, messages[code], intarg1, intarg2);
	putc('\n', stderr);
}


/*
 *	"internal_error" is called when the assembler detects either its own
 *	internal error, or an error occured during a system call.
 *	This routine does not return!
*/
/*VARARGS1*/
internal_error(msgfmt, intarg1, intarg2)
	char *msgfmt;
	int   intarg1, intarg2;
{
	fprintf(stderr,"%s: ", pgm_name);
	if (input_filename != NULL)
	{
		fprintf(stderr,"\"%s\": ",
			filename_for_messages(input_filename));
		if (input_line_no > 0)
		{
			fprintf(stderr,", approx line %d: ", input_line_no);
		}
	}
	fprintf(stderr,"internal assembler error: ");
	fprintf(stderr, msgfmt, intarg1, intarg2);
	putc('\n', stderr);
	fflush(stderr);
	exit(1);
}

/*
 *	"error" is called whenever the assembler recognizes an error in the
 *	user's input.
 */

/*VARARGS3*/
error(code, filename, lineno, intarg1, intarg2)
	int	code;
	char   *filename;	/* input file error occurred in	  */
	LINENO	lineno;		/* input line # error occurred on */
	int	intarg1;
	int	intarg2;
{
	error_count++;		/* increment error count */

	print_err_msg("error", code, filename, lineno, intarg1, intarg2);
}

/*	"warning" issues a warning regarding the user's input.
 *	The only differences between a warning and an error are:
 *		[1] "warning" vs. "error" on the message
 *		[2] if there are warnings but no errors, an object file
 *			will still be generated by the assembler.
 */

/*VARARGS3*/
warning(code, filename, lineno, intarg1, intarg2)
	int	code;
	char   *filename;	/* input file error occurred in	  */
	LINENO	lineno;		/* input line # error occurred on */
	int	intarg1;
	int	intarg2;
{
	print_err_msg("warning", code, filename, lineno, intarg1, intarg2);
}
