#ifndef lint
static	char sccsid[] = "@(#)printsr.c 1.1 94/10/31 SMI";
#endif

/*
 * adb - sparc-specific print routines
 *	printregs
 *	printstack
 *		get_nrparms
 *		printargs
 *		printlocals
 *	fp_print
 */

#include "adb.h"
#include "symtab.h"

printregs()
{
	register int i;
	int val, off;

	/* Sparc has too many registers.  Print two columns. */
	for (i = 0; i <= LAST_NREG; ++i ) {
#ifndef KADB
		/* adb can't get even get at these registers */
		if( i == REG_WIM || i == REG_TBR )
			continue;
#endif KADB
		val = readreg(i);
		printf("%s%6t%R %16t", regnames[i], val);

		/*
		 * Don't print symbols for regs that can't be addresses.
		 */
		if( i != REG_PSR && i != REG_WIM && i != REG_TBR ) {
			off = findsym(val, DSP);
			if (off < maxoff) {
				printf("%s", cursym->s_name);
				if (off) {
					printf("+%R", off);
				}
			}
		}
		if( i < REG_L0 ) {
		  int cp;
			/* go to column 40 */
			if( (cp = charpos() ) < 40 )
				printf( "%M", 40 - cp );
			else
				printf( "%8t" );

			i += REG_L0 - REG_G0 -1;
		} else if( i < REG_I7 ) {
			i -= REG_L0 - REG_G0 ;
			printf( "\n" );
		} else {
			printf( "\n" );
		}

	}
	printpc();
}

	        
/*
 * printstack -- implements "$c" and "$C" commands:  print out the
 * stack frames.  printstack was moved from print.c due to heavy
 * machine-dependence.
 *
 * Since there are no hints in the code on a sparc as to the number
 * of parameters that were really passed,  printstack on a sparc
 * allows the user to specify a (hex) number following the c in a
 * $c command.  That number will tell how many parameters $c displays
 * for each routine (e.g., $c22).  We allow only two hex digits there.
 */
printstack(modif)
	int modif;
{
	int i, val;
	char *name;
	struct stackpos pos;
	struct asym *savesym;

	int def_nargs = -1;
	
	/*
	 * Read the nr-of-parameters kluge command, if present.
	 * Up to two hex digits are allowed.
	 */
	if( nextchar( ) ) {
		def_nargs= get_nrparms( );
	}

	stacktop(&pos);
	if (hadaddress) {
		pos.k_fp = address;
		pos.k_nargs = 0;
		pos.k_pc = MAXINT;
		pos.k_entry = MAXINT;
		/* sorry, we cannot find our registers without knowing our pc */
		for (i = FIRST_STK_REG; i <= LAST_STK_REG; i++)
			pos.k_regloc[ REG_RN(i) ] = 0;
		findentry(&pos, 0);

	}
	while (count) {		/* loop once per stack frame */
		count--;
		chkerr();
#ifdef vax
		/* HACK */
		if (INUDOT(pos.k_pc))
			name = "sigtramp";
#endif
#ifdef sun
		/* EQUAL HACK */
		if (pos.k_pc == MAXINT) {
			name = "?";
			pos.k_pc = 0;
		}
#endif
		else {
			val =  findsym(pos.k_pc, ISYM);
			if (cursym && !strcmp(cursym->s_name, "start"))
				break;
			if (cursym)
				name = cursym->s_name;
			else
				name = "?";
		}
		printf("%s", name);
#ifdef sun
		/* deal with complications introduced by PIC and frameless
		** routines. For pic,  we want to show the presence of the jump 
		** vector, for frameless routines we don't know where to find
		** our ret addr but we want to display 2 lines, eg one for
		** write and one for the caller of write(). k_entry is the location
		** called from the caller of the current frame.
		*/
		if (pos.k_entry != MAXINT) {
			int val2, jmptarget;
			struct asym *savesym = cursym;

			val2 = findsym(pos.k_entry, ISYM);
			if (cursym != NULL && cursym != savesym && 
			    (savesym == NULL ||
			      cursym->s_value != savesym->s_value)) {
				/* caller's callee is not us. If caller's callee
				** is a jmp (ie caller is pic) , test the jmp target
				*/
				if(jmptarget = ispltentry(pos.k_entry)) {
						struct asym *savesym2 = cursym;

						findsym(jmptarget, ISYM);
						if ( cursym != NULL && cursym != savesym &&
								(savesym == NULL || 
								cursym->s_value != savesym->s_value)
						) { /* caller's callee is not us nor is it a
						   ** jmp that points to us - assume we're in
						   ** a frameless routine.
						   */
							printf("(?)\n");
							if (cursym)
								name = cursym->s_name;
							else
								name = "?";
							printf("%s", name);
					}
					/* indicate that caller called callee via a jmp */
					printf("(via ");
					if (savesym2)
						name = savesym2->s_name;
					else
						name = "?";
					printf("%s", name);
					if(val2 && val2 != MAXINT) {
						printf("+%X", val2);
					}
					printf(")");
				} else {
					/* caller's callee not us and not a jmp : assume we're
					** in a frameless routine
					*/
					printf("(?)\n");
					if (cursym)
						name = cursym->s_name;
					else
						name = "?";
					printf("%s", name);
				}
			}

#if DEBUG
		    { extern int adb_debug;

			if( adb_debug ) {

			    printf( "In printstack, savesym ", 0, 0, 0 );
			    if( savesym == 0 ) printf( "is NULL", 0, 0, 0 );
			    else printf( "%X <%s>", savesym->s_value,
						    savesym->s_name );
			    printf( ", cursym ", 0, 0, 0 );
			    if( cursym == 0 ) printf( "is NULL\n", 0, 0, 0 );
			    else printf( "%X <%s>\n", cursym->s_value,
						      cursym->s_name );

			}
		    }
#endif DEBUG

		} /* end if k_entry indicates a problem */
#endif

		/*
		 * Print out this procedure's arguments
		 */
		printargs( def_nargs, val, &pos );

		if (modif == 'C' && cursym) {
			printlocals( &pos );
		}

		if (nextframe(&pos) == 0 || errflg)
			break;
#ifndef sun
		/*
		 * due to our different interpretation of the stack frame,
		 * this fails to work
		 */
		if (hadaddress == 0 && !INSTACK(pos.k_fp))
			break;
#endif
	} /* end while (looping once per stack frame) */

} /* end printstack */


/*
 * Get the (optional) number following a "$c", which tells
 * how many parameters adb should pretend were passed into
 * each routine in the stack trace.  Zero, one, or two hex
 * digits is/are allowed.
 */
static int
get_nrparms ( ) {
  int na = 0;

	if( isxdigit(lastc) ) {
		na = convdig(lastc);
		if( nextchar() ) {
			if( isxdigit(lastc) )
				na = na * 16 + convdig(lastc);
		}
	}
	return na;
}


/*
 * Print out procedure arguments
 *
 * There are two places to look -- where the kernel has stored the
 * current value of registers i0-i5, or where the callee may have
 * dumped those register arguments in order to take their addresses
 * or whatever.  Since optimized routines never do that, we'll opt
 * for the i-regs.
 */
 static
printargs ( def_nargs, val, ppos ) struct stackpos *ppos; {
  addr_t regp, callers_fp, ccall_fp;
  int anr, nargs;

	printf("(");

	/* Stack frame loc of where reg i0 is stored */
	regp = ppos->k_fp + FR_I0 ;
	callers_fp = get( ppos->k_fp + FR_SAVFP, DSP );
	if( callers_fp )
		ccall_fp = get( callers_fp + FR_SAVFP, DSP );
	else
		ccall_fp = 0;
	db_printf( "printargs:  caller's fp %X; ccall_fp %X\n",
		callers_fp, ccall_fp );

	if( def_nargs < 0  ||  ppos->k_flags & K_CALLTRAMP
		|| callers_fp == 0 ) {
		nargs = ppos->k_nargs;
	} else {
		nargs = def_nargs;
	}
	
	if ( nargs ) {

		for( anr=0; anr < nargs; ++anr ) {
			if( anr == NARG_REGS ) {
				/* Reset regp for >6 (register) args */
				regp = callers_fp + FR_XTRARGS ;
				db_printf( "printargs:  xtrargs %X\n", regp );
			}
			printf("%R", get(regp, DSP));
			regp += 4;

			/* Don't print past the caller's FP */
			if( ccall_fp  &&  regp >= ccall_fp )
				break;

			if( anr < nargs-1 ) printf(",");
		}
	}

	if (val == MAXINT)
		printf(") at %X\n", ppos->k_pc );
	else
		printf(") + %X\n", val);

}

printlocals ( spos )
	struct stackpos *spos;
{
	register struct afield *f;
	int i, val;

	errflg = 0;
	for (i = cursym->s_fcnt, f = cursym->s_f; i > 0; i--, f++) {
		switch (f->f_type) {

		case N_PSYM:
			continue;

		case N_LSYM:
			val = get(spos->k_fp - f->f_offset, DSP);
			break;

		case N_RSYM:
			val = spos->k_regloc[ REG_RN( f->f_offset ) ];
			if (val == 0)
				errflg = "reg location !known";
			else
				val = get(val, DSP);
		}
		printf("%8t%s:%10t", f->f_name);
		if (errflg) {
			printf("?\n");
			errflg = 0;
			continue;
		}
		printf("%R\n", val);
	}
} /* end printlocals */


/*
 * Machine-dependent routine to handle $R $x $X commands.
 * called by printtrace (../common/print.c)
 */
fp_print ( modif ) {

	switch( modif ) {
	case 'R':
		error("no 68881");	/* never a 68881 on a sparc */
		return;

#ifdef KADB
	/* No floating point code in KADB */
	case 'x':
	case 'X':
		error("no fpa");
		return;
#else !KADB

	case 'x':
		printfpuregs(REG_F0);
		return;

	case 'X':
		printfpuregs(REG_F16);
		return;
#endif !KADB

	}
}
