#ifndef lint
static	char sccsid[] = "@(#)runpcs.c 1.1 94/10/31";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * adb - subprocess running routines
 */

#define BPT_INIT
#include "adb.h"
#include <stdio.h>
#include <sys/ptrace.h>
#include "fpascii.h"
#include <link.h>

/*
 * On a sparc, we must simulate single-stepping.  The only ptrace
 * calls that might try to single-step are in this file.  I've replaced
 * those calls with calls to adb_ptrace.  The following #define allows
 * ptrace to be called directly if not on a sparc.
 */
#if !defined(sparc)
#   define adb_ptrace ptrace
#endif


struct	bkpt *bkpthead;
extern debugging;
extern use_shlib;
extern struct ld_debug ld_debug_struct;
extern addr_t ld_debug_addr;
extern address_invalid;
#ifdef	KADB
extern int print_cpu_done;
#endif	KADB

#define SYM_LOADED ((addr_t)&(ld_debug_struct.ldd_sym_loaded) - (addr_t)&ld_debug_struct)
#define IN_DEBUGGER ((addr_t)&(ld_debug_struct.ldd_in_debugger) - (addr_t)&ld_debug_struct)

int	loopcnt;

#define BPOUT	0
#define BPIN	1
#define BPSTEP	2

int	bpstate = BPOUT;

addr_t userpc;
#ifdef sparc
addr_t usernpc;
#endif

/*
 * A breakpoint instruction is in the extern "bpt", defined in
 * the machine-dependent sun.h, sparc.h, or whatever.
 */

getsig(sig)
	int sig;
{

	return (expr(0) ? expv : sig);
}

runpcs(runmode, execsig)
	enum ptracereq runmode;
	int execsig;
{
	int rc = 0;
	register struct bkpt *bkpt;
	unsigned loc;
	int changed_pc = 0;

	if (hadaddress) {
#ifdef INSTR_ALIGN_ERROR
		if ( dot & INSTR_ALIGN_MASK ) {
			error( INSTR_ALIGN_ERROR );
		}
#endif INSTR_ALIGN_ERROR
		userpc = (addr_t)dot;
#ifdef sparc
		usernpc = userpc + 4;
		changed_pc = 1;
#endif
	}
	while (--loopcnt >= 0) {
#ifdef DEBUG
		if (debugging >= 2)
			printf("COUNTDOWN = %D\n", loopcnt);
#endif
		if (runmode == PTRACE_SINGLESTEP) {
			ss_setup(userpc);
		} else {
			if (bkpt = bkptlookup(userpc)) {
				execbkpt(bkpt, execsig);
				execsig = 0;
			}
			setbp();
		}

		loc = (unsigned)userpc;
		(void) adb_ptrace(runmode, pid, (int)userpc,
					execsig, changed_pc);
#ifdef KADB
#ifdef sparc
		if (runmode != PTRACE_SINGLESTEP) {
			bpwait(runmode);
			chkerr();
		}
#else
		bpwait(runmode);
		chkerr();
#endif
#else
		bpwait(runmode);
		if (pid && use_shlib){
		    ld_debug_struct = *(struct ld_debug *)read_ld_debug();
		    if (ld_debug_struct.ldd_sym_loaded){
		    put(ld_debug_addr+SYM_LOADED, DSP, 0);
			if (userpc == (addr_t)ld_debug_struct.ldd_bp_addr){
			    read_in_shlib();
			    crt0_execbkpt(userpc, execsig);
			    setbp();
			    (void) adb_ptrace(runmode, pid, (int)userpc,
						execsig, changed_pc);
			    bpwait(runmode);
			} else {
			    error("rtld doesn't place the break point right\n");
			}
		    }
		}
		chkerr();
#endif KADB
		execsig = 0; delbp();
#ifndef KADB
#ifndef sparc
		/* Special case for FPA on sun-680x0 machines */
		if (runmode == PTRACE_SINGLESTEP &&
		    signo == 0 && fpa_disasm && twomoveop(loc)) {
			(void) ptrace(runmode, pid, (int)userpc, execsig);
			bpwait(runmode); chkerr(); execsig = 0; delbp();
		}
#endif sparc
#endif !KADB
		if (signo == 0 && (bkpt = bkptlookup(userpc))) {
			dot = (int)bkpt->loc;
			if ((bkpt->comm[0] == '\n' || /* HACK HERE */
			    command(bkpt->comm, ':')) && --bkpt->count) {
				execbkpt(bkpt, execsig);
				execsig = 0;
				loopcnt++;
			} else {
				bkpt->count = bkpt->initcnt;
				rc = 1;
			}
		} else {
			execsig = signo;
			rc = 0;
		}
		if (interrupted)
			break;
	}
	return (rc);
}

endpcs()
{
	register struct bkpt *bp;

	if (pid) {
		exitproc();
		userpc = (addr_t)filhdr.a_entry;
#ifdef sparc
		usernpc = userpc + 4;
#endif
		for (bp = bkpthead; bp; bp = bp->nxtbkpt)
			if (bp->flag)
				bp->flag = BKPTSET;
	}
	address_invalid = 1;
	bpstate = BPOUT;
}

/*
 * remove the secret bp in crt0 which was set by rtld.
 */
static
crt0_execbkpt(addr, execsig)
	addr_t addr;
{
	delbp();
	put(addr, ISP, ld_debug_struct.ldd_bp_inst);
	(void) adb_ptrace(PTRACE_SINGLESTEP, pid, addr, execsig, 0);
	bpwait(PTRACE_SINGLESTEP);
	chkerr();
}

/*
 * execute the instruction at a breakpoint, by deleting all breakpoints,
 * and then single stepping (called when we're about to continue from one).
 */
static
execbkpt(bp, execsig)
	struct bkpt *bp;
	int execsig;
{

#ifdef	KADB
	print_cpu_done = 1;
#endif	KADB
	ss_setup(userpc);
	(void) adb_ptrace(PTRACE_SINGLESTEP, pid, (int)bp->loc, execsig, 0);
#ifndef KADB
	bpwait(PTRACE_SINGLESTEP);
#endif
	bp->flag = BKPTSET;
#ifdef KADB
#ifndef sparc
	bpwait(PTRACE_SINGLESTEP);
#endif
#ifdef sparc
	delbp();		/* to fix a kadb bug with multiple  */
				/* breakpoints  */
#endif
#endif
	chkerr();
#ifdef	KADB
	print_cpu_done = 0;
#endif	KADB
}

#ifndef KADB
char	**environ;

static
doexec()
{
	char *argl[BUFSIZ/8];
	char args[BUFSIZ];
	char *p = args, **ap = argl, *filnam;

	*ap++ = symfil;
	do {
		if (rdc()=='\n')
			break;
		*ap = p;
		while (!isspace(lastc)) {
			*p++=lastc;
			(void) readchar();
		}
		*p++ = '\0';
		filnam = *ap + 1;
		if (**ap=='<') {
			(void) close(0);
			if (open(filnam, 0) < 0) {
				printf("%s: cannot open\n", filnam);
				exit(0);
			}
		} else if (**ap=='>') {
			(void) close(1);
			if (creat(filnam, 0666) < 0) {
				printf("%s: cannot create\n", filnam);
				exit(0);
			}
		} else {
			ap++;
			if (ap >= &argl[sizeof (argl)/sizeof (argl[0]) - 2]) {
				printf("too many arguments\n");
				exit(0);
			}
		}
	} while (lastc != '\n');
	*ap++ = 0;
	flushbuf();
	execve(symfil, argl, environ);
	perror(symfil);
}
#endif !KADB

struct bkpt *
bkptlookup(addr)
	unsigned addr;
{
	register struct bkpt *bp;

	for (bp = bkpthead; bp; bp = bp->nxtbkpt)
		if (bp->flag && (unsigned)bp->loc == addr)
			break;
	return (bp);
}

delbp()
{
	register struct bkpt *bp;

	if (bpstate == BPOUT)
		return;
	for (bp = bkpthead; bp; bp = bp->nxtbkpt)
		if (bp->flag && (bp->flag & BKPT_ERR) == 0)
			(void) writeproc(bp->loc, (char *)&bp->ins, SZBPT);
	bpstate = BPOUT;
}

static
setbrkpt(bp)
	register struct bkpt *bp;
{
	if (readproc(bp->loc, (char *)&bp->ins, SZBPT) != SZBPT ||
	    writeproc(bp->loc, (char *)&bpt, SZBPT) != SZBPT) {
		bp->flag |= BKPT_ERR;   /* turn on err flag */
		if (use_shlib && address_invalid)
			return;
		prints("cannot set breakpoint: ");
		psymoff(bp->loc, ISYM, "\n");
	} else {
		bp->flag &= ~BKPT_ERR;  /* turn off err flag */
#ifdef DEBUG
		if (debugging >= 2)
			prints("set breakpoint\n");
#endif
	}
}

static
setbp()
{
	register struct bkpt *bp;

	if (bpstate == BPIN) {
		return;
	}
	for (bp = bkpthead; bp; bp = bp->nxtbkpt) {
		if (bp->flag) {
			setbrkpt(bp);
		}
	}
	bpstate = BPIN;
}

/*
 * Called to set breakpoints up for executing a single step at addr
 */
ss_setup(addr)
	addr_t addr;
{
#ifdef KADB
	/*
	 * For kadb we need to set all the current breakpoints except
	 * for those at addr.  This is needed so that we can hit break
	 * points in interrupt and trap routines that we may run into
	 * during a signle step.
	 */
	register struct bkpt *bp;

	for (bp = bkpthead; bp; bp = bp->nxtbkpt)
		if (bp->flag && bp->loc != addr)
			setbrkpt(bp);
	bpstate = BPSTEP;		/* force not BPOUT or BPIN */
#else KADB
	/*
	 * In the UNIX case, we simply delete all breakpoints
	 * before doing the single step.  Don't have to worry
	 * about asynchronous hardware events here.
	 */
	delbp();
#endif KADB
}

#ifndef KADB
/*
 * If the child is in another process group, ptrace will not always inform
 * adb (via "wait") when a signal typed at the terminal has occurred.
 * (Actually, if the controlling tty for the child is different from
 * the adb's controlling tty the notification fails to happen.)  Thus,
 * adb should catch the signal and if the child is in the same process
 * group, simply return (ptrace will inform adb of the signal via "wait").
 * If the child is in a different process group, adb should issue a "kill"
 * of that signal to the child.  (This will cause ptrace to "wake up" and
 * inform adb that the child received a signal.)  [Note that if the
 * child's process group is different than that of adb, the controlling
 * tty's process group is also different.]
 */
void sigint_handler()
{

	if (getpgrp(pid) != adb_pgrpid) {
		kill(pid, SIGINT);
	}
}

void sigquit_handler()
{

	if (getpgrp(pid) != adb_pgrpid) {
		kill(pid, SIGQUIT);
	}
}

bpwait(mode)
enum ptracereq mode;
{
	register unsigned w;
	int stat;
	extern char *corfil;
	int pcfudge = 0;
	void (*oldisig)();
	void (*oldqsig)();

	tbia();
rewait:
	oldisig = signal(SIGINT, sigint_handler);
	oldqsig = signal(SIGQUIT, sigquit_handler);
	while ((w = wait(&stat)) != pid && w != -1 )
		continue;
	signal(SIGINT, oldisig);
	signal(SIGQUIT, oldqsig);
#ifdef DEBUG
	if (debugging >= 1) {
		prints("wait: ");
		prmode(mode);
	}
#endif
	if (w == -1) {
		pid = 0;
		errflg = "wait error: process disappeared";
		return;
	}
	if ((stat & 0177) != 0177) {
		sigcode = 0;
		if (signo = stat&0177)
			sigprint(signo);
		if (stat&0200) {
			prints(" - core dumped\n");
			(void) close(fcor);
			corfil = "core";
			pid = 0;
			setcor();
		} else {
			pid = 0;
		}
		errflg = "process terminated";
	} else {
		signo = stat>>8;
		sigcode = ptrace(PTRACE_PEEKUSER, pid,
		    (int)&((struct user *)0)->u_code, 0);
		ptrace(PTRACE_GETREGS, pid, &core.c_regs, 0, 0);
#ifdef sparc
		core_to_regs( );
		db_printf( "bpwait:  e.g., PC is %X\n", core.c_regs.r_pc );

#ifdef KADB
		/* Get the other register windows */
		regwin_ptrace( pid, PTRACE_GETWINDOW );
#endif KADB
		db_printf( "bpwait after rw_pt:  e.g., PC is %X\n",
			core.c_regs.r_pc );
#   ifndef KADB
		stat = ptrace(PTRACE_GETFPREGS, pid, core.c_fpu.fpu_regs, 0, 0);
#	ifdef DEBUG
		db_fprdump( stat );
#	endif DEBUG

#   endif !KADB

#else  !sparc
#ifdef sun3
		if (fpa_avail) {
			ptrace(PTRACE_GETFPAREGS, pid, &core.c_fpu.f_fparegs, 0, 0);
		}
		if (mc68881) {
			ptrace(PTRACE_GETFPREGS, pid, &core.c_fpu.f_fpstatus, 0, 0);
		}
#endif sun3
#endif !sparc

#ifdef DEBUG
		if (debugging >= 1) {
			prints("wait: ");
			sigprint(signo);
			prints("\n");
		}
#endif
		if (signo != SIGTRAP) {
			if (mode == PTRACE_CONT && sigpass[signo]) {
				chk_ptrace(PTRACE_CONT, pid, (int *)1, signo);
				goto rewait;
			} /* else */
			sigprint(signo);
		} else {
			signo = 0;
#ifdef sun
			/*
			 * if pc was advanced during execution of the breakpoint
			 * (probably by the size of the breakpoint instruction)
			 * push it back to the address of the instruction, so
			 * we will later recognize it as a tripped trap.
			 */
			if (mode == PTRACE_CONT) {
#ifdef DEBUG
				if (debugging >= 1)
					prints("BREAKPOINT\n");
#endif
				pcfudge = PCFUDGE; /* for sun this is -SZBPT */
			}
#endif sun
		}
	}
	flushbuf();
	userpc = (addr_t)readreg(REG_PC);
#ifdef sparc
	usernpc = (addr_t)readreg(REG_NPC);
#endif
#ifdef DEBUG
	if (debugging >= 1) {
#ifdef sparc
		printf("User NPC is %X; ", usernpc );
#endif
		printf("User PC is %X\n", userpc );
	}
#endif DEBUG

#ifndef sparc
	if (pcfudge) {
		userpc += pcfudge;
		pc_cheat(userpc);
	}
#endif !sparc

#ifndef sun
	/* delineate current stack */
	STKmax = USRSTACK;
	STKmin = STKmax - ctob(ptrace(PTRACE_PEEKUSER, pid,
		(int)&((struct user *)0)->u_ssize, 0));
#endif sun
}  /* end of NON-KADB version of bpwait */





/*
 * Workaround for sparc kernel bug -- this routine checks to
 * see whether PTRACE_CONT with a signo is working.  Naturally,
 * adb ignores the return value of _all_ ptrace calls!
 */
chk_ptrace ( mode, pid, upc, xsig ) enum ptracereq mode; {
  int rtn, *userpc;
  extern int adb_debug;

	if ( mode != PTRACE_CONT  ||  sigpass[ signo ] == 0 )
		return ptrace( mode, pid, upc, xsig );

	rtn = ptrace( mode, pid, upc, xsig );
	if ( rtn >= 0 ) return rtn;

#ifdef DEBUG
	if ( adb_debug ) {
	    printf( "adb attempted ptrace(%d, %d, %d, %d): \n",
		mode, pid, upc, xsig );
	    printf( "adb caught signal %d\n", xsig );
	    perror( "attempt to continue subproc failed" );
	}
#endif DEBUG

#ifdef sparc
	/*
	 * Workaround:  use the pc explicitly
	 */
	userpc = (int *)readreg(REG_PC);
	rtn = ptrace( mode, pid, userpc, xsig );
	if ( rtn >= 0 ) return rtn;

#ifdef DEBUG
	printf( "adb attempted ptrace(%d, %d, %d, %d): \n",
		mode, pid, userpc, xsig );
	printf( "adb caught signal %d\n", xsig );
	perror( "attempt to continue subproc failed" );
#endif DEBUG

#endif sparc


	return rtn;
}

#endif !KADB



#ifdef KADB
/*
 * kadb's version - much simpler and fewer assumptions
 * bpwait should be in per-CPU and kadb/non-kadb directories, but
 * until I get time to do that, here it is #ifdef'd.
 */

# ifdef mc68000

bpwait(mode)		/* sun-m68k-kadb version */
enum ptracereq mode;
{
	extern int istrap;

	tbia();
	doswitch();	/* wait for something to happen */

	ptrace(PTRACE_GETREGS, pid, &core.c_regs, 0, 0);
	signo = 0;

	userpc = (addr_t)readreg(REG_PC);
	if (istrap) {
		userpc += PCFUDGE;
		pc_cheat(userpc);
	}
	delbp();	/* clear out bp's now, who knows what happened */
	flushbuf();
	tryabort(0);		/* set interrupt if user is trying to abort */
	ttysync();
}  /* end of sun-m68k-kadb version of bpwait */

#endif mc68000

# ifdef sparc

bpwait(mode)		/* sparc-kadb version */
enum ptracereq mode;
{
	extern int istrap;
	extern struct allregs adb_regs;

	tbia();
	doswitch();	/* wait for something to happen */

	ptrace(PTRACE_GETREGS, pid, &adb_regs, 0, 0);
	signo = 0;
	regs_to_core( );
	flushbuf();
	userpc = (addr_t)readreg(REG_PC);
	usernpc = (addr_t)readreg(REG_NPC);
#ifdef DEBUG
	if (debugging >= 1) {
		printf("User PC is %X [fudge %D]\n", userpc, pcfudge);
		printf("User NPC is %X\n", usernpc );
	}
#endif

	tryabort(0);		/* set interrupt if user is trying to abort */
	ttysync();
}  /* end of sparc-kadb version of bpwait */
#endif sparc
#endif KADB

#ifdef VFORK
nullsig()
{
}
#endif

setup()
{

#ifndef KADB
	(void) close(fsym); fsym = -1;
	flushbuf();
	newsubtty();
#ifndef VFORK
	if ((pid = fork()) == 0)
#else
	if ((pid = vfork()) == 0)
#endif
	{
		(void) ptrace(PTRACE_TRACEME, 0, 0, 0);		/* XXX */
#ifdef VFORK
		signal(SIGTRAP, nullsig);
#endif
		signal(SIGINT,  sigint);
		signal(SIGQUIT, sigqit);
		closettyfd();
		doexec();
		exit(0);
	}
	fsym = open(symfil, wtflag);
	if (pid == -1)
		error("try again");
	bpwait(PTRACE_TRACEME);
	lp[0] = '\n'; lp[1] = 0;
	if (fsym < 0)
		perror(symfil);
	if (errflg) {
		printf("%s: cannot execute\n", symfil);
		endpcs();
		error((char *)0);
	}
#endif !KADB
	if (use_shlib){
		read_ld_debug();
		put(ld_debug_addr+IN_DEBUGGER, DSP, 1);
	}
	bpstate = BPOUT;
}

#ifdef DEBUG
static
prmode(mode)
enum ptracereq mode;
{
	register char *m;

	switch (mode) {
	case PTRACE_TRACEME:
		m = "PTRACE_TRACEME\n";
		break;
	case PTRACE_CONT:
		m = "PTRACE_CONT\n";
		break;
	case PTRACE_SINGLESTEP:
		m = "PTRACE_SINGLESTEP\n";
		break;
	case PTRACE_KILL:
		m = "PTRACE_KILL\n";
		break;
	default:
		printf("MODE[ %d ]\n", mode);
		return;
	}
	prints(m);
}


#   ifdef sparc
#	ifndef KADB

db_fprdump ( stat ) {
  int *fptr, rn, rnx;
  extern int adb_debug;

	if ( adb_debug == 0 ) return;

	printf( "Dump of FP register area:\n" );
	printf( "ptrace returned %d; ", stat );

	if ( stat < 0 ) {
	    perror( "adb -- ptrace( GETFPREGS )" );
	} else {
	    printf( "\n" );
	}

	for ( rn = 0; rn < 32; rn += 4 ) {
	    printf( "Reg F%d:  ", rn );

	    fptr = (int *) &core.c_fpu.fpu_regs[rn];

	    printf( "%8X %8X %8X %8X\n", fptr[0], fptr[1], fptr[2], fptr[3] );
	}
	printf( "End of dump of FP register area\n" );
}



#	endif !KADB
#   endif sparc
#endif DEBUG
