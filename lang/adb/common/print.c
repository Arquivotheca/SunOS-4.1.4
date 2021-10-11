#ifndef lint
static  char sccsid[] = "@(#)print.c 1.1 94/10/31 SMI";
#endif

/*
 * adb - print routines ($ command)
 */

#define	REGNAMESINIT
#include "adb.h"
#include "symtab.h"
#ifndef KADB
#include "fpascii.h"
#endif !KADB

int	infile;
int	outfile;

#define	MAXOFF	0x8000
int	maxoff = MAXOFF;

#define	MAXPOS	80
int	maxpos = MAXPOS;

/* breakpoints */
struct	bkpt *bkpthead;

#ifndef KADB
char *signals[NSIG] = {
	"SIGundefined 0:",
	"SIGHUP 1: hangup",
	"SIGINT 2: interrupt",
	"SIGQUIT 3: quit",
	"SIGILL 4: illegal instruction",
	"SIGTRAP 5: trace/BPT",
	"SIGIOT 6: abort",
#ifdef vax
	"SIGEMT 7: undefined",
#endif vax
#ifdef sun
	"SIGEMT 7: F-line or A-line op",
#endif sun
#ifdef vax
	"SIGFPE 8: numerical exception, or subscript out of range",
#endif vax
#ifdef sun
	"SIGFPE 8: numerical exception, CHK, or TRAP",
#endif sun
	"SIGKILL 9: kill",
	"SIGBUS 10: bus error",
	"SIGSEGV 11: segmentation violation",
	"SIGSYS 12: bad system call",
	"SIGPIPE 13: broken pipe",
	"SIGALRM 14: real timer alarm",
	"SIGTERM 15: software termination",
	"SIGURG 16: urgent condition on socket",
	"SIGSTOP 17: stop",
	"SIGTSTP 18: stop from tty",
	"SIGCONT 19: continue",
	"SIGCHLD 20: child status change",
	"SIGTTIN 21: background read",
	"SIGTTOU 22: background write",
	"SIGIO 23: input available",
	"SIGXCPU 24: cpu timelimit",
	"SIGFSZ 25: file sizelimit",
	"SIGVTALRM 26: virtual timer alarm",
	"SIGPROF 27: profiling timer alarm",
	"SIGWINCH 28: window changed",
	"SIGLOST 29: resource lost",
	"SIGUSR1 30: user-defined signal 1",
	"SIGUSR2 31: user-defined signal 2",
};
#endif !KADB

printtrace(modif)
	int modif;
{
	int i, stack;
	struct map_range *mpr;

	if (hadcount == 0)
		count = -1;
	switch (modif) {

	case '<':
		if (count == 0) {
			while (readchar() != '\n')
				;
			lp--;
			break;
		}
		if (rdc() == '<')
			stack = 1;
		else {
			stack = 0;
			lp--;
		}
		/* fall into ... */
	case '>': {
		char file[64], Ifile[128];
		extern char *Ipath;
		int index;

		index = 0;
		if (modif == '<')
			iclose(stack, 0);
		else
			oclose();
		if (rdc() != '\n') {
			do {
				file[index++] = lastc;
				if (index >= 63)
					error("filename too long");
			} while (readchar() != '\n');
			file[index] = '\0';
			if (modif == '<') {
				infile = open(file, 0);
				if (infile < 0) {
#ifndef KADB
					(void) sprintf(Ifile, "%s/%s",
					    Ipath, file);
					if ((infile = open(Ifile, 0)) < 0)
#endif !KADB
						infile = 0, error("can't open");
				}
				var[9] = hadcount ? count : 1;
			} else {
				outfile = open(file, 1);
				if (outfile < 0)
					outfile = creat(file, 0666);
				else
					(void) lseek(outfile, (long)0, 2);
			}
		} else
			if (modif == '<')
				iclose(-1, 0);
		lp--;
		}
		break;

	case 'p':
#ifndef KADB
		if (kernel == 0) {
			printfuns();
			break;
		}
#endif !KADB
#ifdef vax
		if (hadaddress) {
			int pte = rwmap('r', dot, DSP, 0);
			masterpcbb = (pte&PG_PFNUM)*NBPG;
		}
		getpcb((int)ptob(upage));
#endif
#ifdef sun
		if (hadaddress) {
			masterprocp = dot;
			getproc();
		}
#endif
		break;

	case 'f':
		printfiles();
		break;

#ifndef KADB
	case 'i':
		whichsigs();
		break;
#endif !KADB

	case 'd':
		if (hadaddress) {
			if (address < 2 || address > 16)
				error("must have 2 <= radix <= 16");
			radix = address;
		}
		printf("radix=%d base ten", radix);
		break;

	case 'q': case '%':
#ifdef KADB
		kadb_done();
		break;
#else
		done();
#endif

	case 'w':
		maxpos = hadaddress ? address : MAXPOS;
		break;

	case 's':
		maxoff = hadaddress ? address : MAXOFF;
		break;

	case 'v':
		prints("variables\n");
		for (i=0; i < NVARS; i++)
			if (var[i]) {
				char c;
				if (i <= 9)
					c = i + '0';
				else if (i <= 36)
					c = i - 10 + 'a';
				else
					c = i - 36 + 'A';
				printc(c);
				printf(" = %Q\n", var[i]);
			}
		break;

	case 'm':
		printmap("? map", &txtmap);
		printmap("/ map", &datmap);
		break;

	case 0: case '?':
#ifndef KADB
		if (pid)
			printf("pcs id = %d\n", pid);
		else
			prints("no process\n");
		sigprint(signo);
#endif !KADB
		flushbuf();
		/* fall into ... */
	case 'r':
		printregs();
		return;

	/* Floating point register commands are machine-specific */
	case 'R':
	case 'x':
	case 'X':
		fp_print( modif );	/* see ../${CPU}/print*.c */
		return;

	case 'C':
	case 'c':
		printstack(modif);
		return;

	case 'e': {
		register struct asym **p;
		register struct asym *s;

		for (p = globals; p < globals + nglobals; p++) {
			s = *p;
			switch (s->s_type) {

			case N_DATA|N_EXT:
			case N_BSS|N_EXT:
			case N_PC:
				printf("%s:%12t%R\n", s->s_name,
				   get((int)s->s_value, DSP));
			}
		}
		}
		break;

	case 'a':
		error("No algol 68 here");
		/*NOTREACHED*/

	case 'b': {
		register struct bkpt *bkptr;

		printf("breakpoints\ncount%8tbkpt%24tcommand\n");
		for (bkptr = bkpthead; bkptr; bkptr = bkptr->nxtbkpt)
			if (bkptr->flag) {
		   		printf("%-8.8d", bkptr->count);
				psymoff(bkptr->loc, ISYM, "%24t");
				printf("%s", bkptr->comm);
			}
		}
		break;

	case 'W':
		wtflag = 2;
#ifndef KADB
		close(fsym);
		close(fcor);
		kopen();		/* re-init libkvm code, if kernel */
		fsym = getfile(symfil, 1);
		mpr = txtmap.map_head;	/* update fd fields of the first 2 map_ranges*/
		mpr->mpr_fd = fsym; mpr->mpr_next->mpr_fd = fsym;
		fcor = getfile(corfil, 2);
		mpr = datmap.map_head;
		mpr->mpr_fd = fcor; mpr->mpr_next->mpr_fd = fcor;
#endif !KADB
		break;

#ifdef KADB
	case 'M':
		printmacros();
		break;
#endif KADB

	default:
		error("bad modifier");
	}
}

static
printmap(s, amap)
	char *s;
	struct map *amap;
{
	int count;
	struct map_range *mpr;

	printf("%s%\n", s);
	count = 1;
	for (mpr = amap->map_head; mpr; mpr = mpr->mpr_next) {
		printf("b%d = %-12R", count, mpr->mpr_b );
		printf("e%d = %-12R", count, mpr->mpr_e );
		printf("f%d = %-12R", count, mpr->mpr_f );
		printf("`%s'\n", mpr->mpr_fd < 0 ? "-" : mpr->mpr_fn);
		count ++;
	}
}


getreg(regnam)
	char regnam;
{
	register int i;
	register char c;
	char *olp = lp;
	char name[30];

	olp = lp;
	i = 0;
	if (isalpha(regnam)) {
		name[i++] = regnam;
		c = readchar();
		while (isalnum(c) && i < 30) {
			name[i++] = c;
			c = readchar();
		}
		name[i] = 0;
		for (i = 0; i < NREGISTERS; i++) {
			if (strcmp(name,regnames[i]) == 0) {
				lp--;
				return(i);
			}
		}
	}
	lp = olp;
	return (-1);
}

printpc()
{
	dot = readreg(REG_PC);
	psymoff(dot, ISYM, ":%16t");

	/* print the instruction without timings */
	printins( 'i', ISP, t_srcinstr( chkget(dot, ISP ) ) );

	printc('\n');
}


#ifdef vax
char	*illinames[] = {
	"reserved addressing fault",
	"priviliged instruction fault",
	"reserved operand fault"
};
char	*fpenames[] = {
	0,
	"integer overflow trap",
	"integer divide by zero trap",
	"floating overflow trap",
	"floating/decimal divide by zero trap",
	"floating underflow trap",
	"decimal overflow trap",
	"subscript out of range trap",
	"floating overflow fault",
	"floating divide by zero fault",
	"floating undeflow fault"
};
#endif vax

#ifndef KADB
/*
 * indicate which signals are passed to the subprocess.
 * implements the $i command
 */
static
whichsigs()
{
	register i;

	for (i = 0; i < NSIG; i++)
		if (sigpass[i]) {
			printf("%R\t", i);
			sigprint(i);
			prints("\n");
		}
}

sigprint(signo)
	int signo;
{

	if ((signo>=0) && (signo<sizeof signals/sizeof signals[0]))
		prints(signals[signo]);
#ifdef vax
	switch (signo) {

	case SIGFPE:
		if (sigcode > 0 &&
		    sigcode < sizeof fpenames / sizeof fpenames[0]) {
			prints(" ("); prints(fpenames[sigcode]); prints(")");
		}
		break;

	case SIGILL:
		if (sigcode >= 0 &&
		    sigcode < sizeof illinames / sizeof illinames[0]) {
			prints(" ("); prints(illinames[sigcode]); prints(")");
		}
		break;
	}
#endif
}
#endif !KADB
