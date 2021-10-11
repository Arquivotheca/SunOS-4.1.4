#ifndef lint
static	char sccsid[] = "@(#)command.c 1.1 94/10/31 SMI";
#endif

/*
 * adb - command parser
 */

#include "adb.h"
#include <stdio.h>
#include "fio.h"
#include "fpascii.h"

char	eqformat[512] = "z";
char	stformat[512] = "X^\"= \"i";

#define	QUOTE	0200
#define	STRIP	0177

int	lastcom = '=';

command(buf, defcom)
	char *buf, defcom;
{
	int itype, ptype, modifier, reg;
	int fourbyte, eqcom, atcom;
	char wformat[1];
	char c, savc;
	int w, savdot;
	char *savlp = lp;
	int locval, locmsk;

	if (buf) {
		if (*buf == '\n')
			return (0);
		lp = buf;
	}
	do {
		adb_raddr.ra_raddr = 0;		/* initialize to no */
		if (hadaddress = expr(0)) {
			dot = expv;
			ditto = dot;
		}
		address = dot;
		if (rdc() == ',' && expr(0)) {
			hadcount = 1;
			count = expv;
		} else {
			hadcount = 0;
			count = 1;
			lp--;
		}
		if (!eol(rdc()))
			lastcom = lastc;
		else {
			if (hadaddress == 0)
				dot = inkdot(dotinc);
			lp--;
			lastcom = defcom;
		}

		switch (lastcom&STRIP) {

		case '?':
#ifndef KADB
			itype = ISP; ptype = ISYM;
			goto trystar;
#endif !KADB

		case '/':
			itype = DSP; ptype = DSYM;
			goto trystar;

		case '=':
			itype = NSP; ptype = 0;
			goto trypr;

		case '@':
			itype = SSP; ptype = 0;
			goto trypr;

trystar:
			if (rdc() == '*')
				lastcom |= QUOTE;
			else
				lp--;
			if (lastcom & QUOTE) {
				itype |= STAR;
				ptype = (DSYM+ISYM) - ptype;
			}

trypr:
			fourbyte = 0;
			eqcom = lastcom=='=';
			atcom = lastcom=='@';
			c = rdc();
			if ((eqcom || atcom) && index("mLlWw", c))
				error(eqcom ?
				    "unexpected '='" : "unexpected '@'");
			switch (c) {

			case 'm': {
				int fcount, *mp;
				struct map *smap;
				struct map_range *mpr;

				/* need a syntax for setting any map range - 
				** perhaps ?/[*|0-9]m  b e f fn 
				** where the digit following the ?/ selects the map
				** range - but it's too late for 4.0
				*/
				smap = (itype&DSP?&datmap:&txtmap);
				mpr = smap->map_head;
				if (itype&STAR)
					mpr = mpr->mpr_next;
				mp = &(mpr->mpr_b); fcount = 3;
				while (fcount && expr(0)) {
					*mp++ = expv;
					fcount--;
				}
				if (rdc() == '?')
					mpr->mpr_fd = fsym;
				else if (lastc == '/')
					mpr->mpr_fd = fcor;
				else
					lp--;
				}
				break;

			case 'L':
				fourbyte = 1;
			case 'l':
				dotinc = (fourbyte ? 4 : 2);
				savdot = dot;
				(void) expr(1); locval = expv;
				locmsk = expr(0) ? expv : -1;
				if (!fourbyte) {
					locmsk = (locmsk << 16 );
					locval = (locval << 16 );
				}
				for (;;) {
					w = get(dot, itype);
					if (errflg || interrupted)
						break;
					if ((w&locmsk) == locval)
						break;
					dot = inkdot(dotinc);
				}
				if (errflg) {
					dot = savdot;
					errflg = "cannot locate value";
				}
				psymoff(dot, ptype, "");
				break;

			case 'W':
				fourbyte = 1;
			case 'w':
				wformat[0] = lastc; (void) expr(1);
				do {
				    savdot = dot;
				    psymoff(dot, ptype, ":%16t");
				    (void) exform(1,wformat,itype,ptype);
				    errflg = 0; dot = savdot;
				    if (fourbyte)
					put(dot, itype, expv);
				    else {
					/*NONUXI*/
					long longvalue = get(dot, itype);

					*(short*)&longvalue = (short)expv;
					put(dot, itype, longvalue);
				    }
				    savdot = dot;
				    printf("=%8t");
				    (void) exform(1,wformat,itype,ptype);
				    newline();
				} while (expr(0) && errflg == 0);
				dot = savdot;
				chkerr();
				break;

			default:
				lp--;
				getformat(eqcom ? eqformat : stformat);
				if (atcom) {
					if (indexf(XFILE(dot)) == 0)
						error("bad file index");
					printf("\"%s\"+%d:%16t",
					    indexf(XFILE(dot))->f_name,
					    XLINE(dot));
				} else if (!eqcom)
					psymoff(dot, ptype, ":%16t");
				scanform(count, (eqcom?eqformat:stformat),
				    itype, ptype);
			}
			break;

		case '>':
			lastcom = 0; savc = rdc();
			reg = getreg(savc);
			if (reg >= 0) {
				(void) writereg(reg, dot);
				break;
			}
			modifier = varchk(savc);
			if (modifier == -1)
				error("bad variable");
			var[modifier] = dot;
			break;

#ifndef KADB
		case '!':
			lastcom = 0;
			shell();
			break;
#endif !KADB

		case '$':
			lastcom = 0;
			printtrace(nextchar());
			break;

		case ':':
			if (!executing) {
				executing = 1;
				subpcs(nextchar());
				executing = 0;
				lastcom = 0;
			}
			break;

		case 0:
			prints("adb\n");
			break;

		default:
			error("bad command");
		}
		flushbuf();
	} while (rdc() == ';');
	if (buf)
		lp = savlp;
	else
		lp--;
	return (hadaddress && dot != 0);		/* for :b */
}
