#ifndef lint
static	char sccsid[] = "@(#)format.c 1.1 94/10/31 SMI";
#endif

/*
 * adb - output formatting
 */
#include "adb.h"
#include <stdio.h>
#include <ctype.h>
#include "symtab.h"
#include "fio.h"
#include "fpascii.h"

scanform(icount, ifp, itype, ptype)
	int icount;
	char *ifp;
	int itype, ptype;
{
	char *fp, modifier;
	int fcount, savdot, exact;

	while (icount) {
		fp = ifp;
		savdot = dot;

		if (itype != SSP) {
			exact = findsym(dot, ptype) == 0;
			if (exact && maxoff) {
				switch (cursym->s_type) {
				case N_ABS:
				case N_ABS|N_EXT:
				case N_COMM:
				case N_COMM|N_EXT:
					break;
				default:
					printf("\n%s:%16t", cursym->s_name);
				}
			}
		}

		while (*fp && errflg==0) {
			if (isdigit(modifier = *fp)) {
				fcount = 0;
				while (isdigit(modifier = *fp++)) {
					fcount *= 10;
					fcount += modifier-'0';
				}
				fp--;
			} else
				fcount = 1;
			if (*fp == 0)
				break;

#ifdef vax
			if (exact && dot==savdot && (itype==ISP||itype==DSP) &&
			    cursym->s_name[0]=='_' && *fp=='i') {
				(void) exform(1, "x", itype, ptype);
				fp++;
				printc('\n');
			} else
#endif
				fp = exform(fcount, fp, itype, ptype);
		}

		dotinc = dot - savdot;
		dot = savdot;

		if (errflg) {
			if (icount < 0) {
				errflg = 0;
				break;
			}
			error(errflg);
		}

		if (--icount)
			dot = inkdot(dotinc);
		if (interrupted)
			error((char *)0);
	}
} /* end scanform */


/*
 * exform, eachform and each_switch together interpret the
 * given format string "ifp" for "fcount" times;
 */

/* exform_break allows eachform to tell exform to break its while loop */
static char exform_break;

/*
 * exform_first allows exform to tell each_switch whether it's
 * the first time around exform's loop.
 */
static char exform_first;

char *
exform(fcount, ifp, itype, ptype)
	int fcount;
	char *ifp;
	int itype, ptype;
{
	char *eachform();
	char *fp;

	if (fcount <= 0) {
		/*
		 * stupid hack to make zero repeat
		 * count do something reasonable
		 */
		fp = ifp;
		if (*fp =='"') {
			/*
			 * scan out quoted string
			 * idiot really deserves a core dump here...
			 */
			fp++;
			while (*fp != '"' && *fp)
				fp++;
		}
		if (*fp)
			fp++;
		return (fp);
	}

	exform_first = 1;
	while (fcount > 0) {

		fp = eachform( fcount, ifp, itype, ptype);
		if ( exform_break )
			break;

		exform_first = 0;

		if (itype != NSP)
			dot = inkdot(dotinc);
		fcount--;
		endline();
	}
	return (fp);
} /* end of exform */


/*
 * eachform is the guts of exform's for loop.  It contains a large
 * switch which handles the blank/tab and "tT+-IAaiz" modifiers
 * (except i and z where itype isn't SSP).  Note that while most
 * of the branches simply return fp, some "ret_break", i.e., return
 * to exform AND break out of its for loop.  These are the cases
 * where the fcount can be done all at once, or where we encounter
 * an error.
 *
 * The cases that fall through are handled in each_switch.
 */
#define ret_break( fp )  { exform_break = 1; return(fp); }

char *
eachform(fcount, ifp, itype, ptype)
	int fcount;
	char *ifp;
	int itype, ptype;
{
	u_short shortval;
	int val;
	unsigned savdot;
	char *fp, modifier;
	unsigned char c;

	exform_break = 0;
	fp = ifp;
	c = *fp;
	var[0] = dot;
	modifier = *fp++;

	switch (modifier) {

	case 't': case 'T':
		printf("%T", fcount); ret_break(fp);
	case '+':
		dot = inkdot(fcount); ret_break(fp);
	case '-':
		dot = inkdot(-fcount); ret_break(fp);
	case '^':
		dot = inkdot(-dotinc*fcount); ret_break(fp);

	case ' ': case '\t':
		return (fp);

	case 'I':
		if (itype != SSP) {
			(void) pctofilex(dot);
			if (filex == 0)
				error("source line not found");
			val = filex;
			dotinc = 0;
		} else {
			val = dot;
			/*
			 * if this is all we're doing, then we must
			 * bump dot. Otherwise, we probably want
			 * multiple representations of the same dot.
			 */
			if (*fp == 0)
				dotinc = 1;
			else
				dotinc = 0;
		}
		getline(XFILE(val), XLINE(val));
		if (errflg) {
			ret_break(fp);
		}
		printf("%s", linebuf);
		return (fp);

	case 'A':
		if (itype != SSP) {
			(void) pctofilex(dot);
			val = filex;
		} else
			val = dot;
		dotinc = 0;
		if (val) {
			printf("\"%s\"+%R:%16t",
			    indexf(XFILE(val))->f_name,
				XLINE(val));
			return (fp);
		}
		psymoff(val, ptype, ":%16t");
		return (fp);

	case 'a':
		if (itype == SSP) {
			val = filextopc(dot);
			if (val == 0) {
				errflg = "pc for source line unknown";
				ret_break(fp);
			}
		} else {
			val = dot;
		}
		psymoff(val, ptype, ":%16t");
		dotinc = 0;
		return (fp);

	case 'i':
	case 'z':
		if (itype != SSP)
			break;
		savdot = dot;
		val = filextopc(dot);
		if (val != 0) {
		    /*
		     * we used just to report an error for val == 0
		     * this was very unfriendly
		     */
		    dot = val;
		    for (;;) {
			    (void) pctofilex(dot);
			    if (filex != savdot)
				    break;
			    if (charpos() == 0)
				    printf("%16m");

			    printins( modifier, ISP,
			       t_srcinstr( chkget(dot, ISP ) ) );

			    printc('\n');
			    dot = inkdot(dotinc);
		    }
		}
		dot = savdot;
		dotinc = 1;
		return (fp);
	} /* end first switch on modifier */



	if (charpos() == 0  && modifier != 'z'  && modifier != 'i' )
		printf("%16m");

	switch (modifier) {

	case '"':
		dotinc = 0;
		while (*fp != '"' && *fp)
			printc(*fp++);
		if (*fp)
			fp++;
		return (fp);

	case 'n': case 'N':
		printc('\n'); dotinc = 0; return (fp);

	}

	if (itype == SSP)
		error("format not supported with @");
	if (itype == NSP) {
		val = dot;
		shortval = dot;
		c = dot;
	} else {
		/* 32- bit hacks go here */
		val = get(dot, itype);
		/* handle big-end and little-end machines */
		shortval = *(short *)&val;
		c = *(char *)&val;
		/*
		 * All adb command/modifiers that want a 32-bit
		 * value should be listed in the "pPU4W..." string.
		 */
#ifdef sparc
		/* All sparc instructions are 32 bits long */
		if (!index("pPU4WXYQODfFiI", modifier))
			val = shortval;
#else !sparc
		if (!index("pPU4WXYQODfF", modifier))
			val = shortval;
#endif !sparc
	}
	if (errflg) {
		ret_break(fp);
	}
	if (interrupted)
		error((char *)0);
	var[0] = val;

	each_switch( modifier, val, itype, ptype, c, shortval );

	return (fp);

} /* end of eachform */



/*
 * each_switch handles most of the "normal" modifiers for exform/eachform.
 * These are:
 *	B C D E F   O P Q S U W X Y
 *	b c d e f i o p q s u w x   z
 * This is your last chance, so finally we have a default case.
 */
each_switch ( modifier, val, itype, ptype, c, shortval )
	char modifier;
	int val;
	int itype, ptype;
	unsigned char c;
	u_short shortval;
{
	unsigned savdot;
	double fval;
	union {
		int funi[3];
		double fund;
#ifdef sun3
		ext_fp x;
#endif sun3
	} fun;
	static char buf[64];


	switch (modifier) {
	case 'i':
	case 'z':
		if( ! exform_first ) {
			/* itype or ptype? */
			if( ssymoff( dot, itype, buf ) == 0 ) {
				/* Hit a symbol exactly */
				printf( "%s:", buf );
			}
			printf("%16t");
		}
		printins( modifier, itype, (ins_type)val);
		printc('\n');
		break;
	case 'P':
		dotinc = 4;
		(void) pctofilex(val);
		if (filex) {
			val = filex;
			printf("\"%s\"+%R:%16t",
			    indexf(XFILE(val))->f_name, XLINE(val));
		} else {
			psymoff(val, ptype, "%16t");
		}
		break;

	case 'p':
		dotinc = 4; psymoff(val, ptype, "%16t"); break;
	case 'u':	
		dotinc = 2; printf("%-8u", shortval); break;
	case 'U':
		dotinc = 4; printf("%-16U", val); break;
	case 'c':
		dotinc = 1; printc(c); break;
	case 'C':
		dotinc = 1; printesc(c); break;
	case 'b': case 'B':
		dotinc = 1; printf("%-8o", c); break;
	case 'w':
		dotinc = 2; printf("%-8r", shortval); break;
	case 'W':
		dotinc = 4; printf("%-16R", val); break;

	case 's': case 'S':
		savdot = dot;
		dotinc = 1;

                for( ; ; ) {
                  int g;

                        g = get(dot, itype);
                        c = first_byte( g );
                        if( c == 0  ||  errflg ) {
                            break;
                        }
                        dot = inkdot(1);
                        if (modifier == 'S')
                                printesc(c);
                        else
                                printc(c);
                        endline();
                }
                dotinc = dot - savdot + 1;
                dot = savdot;
                break;

	case 'x':
		dotinc = 2; printf("%-8x", shortval); break;
	case 'X':
		dotinc = 4; printf("%-16X", val); break;
	case 'Y':
		dotinc = 4; printf("%-24Y", val); break;
	case 'q':
		dotinc = 2; printf("%-8q", shortval); break;
	case 'Q':
		dotinc = 4; printf("%-16Q", val); break;
	case 'o':
		dotinc = 2; printf("%-8o", shortval); break;
	case 'O':
		dotinc = 4; printf("%-16O", val); break;
	case 'd':
		dotinc = 2; printf("%-8d", shortval); break;
	case 'D':
		dotinc = 4; printf("%-16D", val); break;
#ifndef KADB
	case 'f':
		dotinc = 4;
		fval = *(float *)&val;
		printf("%-16.9f", fval);
		break;
	case 'F':
		dotinc = 8;
		/* these lines depend on the layout of doubles */
		if (itype != NSP || adb_raddr.ra_raddr == 0) {
			fun.funi[0] = get(dot, itype);
			fun.funi[1] = get(dot+4, itype);
		} else {
			fun.funi[0] = *(long *)adb_raddr.ra_raddr;
			fun.funi[1] = *(long *)(adb_raddr.ra_raddr+4);
		}
		printf("%-32.18F", fun.fund);
		break;
	case 'e':
	case 'E':
		if (mc68881 == 0) {
			error("no 68881");
		} else {
#ifdef sun3
			char s[64];
			dotinc = 12;
			/*
			 * these lines depend on the
			 * layout of extendeds
			 */
			if (itype != NSP || adb_raddr.ra_raddr == 0) {
				fun.funi[0] = get(dot,itype);
				fun.funi[1] = get(dot+4,itype);
				fun.funi[2] = get(dot+8,itype);
			} else {
				fun.funi[0] = *(long *)adb_raddr.ra_raddr;
				fun.funi[1] = *(long *)
						(adb_raddr.ra_raddr+4);
				fun.funi[2] = *(long *)
						(adb_raddr.ra_raddr+8);
			}
			fprtos(&fun.x, s);
			prints(s);
#endif sun3
		}
		break;
#endif !KADB
	default:
		error("bad modifier");
	} /* end 3rd and final switch on modifier */

} /* end of each_switch */



static
printesc(c)
	char c;
{

	c &= 0177;
	if (c == 0177)
		printf("^?");
	else if (c < ' ')
		printf("^%c", c + '@');
	else
		printc(c);
}

inkdot(incr)
	int incr;
{
	unsigned newdot;

	newdot = dot + incr;
	if ((dot ^ newdot) & 0x80000000)
		error("address wrap around");
	return (newdot);
}
