#ifndef lint
static	char sccsid[] = "@(#)pcs.c 1.1 94/10/31 SMI";
#endif

/*
 * adb - process control
 */
#include "adb.h"
#include <sys/ptrace.h>

/* breakpoints */
struct	bkpt *bkpthead;

int	loopcnt;
extern int filex;

subpcs(modif)
	int modif;
{
	register int check;
	int execsig;
	enum ptracereq runmode;
	register struct bkpt *bkptr;
	char *comptr;
	int i, line, hitbp = 0;
	char *m;

	execsig = 0;
	loopcnt = count;
	switch (modif) {

	case 'D':
		dot = filextopc(dot);
		if (dot == 0)
			error("don't know pc for that source line");
		/* fall into ... */
	case 'd':
		bkptr = bkptlookup(dot);
		if (bkptr == 0)
			error("no breakpoint set");
		bkptr->flag = 0;
		return;

	case 'B':
		dot = filextopc(dot);
		if (dot == 0)
			error("don't know pc for that source line");
		/* fall into ... */
	case 'b':
		bkptr = bkptlookup(dot);
		if (bkptr)
			bkptr->flag = 0;
		for (bkptr = bkpthead; bkptr; bkptr = bkptr->nxtbkpt)
			if (bkptr->flag == 0)
				break;
		if (bkptr == 0) {
			bkptr = (struct bkpt *)malloc(sizeof *bkptr);
			if (bkptr == 0)
				error("bkpt: no memory");
			bkptr->nxtbkpt = bkpthead;
			bkpthead = bkptr;
		}
#ifdef INSTR_ALIGN_MASK
		if( dot & INSTR_ALIGN_MASK ) {
			error( BPT_ALIGN_ERROR );
		}
#endif INSTR_ALIGN_MASK
		bkptr->loc = (addr_t)dot;
		bkptr->initcnt = bkptr->count = count;
		bkptr->flag = BKPTSET;
		check = MAXCOM-1;
		comptr = bkptr->comm;
		(void) rdc(); lp--;
		do
			*comptr++ = readchar();
		while (check-- && lastc != '\n');
		*comptr = 0; lp--;
		if (check)
			return;
		error("bkpt command too long");

#ifndef KADB
	case 'i':
	case 't':
		if (!hadaddress)
			error("which signal?");
		if (expv <= 0 || expv >=NSIG)
			error("signal number out of range");
		sigpass[expv] = modif == 'i';
		return;

	case 'k':
		if (pid == 0)
			error("no process");
		printf("%d: killed", pid);
		endpcs();
		return;

	case 'r':
		endpcs();
		setup();
		runmode = PTRACE_CONT;
#ifdef notdef
		if (hadaddress) {
			if (!bkptlookup(dot))
				loopcnt++;
		} else
#ifdef vax
			if (!bkptlookup(filhdr.a_entry+2))
#endif
#ifdef sun
			if (!bkptlookup(filhdr.a_entry))
#endif
				loopcnt++;
#endif notdef
		subtty();
		break;
#endif !KADB

	case 's':
	case 'S':
		if (pid) {
			execsig = getsig(signo);
		} else {
			setup();
			loopcnt--;
		}
		runmode = PTRACE_SINGLESTEP;
		if (modif == 's')
			break;
		if ((pctofilex(userpc), filex) == 0)
			break;
		subtty();
		for (i = loopcnt; i > 0; i--) {
			line = (pctofilex(userpc), filex);
			if (line == 0)
				break;
			do {
				loopcnt = 1;
				if (runpcs(runmode, execsig)) {
					hitbp = 1;
					break;
				}
				if (interrupted)
					break;
			} while ((pctofilex(userpc), filex) == line);
			loopcnt = 0;
		}
		break;

	case 'c':
		if (pid == 0)
			error("no process");
		runmode = PTRACE_CONT;
		execsig = getsig(signo);
		subtty();
		break;

	default:
		error("bad modifier");
	}

	if (loopcnt > 0 && runpcs(runmode, execsig) || hitbp)
		m = "breakpoint%16t";
	else
		m = "stopped at%16t";
	adbtty();
	printf(m);
	delbp();
	printpc();
}
