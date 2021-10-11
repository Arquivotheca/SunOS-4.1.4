#ifndef lint
static	char sccsid[] = "@(#)process.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Process management.
 *
 * This module contains the routines to manage the execution and
 * tracing of the debuggee process.
 */

#include "defs.h"
#include "ttyfix.h"
#include "process.h"
#include "machine.h"
#include "events.h"
#include "tree.h"
#include "eval.h"
#include "operators.h"
#include "source.h"
#include "symbols.h"
#include "object.h"
#include "mappings.h"
#include "dbxmain.h"
#include "coredump.h"
#include "getshlib.h"
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/param.h>
#include <sys/user.h>
#include <sys/stat.h>
#include <sys/filio.h>

#define _IOCTL_ To prevent inclusion of <sys/ioctl.h>.
#include <sundev/kbio.h>

#include <sundev/kbd.h>
#include <sys/ptrace.h>


#ifndef public

typedef struct Process *Process;

Process process;

int read_err_flag;

#include "machine.h"

#endif

/* NOT PUBLIC! */
private Process *dbg_proc_ptr = &process;

#define NOTSTARTED 1
#define STOPPED 0177
#define FINISHED 0

#define SYM_LOADED ((caddr_t)&(ld_debug_struct.ldd_sym_loaded) - (caddr_t)&ld_debug_struct)
#define IN_DEBUGGER ((caddr_t)&(ld_debug_struct.ldd_in_debugger) - (caddr_t)&ld_debug_struct)

/*
 * Cache-ing of instruction segment is done to reduce the number
 * of system calls.
 */

#define ICACHE_SIZE 1003       /* size of instruction cache */

typedef struct {
    Word addr;
    Word val;
} CacheWord;

/*
 * This structure holds the information we need from the user structure.
 */

struct Process {
    int pid;			/* process being traced */
    struct regs reg;		/* process' registers */
    struct regs oreg;		/* registers when process last stopped */
    struct pcb  rpcb;		/* registers from other windows */
    struct fpu rfpu;		/* floating point registers */
    short status;		/* either STOPPED or FINISHED */
    short signo;		/* signal that stopped process */
    short sigcode;		/* extra signal information */
    int exitval;		/* return value from exit() */
    int pc_changed;		/* have we changed the pc since stopping? */
    long sigset;		/* bit array of traced signals */
    CacheWord word[ICACHE_SIZE]; /* text segment cache */
    Ttyinfo ttyinfo;		/* process' terminal characteristics */
};

/*
 * These definitions are for the arguments to "pio".
 */

typedef enum { PREAD, PWRITE } PioOp;
typedef enum { TEXTSEG, DATASEG } PioSeg;

private struct Process pbuf;

#define MAXNCMDARGS 500         /* maximum number of arguments to RUN */
extern int errno;
extern Boolean nocerror;	/* if false, a ptrace error return bombs dbx */

private Boolean just_started;
private int argc;
private String argv[MAXNCMDARGS];
private String infile;
private String outfile;
private String appfile;

private int dbx_pgrpid;

/*
 * Initialize process information.
 */
public process_init()
{
    register Integer i;
    Symbol t;
    Char buf[10];

    process = &pbuf;
    process->status = (coredump) ? STOPPED : NOTSTARTED;
    setsigtrace();
    bzero((char *) &process->reg, sizeof(process->reg));
    bzero((char *) &process->oreg, sizeof(process->oreg));

    for( i= REG_G0; i <= REG_I7; ++i ) {
	sprintf( buf, "$%s", regnames[i] );	/* insert the '$' */
	defregname( identname( buf, false ), i, t_int );
    }

    t = newSymbol(nil, 0, PTR, t_int, nil);	/* make a fake pointer type */
	t->language = findlanguage(".c");

    /*
     * Define near-synonyms for o6 & i6 respectively.  Note the
     * subtle difference -- if you refer to $o6 or $i6, you
     * get an int, but if you refer to $sp or $fp, it's a pointer.
     */
    defregname(identname("$sp", true), REG_SP, t);
    defregname(identname("$fp", true), REG_FP, t);

    for (i = REG_Y; i <= REG_TBR ; i++) {
	sprintf( buf, "$%s", regnames[i] );	/* insert the '$' */
        defregname( identname( buf, false ), i, t_int);
    }
    defregname(identname("$pc", true),  REG_PC,  t);
    defregname(identname("$npc", true), REG_NPC, t);
    defregname(identname("$fsr", true), REG_FSR, t_int);
    defregname(identname("$fq", true),  REG_FQ,  t);

    /* Create the floating point register names */
    for (i = REG_F0; i <= REG_F31 ; i++) {
	sprintf( buf, "$%s", regnames[i] );	/* insert the '$' */
        defregname( identname( buf, false ), i, t_real);
    }

    /* Need another set for double-precision? */

    if (coredump) {
	coredump_readin(&process->reg, &process->signo, &process->sigcode,
			&process->rpcb, &process->rfpu);
	if (coredump) {
	    pc = process->reg.r_pc;
	}
    }
    arginit();
    dbx_pgrpid = getpgrp(0);
}

/*
 * fetch the value of a register in the context of the current frame
 */
public Word reg(n)
Integer n;
{
    Word *wp, value;

    wp = regaddr( n );
	if( wp) {
		if(IS_WINDOW_REG(n)) {
			dread(&value, wp, sizeof(value));
		} else {
			value = *wp;
		}
    } else if( n == REG_G0 ) {
		value = 0; /* be nice */
    } else if( process->pid == 0 ) {
		value = 0; /* no process ==> lie */
	} else {
		error("Invalid register number (%d)", n);
	}
	return value;
}

public void setreg(n, w)
Integer n;
Word w;
{
    register Word *wp;

    wp = regaddr( n );
    if( wp == nil ) {
		panic("Invalid register number (%d)", n);
    } else {
		if(IS_WINDOW_REG(n)) {
			dwrite(&w, wp, sizeof(w));
		} else {
			*wp = w;
		}
    }
}



/*
 * reg_windows -- read or write value of register regn to/from
 * dbx' internal data structures (and core file or subprocess).
 * regn should be in the set from REG_L0 thru REG_I7 (unless
 * we extend dbx to allow access to other windows).
 *
 * The register might not be in the c_regs structure:  if the
 * stack was not too confusing, the kernel will have stored
 * these registers into their proper places in the stack frame;
 * if the stack was trashed, it will have kept them
 * in the user struct's pcb struct as u.u_pcb.pcb_stuff, where
 * stuff = { wbuf, spbuf, wbcnt, uwm }.
 *
 * NOTE:  The value that reg_windows returns may be misleading
 * in the case of a "leaf" procedure.
 *
 * The start of the sparc stack frame is just a struct rwindow (the saved
 * local & in registers, which is all that reg_windows is interested in),
 * followed by a few other things.   [See the definition of "struct
 * real_Frame" in runtime.c.]
 */

#define FR_IREG(reg_nr)

private Word * reg_windows( regn )
int regn;
{
    register Word *wp, *usp;
    struct pcb *pcb_ptr;
    static struct rwindow *stack_regs; /* defined in <machine/pcb.h> */

    pcb_ptr = & process->rpcb ;
    if( pcb_ptr->pcb_wbcnt == 0 ) {
	/*
	 * No windows in pcb.  Use the stack itself.
	 * return the address of the saved register
	 */
	usp = (Word *) process->reg.r_sp;
	stack_regs = (struct rwindow*) usp;
	if( regn >= REG_I0 ) {
	    wp = (Word *)&( stack_regs->rw_in[ regn-REG_I0 ] );
	} else {
	    wp = (Word *)&( stack_regs->rw_local[ regn-REG_L0 ] );
	}
    } else {
	/*
	 * There are windows in the pcb.
	 */
	if( regn >= REG_I0 ) {
	    wp = (Word *)&( pcb_ptr->pcb_wbuf[ 0 ].rw_in[ regn-REG_I0 ] );
	} else {
	    wp = (Word *)&( pcb_ptr->pcb_wbuf[ 0 ].rw_local[ regn-REG_L0 ] );
	}

    }

    return wp ;

} /* end reg_windows */


public Word *regaddr(n)
Integer n;
{
    register Word *wp;

    switch( n ) {
     case REG_PSR: wp = (Word *)&process->reg.r_psr;  break;
     case REG_PC:  wp = (Word *)&process->reg.r_pc ;  break;
     case REG_NPC: wp = (Word *)&process->reg.r_npc;  break;
     case REG_G1:  wp = (Word *)&process->reg.r_g1 ;  break;
     case REG_G2:  wp = (Word *)&process->reg.r_g2 ;  break;
     case REG_G3:  wp = (Word *)&process->reg.r_g3 ;  break;
     case REG_G4:  wp = (Word *)&process->reg.r_g4 ;  break;
     case REG_G5:  wp = (Word *)&process->reg.r_g5 ;  break;
     case REG_G6:  wp = (Word *)&process->reg.r_g6 ;  break;
     case REG_G7:  wp = (Word *)&process->reg.r_g7 ;  break;
     case REG_O0:  wp = (Word *)&process->reg.r_o0 ;  break;
     case REG_O1:  wp = (Word *)&process->reg.r_o1 ;  break;
     case REG_O2:  wp = (Word *)&process->reg.r_o2 ;  break;
     case REG_O3:  wp = (Word *)&process->reg.r_o3 ;  break;
     case REG_O4:  wp = (Word *)&process->reg.r_o4 ;  break;
     case REG_O5:  wp = (Word *)&process->reg.r_o5 ;  break;
     case REG_O6:  wp = (Word *)&process->reg.r_o6 ;  break;
     case REG_O7:  wp = (Word *)&process->reg.r_o7 ;  break;

     default:
	if( n >= REG_F0  &&  n <= REG_F31 ) {
	    /* Floating point registers */
	    wp = (Word *)&process->rfpu.fpu_regs[ n - REG_F0 ];
	} else if( n >= REG_L0  &&  n <= REG_I7 ) {
	    /* Window (Local/In) registers */
	    wp = reg_windows( n );
	} else {
	    wp = nil;
	}
	break;
    } /* end switch */

    return wp;
}

private crt0_execbp()
{
	iwrite(&ld_debug_struct.ldd_bp_inst, pc, sizeof(ld_debug_struct.ldd_bp_inst));
	isstopped = false; /* WHY ?*/
	/* now that rtld's done, we want to deal with any actions associated
	** with pc==codestart - typically a traceon - so do a bpact 
	** also, since we're not actually at codestart, adjust the definition
	** of "beginning of program"
	*/
	program->symvalue.funcv.beginaddr = pc;
	bpact(false);
	stepover();
}

/*
 * Begin execution.
 *
 * We set a breakpoint at the end of the code so that the
 * process data doesn't disappear after the program terminates.
 */

private Boolean remade();
private Boolean newobject = false;

public start(args, in, out, app)
String args[];
String in, out, app;
{
    String pargv[4];
    Node cond;
    Address last_addr;

    if (coredump) {
	coredump = false;
	fclose(corefile);
	coredump_close();
    }
    if (args == nil) {
	args = pargv;
	pargv[0] = objname;
	pargv[1] = nil;
    } else {
	args[argc] = nil;
    }
    if (remade(objname)) {
	printf("Program \"%s\" has been recompiled", args[0]);
	printf(" - breakpoints may be incorrect.\n");
	fflush(stdout);
	reinit();
    }
    pstart(process, args, in, out, app);
    if (process->status == STOPPED && use_shlib){
	resume(0);
	pc = reg(REG_PC);
	read_ld_debug();
	if (ld_debug_struct.ldd_sym_loaded && pc == (Address)ld_debug_struct.ldd_bp_addr){
	    int z = 0;
	    getshlib();
	    iwrite(&z, ld_debug_addr+SYM_LOADED, sizeof(z));
	    crt0_execbp();
	} else {
	    error("rtld doesn't place the break point right (try -Bstatic)\n");
	}
    }
    if (process->status == STOPPED) {
	/* pc = codestart; WHY?? */
	setcurfunc(program);
	last_addr = lastaddr();
	if ((objsize != 0) && (last_addr != 0)) {
	    cond = build(O_EQ, build(O_SYM, pcsym), build(O_LCON, last_addr));
	    event_once(cond, buildcmdlist(build(O_ENDX)));
	}
    }
}

private int modtime;

/*
 * Get and remember the last modification time of the object file.
 */
public	objtime(filename)
String	filename;
{
    struct stat statb;

    stat(filename, &statb);
    modtime = statb.st_mtime;
}

/*
 * Return the last modification time of the object file.
 */
public	int get_objtime()
{
    return(modtime);
}

/*
 * Check to see if the object file has changed since the symbolic
 * information last was read.
 */
private Boolean remade(filename)
String filename;
{
    struct stat statb;
    Boolean b;

    stat(filename, &statb);
    b = (Boolean) (modtime != 0 and modtime < statb.st_mtime);
    modtime = statb.st_mtime;
    newobject = b;
    return b;
}

/*
 * Set up what signals we want to trace.
 */

private setsigtrace()
{
    register Integer i;
    register Process p;

    p = process;
    for (i = 1; i < NSIG; i++) {
	psigtrace(p, i, true);
    }
    psigtrace(p, SIGHUP, false);
    psigtrace(p, SIGEMT, false);
    psigtrace(p, SIGFPE, false);
    psigtrace(p, SIGKILL, false);
    psigtrace(p, SIGALRM, false);
    psigtrace(p, SIGTSTP, false);
    psigtrace(p, SIGCONT, false);
    psigtrace(p, SIGCHLD, false);
    psigtrace(p, SIGWINCH, false);
}

/*
 * Initialize the argument list.
 */

public arginit()
{
    infile = nil;
    outfile = nil;
    appfile = nil;
    argv[0] = objname;
    if (objname != nil) {
	argc = 1;
    } else {
	argc = 0;
    }
}

/*
 * Add an argument to the list for the debuggee.
 */

public newarg(arg)
String arg;
{
    if (argc >= MAXNCMDARGS) {
	error("too many arguments");
    }
    argv[argc++] = arg;
}

/*
 * Set the standard input for the debuggee.
 */

public inarg(filename)
String filename;
{
    if (infile != nil) {
	error("multiple input redirects");
    }
    infile = filename;
}

/*
 * Set the standard output for the debuggee.
 * Probably should check to avoid overwriting an existing file.
 */

public outarg(filename)
String filename;
{
    if (outfile != nil or appfile != nil) {
	error("multiple output redirect");
    }
    outfile = filename;
}

/*
 * Set the standard output to be append to a file.
 */
public apparg(filename)
String filename;
{
    if (appfile != nil or outfile != nil) {
	error("multiple output redirect");
    }
    appfile = filename;
}

/*
 * Start debuggee executing.
 * Process is set to nil by kill_process.
 * If we want to re-run the program and process is nil,
 * set process to &pbuf and away we go.
 */

public run()
{
    if (process == nil) {
	if (argv[0] == nil) {
	    error("no process to run");
        }
	process = &pbuf;
    }
    process->status = STOPPED;
    reset_callstack();
    fixbps();
    brkline = 0;
	useInstLoc = false;
    dbx_resume();
    printf("Running: ");
    pr_cmdline(false, 0);
    start(argv, infile, outfile, appfile);
    just_started = true;
    isstopped = false;
    cont(0);
}

/*
 * Continue execution, usually from wherever we left off.
 * (Unless the user gave a "cont at line_number" command.)
 *
 * Note that this routine never returns.  Eventually bpact() will fail
 * and we'll call printstatus or step will call it.
 */

typedef int Intfunc();

private struct sigvec odbintr;
private void intr();

#define succeeds    == true
#define fails       == false

public cont(signo)
int signo;
{
    struct sigvec sigv;

    sigv.sv_handler = intr;
    sigv.sv_mask = 0;
    sigv.sv_onstack = 0;
    sigvec(SIGINT, &sigv, &odbintr);
    if (just_started) {
	just_started = false;
    } else {
	can_continue();
	isstopped = false;
	/* if stopped by catch'ing signal, don't stepover
	 * especially if stopped at a system call [see bug 1019020]
	 * this step would not return until the indefinite future,
	 * and we would not get a chance to issue the resume(signo) below
	 */
	if (isbperr()) stepover();
    }
    for (;;) {
	if (single_stepping) {
	    printnews();
	} else {
	    setallbps();
	    resume(signo);
	    unsetallbps();
	    if (bpact(true) fails) {
		printstatus();
	    }
	}
	stepover();
    }
    /* NOTREACHED */
}

/*
 * Continue at a certain line #
 */

cont_at(addr, sig)
Address addr;
int sig;
{
    struct sigvec sigv;

    isstopped = true;
    can_continue();
    process->reg.r_pc = addr;
    process->pc_changed = 1;
    pc = addr;

    /*
     * cont(sig);
     * The 'stepover()' causes problems because of NPC and isn't really needed
     * for CONT AT anyway. Duplicated 'cont()' below minus 'stepover()'.
     * Alternative would have been to give 'cont()' a flag but that would have
     * affected stuff in ../common.
     * -- ivan
     */

    sigv.sv_handler = intr;
    sigv.sv_mask = 0;
    sigv.sv_onstack = 0;
    sigvec(SIGINT, &sigv, &odbintr);
    if (just_started) {
	just_started = false;
    } else {
	can_continue();
	isstopped = false;
	/*
	stepover();
	*/
    }
    for (;;) {
	if (single_stepping) {
	    printnews();
	} else {
	    setallbps();
	    resume(sig);
	    unsetallbps();
	    if (bpact(true) fails) {
		printstatus();
	    }
	}
	stepover();
    }
    /* NOTREACHED */
}  

/*
 * This routine is called if we get an interrupt while "running" px
 * but actually in the debugger.  Could happen, for example, while
 * processing breakpoints.
 *
 * We basically just want to keep going; the assumption is
 * that when the process resumes it will get the interrupt
 * which will then be handled.
 */

private void intr()
{
}

public fixintr()
{
    sigvec(SIGINT, &odbintr, 0);
}

/*
 * These two routines are entered while child is running.  (See comment
 * above "sigs_off".)
 */
private void sigint_handler()
{
    if (getpgrp(process->pid) != dbx_pgrpid) {
	kill(process->pid, SIGINT);
    }
}

private void sigquit_handler()
{
    if (getpgrp(process->pid) != dbx_pgrpid) {
	kill(process->pid, SIGQUIT);
    }
}

/*
 * Resume execution.
 */

public resume(signo)
int signo;
{
    register Process p;

    p = process;
    if (traceexec) {
	if( process->pc_changed ) {
	    printf("execution resumes at pc 0x%x (CHANGED).\n",
		process->reg.r_pc);
	} else {
	    printf("execution resumes at pc 0x%x\n", process->reg.r_pc);
	}
	fflush(stdout);
    }
    pcont(p, signo);
    pc = process->reg.r_pc;
    if (traceexec) {
	printf("execution stops at pc 0x%x on sig %d\n",
	    process->reg.r_pc, p->signo);
	fflush(stdout);
    }
    if (p->status != STOPPED) {
	if (p->signo != 0) {
	    dirty_exit = 1;
	    printf("program terminated by ");
	    psig(p->signo, p->sigcode);
	    printf("\n");
	    erecover();
	} else if (not runfirst) {
	    dirty_exit = 1;
	    printf("program exited with %d\n", p->exitval);
	    erecover();
	}
    }
}

/*
 * Single-step over the current machine instruction.
 *
 * If we're single-stepping by source line we want to step to the
 * next source line.  Otherwise we're going to continue so there's
 * no reason to do all the work necessary to single-step to the next
 * source line.
 */

public stepover()
{
    Boolean b;

    if (single_stepping) {
	dostep(false);
    } else {
	b = inst_tracing;
	inst_tracing = true;
	dostep(false);
	inst_tracing = b;
    }
}

/*
 * Resume execution up to the given address.  It is assumed that
 * no breakpoints exist between the current address and the one
 * we're stepping to.  This saves us from setting all the breakpoints.
 */

public stepto(addr)
Address addr;
{

    if( reg( REG_PC ) == addr ) return;

    if (addr != CALL_RETADDR) {
        setbp(addr);
    }
    resume(0);
    if (addr != CALL_RETADDR) {
        unsetbp(addr);
    }
    if (not isbperr()) {
	printstatus();
    }
}

/*
 * Print the status of the process.
 * This routine does not return.
 *
 * If -r option was specified, must call init() since object
 * file information hasn't been read in yet.
 */

public printstatus()
{
    if (process->status == FINISHED) {
	quit(0);
    } else {
	if (runfirst) {
	    printf("Entering debugger ...\n");
	    init();
	}
	getsrcpos();
	brkfunc = curfunc;
	if (curfunc != nil and curfunc->symvalue.funcv.src) {
            brksource = cursource;
	} else {
	    brksource = nil;
	    find_some_source();
	}
	/*
	 * Restore the "dbx-time" tty modes, in case the
	 * debuggee has messed things up.  (The "0" tells
	 * restoretty to use ttyfix's private ttyinfo struct.)
	 */
	restoretty( 0 );
	dbx_stopped();
	if (process->signo == SIGINT) {
	    isstopped = true;
	    printerror();
	} else if (isbperr() and isstopped) {
	    if (not istool()) {
	        printf("stopped ");
	        printloc();
	        putchar('\n');
	        if (brkline > 0) {
		    printlines(brkline, brkline);
	        } else {
		    printinst(pc, pc);
	        }
	    } else if (useInstLoc) {
	        printf("stopped ");
	        printloc();
	        putchar('\n');
		printinst(pc, pc);
	    }
	    if (curfunc != brkfunc) {
		printf("Current function is %s\n", symname(curfunc));
		if (not istool()) {
		    printlines(cursrcline, cursrcline);
		}
	    }
	    pr_display(false);
	    erecover();
	} else {
	    fixbps();
	    fixintr();
	    isstopped = true;
	    printerror();
	}
    }
}

/*
 * Print out the current location in the debuggee.
 */

public printloc()
{
    printf("in ");
    printname(brkfunc);
    printf(" ");
    if (brkline > 0 and not useInstLoc and brkfunc->symvalue.funcv.src) {
	printsrcpos();
    } else {
	useInstLoc = false;
	brkline = 0;
	printf("at 0x%x", pc);
    }
}

/*
 * Some functions for testing the state of the process.
 */

public Boolean notstarted(p)
Process p;
{
    return (Boolean) (p == nil || p->status == NOTSTARTED);
}

public Boolean isfinished(p)
Process p;
{
    return (Boolean) (p != nil && p->status == FINISHED);
}

/*
 * Return the signal number which stopped the process.
 */

public Integer errnum(p)
Process p;
{
    return p->signo;
}

/*
 * Return the signal code  associated with the signal.
 */

public Integer errcode(p)
Process p;
{
    return p->sigcode;
}

/*
 * Return the termination code of the process.
 */

public Integer exitcode(p)
Process p;
{
    return p->exitval;
}

/*
 * These routines are used to access the debuggee process from
 * outside this module.
 *
 * They invoke "pio" which eventually leads to a call to "ptrace".
 * The system generates an I/O error when a ptrace fails.  During reads
 * these are ignored, for writes they are reported as an error and for
 * anything else they cause a fatal error.
 */

extern Intfunc *onsyserr();

private badaddr;
private read_err(), write_err();

/*
 * Read from the process' instruction area.
 */

public iread(buff, addr, nbytes)
char *buff;
Address addr;
int nbytes;
{
    Intfunc *f;

    f = onsyserr(EIO, read_err);
    badaddr = addr;
    if (coredump) {
	coredump_readtext(buff, addr, nbytes);
    } else {
	pio(process, PREAD, TEXTSEG, buff, addr, nbytes);
    }
    onsyserr(EIO, f);
}

/*
 * Write to the process' instruction area, usually in order to set
 * or unset a breakpoint.
 */

public iwrite(buff, addr, nbytes)
char *buff;
Address addr;
int nbytes;
{
    Intfunc *f;

    if (coredump) {
	error("no process to write to");
    }
    f = onsyserr(EIO, write_err);
    badaddr = addr;
    pio(process, PWRITE, TEXTSEG, buff, addr, nbytes);
    onsyserr(EIO, f);
}

/*
 * Read for the process' data area.
 */

public dread(buff, addr, nbytes)
char *buff;
Address addr;
int nbytes;
{
    Intfunc *f;

    read_err_flag = 0;
    f = onsyserr(EIO, read_err);
    badaddr = addr;
    if (coredump) {
	coredump_readdata(buff, addr, nbytes);
    } else {
	pio(process, PREAD, DATASEG, buff, addr, nbytes);
    }
    onsyserr(EIO, f);
}

/*
 * Write to the process' data area.
 */

public dwrite(buff, addr, nbytes)
char *buff;
Address addr;
int nbytes;
{
    Intfunc *f;

    if (coredump) {
	error("no process to write to");
    }
    f = onsyserr(EIO, write_err);
    badaddr = addr;
    pio(process, PWRITE, DATASEG, buff, addr, nbytes);
    onsyserr(EIO, f);
}

/*
 * Trap for errors in reading or writing to a process.
 * The current approach is to "ignore" both read and write errors.
 * (See comment above.)
 */

private read_err()
{
    read_err_flag = 1;
}

private write_err()
{
    printf("can't write to process (address 0x%x)\n", badaddr);
    error("May be unwriteable address, unwriteable process, etc (see ptrace(2))");
}

/*
 * Ptrace interface.
 */

/*
 * This magic macro enables us to look at the process' registers
 * in its user structure.
 */



#ifdef vax

#define regloc(reg)     (ar0 + ( sizeof(int) * (reg) ))
#define ar0 ctob(UPAGES)

#endif

#define WMASK           (~(sizeof(Word) - 1))
#define cachehash(addr) ((unsigned) ((addr >> 2) % ICACHE_SIZE))

#define FIRSTSIG        SIGINT
#define LASTSIG         SIGQUIT
#define ischild(pid)    ((pid) == 0)
#define traceme()       ptrace(0, 0, 0, 0, 0)
#define setrep(n)       (1 << ((n)-1))
#define istraced(p)     (p->sigset&setrep(p->signo))

public kill_process()
{
    if (process != nil and process->pid != 0) {
	ptrace(PTRACE_KILL, process->pid, 0, 0, 0);
	process = nil;
    }
}

/*
 * Set the process free
 */
public detach_process(p)
Process p;
{
    if (p == nil) {
	error("No active process to detach");
    }
    ptrace(PTRACE_DETACH, p->pid, p->reg.r_pc, p->signo, 0);
    p->pid = 0;
}

/*
 * Start up a new process by forking and exec-ing the
 * given argument list, returning when the process is loaded
 * and ready to execute.  The PROCESS information (pointed to
 * by the first argument) is appropriately filled.
 *
 * If the given PROCESS structure is associated with an already running
 * process, we terminate it.
 */

/* VARARGS2 */
private pstart(p, args, filein, fileout, fileapp)
Process p;
String args[];
String filein;
String fileout;
String fileapp;
{
    int status;
    Fileid in, out;
    int	i;

    if (p->pid != 0) {			/* child already running? */
	ptrace(PTRACE_KILL, p->pid, 0, 0, 0);	/* ... kill it! */
	/* pwait(p->pid, &status);  See Bugs_fixed3, #107 */
    }
    psigtrace(p, SIGTRAP, true);
    p->pid = vfork();
    if (p->pid == -1) {
	panic("can't fork");
    }
    if (ischild(p->pid)) {

	traceme();
	if (filein != nil) {
	    in = open(filein, 0);
	    if (in == -1) {
		write(2, "can't read ", 11);
		write(2, filein, strlen(filein));
		write(2, "\n", 1);
		_exit(1);
	    }
	    fswap(0, in);
	}
	if (fileout != nil) {
	    out = create_out(fileout);
	    fswap(1, out);
	}
	if (fileapp != nil) {
	    out = open(fileapp, O_WRONLY | O_APPEND);
	    if (out == -1) {
		out = create_out(fileapp);
	    }
	    fswap(1, out);
	}
	for (i = 3; i < getdtablesize(); i++) {
		ioctl(i, FIOCLEX, 0);
	}
	nocerror = true;
	execv(args[0], args);
	write(2, "can't exec ", 11);
	write(2, args[0], strlen(args[0]));
	write(2, "\n", 1);
	_exit(1);
    }
    nocerror = false;
    pwait(p->pid, &status);
    getinfo(p, status);
    if (p->status != STOPPED) {
	error("program could not begin execution");
    }
    if (use_shlib){
	int o = 1;
	read_ld_debug();
	iwrite(&o, ld_debug_addr+IN_DEBUGGER, sizeof(o));
    }
}

/*
 * Set up the standard output for the child.
 */
private create_out(file)
String file;
{
    int out;

    out = open(file, O_WRONLY | O_CREAT, 0666);
    if (out == -1) {
	write(2, "can't write ", 12);
	write(2, file, strlen(file));
	write(2, "\n", 1);
	_exit(1);
    }
    return(out);
}

/*
 * Continue a stopped process.  The first argument points to a Process
 * structure.  Before the process is restarted, its user area is modified
 * according to the values in the structure.  When this routine finishes,
 * the structure has the new values from the process's user area.
 *
 * Pcont terminates when the process stops with a signal pending that
 * is being traced (via psigtrace), or when the process terminates.
 *
 * If the user gave a "cont at line_number" command (i.e., the pc has
 * changed), we must give the new pc explicitly; otherwise, we *must*
 * give a "1" in order to prevent the kernel from setting npc to pc+4.
 */

private pcont(p, signo)
Process p;
int signo;
{
    int status;
    Address ptrace_pc;

    if (p->pid == 0) {
	error("program not active");
    }
    do {
	setinfo(p, signo);
	sigs_off();

	ptrace_pc = ( p->pc_changed ? p->reg.r_pc : 1 );
	if (ptrace(PTRACE_CONT, p->pid, ptrace_pc, p->signo, 0) < 0) {
	    panic("error %d trying to continue process", errno);
	}
	pwait(p->pid, &status);

	sigs_on();
	getinfo(p, status);
    } while (p->status == STOPPED and not istraced(p));

    if (runfirst and p->status != STOPPED) {
	if (p->signo == 0 and p->pid != 0) {
	    ptrace(PTRACE_KILL, p->pid, 0, 0, 0);	    /* kill process */
	    printf("program terminated successfully with exit value %d\n",
		p->exitval);
	    /* p->pid = 0; */	/* uncomment if not quitting */
	    quit(0);
	}
    }
}

/*
 * Single step as best ptrace can.  This one we must simulate on
 * the sparc, because the hardware can't single-step at all.
 */
public pstep(p)
Process p;
{
    int status;

    do {
        setinfo(p, 0);
        sigs_off();

	/* The single-stepping ptrace simulation lives in machine.c */
        status = dbx_ptrace(PTRACE_SINGLESTEP, p->pid, p->reg.r_pc, p->signo, 0);

        sigs_on();
        getinfo(p, status);
    } while (p->status == STOPPED and not istraced(p));
}

/*
 * Return from execution when the given signal is pending.
 */

public psigtrace(p, sig, sw)
Process p;
int sig;
Boolean sw;
{
    if (sw) {
	p->sigset |= setrep(sig);
    } else {
	p->sigset &= ~setrep(sig);
    }
}

/*
 * If the child is in another process group, ptrace will not always inform dbx
 * (via "wait") when a signal typed at the terminal has occurred.  (i.e.
 * if the controlling tty for the child - which is the same as dbx's ctrolling
 * tty in most cases - is different from the process group of the child).
 * Thus, dbx should catch the signal and if the child is in the same process
 * group, simply return (ptrace will inform dbx of the signal via "wait").
 * If the child is in a different process group, dbx should issue a "kill"
 * of that signal to the child.  (This will cause ptrace to "wake up" and
 * inform dbx that the child received a signal.)
 */

private struct sigvec sigfunc[NSIG];

private sigs_off()
{
    register int i;
    struct sigvec sigv;

    sigv.sv_mask = 0;
    sigv.sv_onstack = 0;
    for (i = FIRSTSIG; i <= LASTSIG; i++) {
	if (i != SIGKILL) {
	    if (i == SIGINT) {
		sigv.sv_handler = sigint_handler;
	    	sigvec(i, &sigv, &sigfunc[i]);
	    } else if (i == SIGQUIT) {
		sigv.sv_handler = sigquit_handler;
	    	sigvec(i, &sigv, &sigfunc[i]);
	    } else {
		sigv.sv_handler = SIG_IGN;
	    	sigvec(i, &sigv, &sigfunc[i]);
	    }
	}
    }
}

/*
 * Turn back on attention to signals.
 */

private sigs_on()
{
    register int i;

    for (i = FIRSTSIG; i <= LASTSIG; i++) {
	if (i != SIGKILL) {
	    sigvec(i, &sigfunc[i], 0);
	}
    }
}

/*
 * Get process information from user area.
 */

#ifdef vax
private int rloc[] ={
    R0, R1, R2, R3, R4, R5, R6, R7,
    R8, R9, R10, R11, AP, FRP, SP, PC
};
#endif

private peekusercode(pid)
{int code;
 Boolean nocerr = nocerror;
 nocerror = true;
 errno = 0;
 code = ptrace(PTRACE_PEEKUSER, pid, &((struct user *) 0)->u_code, 0); 

/* the above line, if compiled on 4.0 will set errno when run on 4.1 */
/* this little kludge is here to insure that dbx compiled
 * under 4.0 will still run under a 4.1 kernel, and vice versa.
 * pre-4.0.3 dbx did not look for u_code and so are already portable.
 * see bug: [1028504]
 * if u_code should change again, you must recompute this address,
 * we allow this hack because we believe dbx will not live past 4.1
 */
#ifndef UCODE_4_1
#define UCODE_4_1 0x4d0
#endif  UCODE_4_1
 if (errno) {
     errno = 0;
     code = ptrace(PTRACE_PEEKUSER, pid, UCODE_4_1, 0); 
 }
 nocerror = nocerr;
 return code;
}

private getinfo(p, status)
register Process p;
register int status;
{
    register int i;
    int keybd_flag;

    p->signo = (status&0177);
    p->exitval = ((status >> 8)&0377);
    if (p->signo != STOPPED) {
	p->status = FINISHED;
    } else {
	p->status = p->signo;
	p->signo = p->exitval;
	p->sigcode = peekusercode(p->pid);
	p->exitval = 0;
	ptrace(PTRACE_GETREGS, p->pid, &p->reg, 0, 0);
	ptrace(PTRACE_GETFPREGS, p->pid, &p->rfpu, 0, 0);
	p->oreg = p->reg;
    }
    p->pc_changed = 0;

    savetty(&(p->ttyinfo));
    if (save_keybd) {
        ioctl(keybd_fd, KIOCGTRANS, &keybd_xlation);
	if (keybd_xlation == 0) {
 	    keybd_flag = 1;			/* Normal keybd operation */
            ioctl(keybd_fd, KIOCTRANS, &keybd_flag);
	}
    }
}

/*
 * Set process's user area information from given process structure.
 */

private setinfo(p, signo)
register Process p;
int signo;
{
    register int i;
    register int r;

    if (istraced(p)) {
	p->signo = signo;
    }
    if (bcmp((char *) &p->reg, (char *) &p->oreg, sizeof(p->reg))) {
	ptrace(PTRACE_SETREGS, p->pid, &p->reg, 0, 0);
    }
	ptrace(PTRACE_SETFPREGS, p->pid, &p->rfpu, 0, 0);
    restoretty(&(p->ttyinfo));
    if (save_keybd) {
        ioctl(keybd_fd, KIOCTRANS, &keybd_xlation);
    }
}

/*
 * Structure for reading and writing by words, but dealing with bytes.
 */

typedef union {
    Word pword;
    Byte pbyte[sizeof(Word)];
} Pword;

/*
 * Read (write) from (to) the process' address space.
 * We must deal with ptrace's inability to look anywhere other
 * than at a word boundary.
 */

private Word fetch();
private store();

private pio(p, op, seg, buff, addr, nbytes)
Process p;
PioOp op;
PioSeg seg;
char *buff;
Address addr;
int nbytes;
{
    register int i;
    register Address newaddr;
    register char *cp;
    char *bufend;
    Pword w;
    Address wordaddr;
    int byteoff;

    if (p == nil) {
	error("no active process");
    }
    if (p->status != STOPPED) {
	if (p->status == NOTSTARTED  and  reading_to_attach == true) {
	    /*
	     * Ignore the error this time -- it's from trying to find the
	     * beginning of a function when we're "attach"ing to a
	     * process -- we're reading the object but haven't grabbed
	     * the process yet, so pio will fail.  In that case, we must
	     * run "findbeginning" on all functions after we do grab the
	     * child.
	     */
	    return;
	}
	if (newobject != true) {
	    error("program is not active");
	}
    }
    cp = buff;
    newaddr = addr;
    wordaddr = (newaddr&WMASK);
    if (wordaddr != newaddr) {
	w.pword = fetch(p, seg, wordaddr);
	for (i = newaddr - wordaddr; i < sizeof(Word) and nbytes > 0; i++) {
	    if (op == PREAD) {
		*cp++ = w.pbyte[i];
	    } else {
		w.pbyte[i] = *cp++;
	    }
	    nbytes--;
	}
	if (op == PWRITE) {
	    store(p, seg, wordaddr, w.pword);
	}
	newaddr = wordaddr + sizeof(Word);
    }
    byteoff = (nbytes&(~WMASK));
    nbytes -= byteoff;
    if (nbytes > 0) {
	if (op == PREAD) {
	    if (seg == DATASEG) {
	        ptrace(PTRACE_READDATA, p->pid, newaddr, nbytes, cp);
	    } else {
	        ptrace(PTRACE_READTEXT, p->pid, newaddr, nbytes, cp);
	    }
	} else {
	    if (seg == DATASEG) {
		ptrace(PTRACE_WRITEDATA, p->pid, newaddr, nbytes, cp);
	    } else {
		ptrace(PTRACE_WRITETEXT, p->pid, newaddr, nbytes, cp);
	    }
	}
	cp += nbytes;
	newaddr += nbytes;
    }
    if (byteoff > 0) {
	w.pword = fetch(p, seg, newaddr);
	for (i = 0; i < byteoff; i++) {
	    if (op == PREAD) {
		*cp++ = w.pbyte[i];
	    } else {
		w.pbyte[i] = *cp++;
	    }
	}
	if (op == PWRITE) {
	    store(p, seg, newaddr, w.pword);
	}
    }
}

/*
 * Get a word from a process at the given address.
 * The address is assumed to be on a word boundary.
 *
 * A simple cache scheme is used to avoid redundant ptrace calls
 * to the instruction space since it is assumed to be pure.
 *
 * It is necessary to use a write-through scheme so that
 * breakpoints right next to each other don't interfere.
 */

private Integer nfetchs, nreads, nwrites;

private Word fetch(p, seg, addr)
Process p;
PioSeg seg;
register int addr;
{
    register CacheWord *wp;
    register Word w;
    extern Integer use_shlib;

    switch (seg) {
	case TEXTSEG:
	    if (use_shlib) {
		w = ptrace(PTRACE_PEEKTEXT, p->pid, addr, 0, 0);
	    }
	    else {
		++nfetchs;
		wp = &p->word[cachehash(addr)];
		if (addr == 0 or wp->addr != addr) {
		    ++nreads;
		    w = ptrace(PTRACE_PEEKTEXT, p->pid, addr, 0, 0);
		    wp->addr = addr;
		    wp->val = w;
		} else {
		    w = wp->val;
	    	}
	    }
	    break;

	case DATASEG:
	    w = ptrace(PTRACE_PEEKDATA, p->pid, addr, 0, 0);
	    break;

	default:
	    panic("fetch: bad seg %d", seg);
	    /* NOTREACHED */
    }
    return w;
}

/*
 * Put a word into the process' address space at the given address.
 * The address is assumed to be on a word boundary.
 */

private store(p, seg, addr, data)
Process p;
PioSeg seg;
int addr;
Word data;
{
    register CacheWord *wp;

    switch (seg) {
	case TEXTSEG:
	    ++nwrites;
	    wp = &p->word[cachehash(addr)];
	    wp->addr = addr;
	    wp->val = data;
	    ptrace(PTRACE_POKETEXT, p->pid, addr, data, 0);
	    break;

	case DATASEG:
	    /* in new VM, never POKEDATA */
	    ptrace(PTRACE_POKETEXT, p->pid, addr, data, 0);
	    break;

	default:
	    panic("store: bad seg %d", seg);
	    /* NOTREACHED */
    }
}

/*
 * Swap file numbers so as to redirect standard input and output.
 */

private fswap(oldfd, newfd)
int oldfd;
int newfd;
{
    if (oldfd != newfd) {
	close(oldfd);
	dup(newfd);
	close(newfd);
    }
}

/*
 * Print the list of signals being ignored
 */
public	pr_ignore(p)
Process p;
{
    int sig;
    Boolean first;

    first = true;
    for (sig = 1; sig <= SIGWINCH; sig++) {
	if (not (p->sigset & setrep(sig))) {
	    if (first) {
		first = false;
		if (isredirected()) {
		    printf("ignore ");
		}
	    }
	    printf("%s ", signo_to_name(sig));
	}
    }
    printf("\n");
}

/*
 * Print the list of signals being caught
 */
public	pr_catch(p)
Process p;
{
    int sig;
    Boolean first;

    first = true;
    for (sig = 1; sig <= SIGWINCH; sig++) {
	if (p->sigset & setrep(sig)) {
	    if (first) {
		first = false;
		if (isredirected()) {
		    printf("catch ");
		}
	    }
	    printf("%s ", signo_to_name(sig));
	}
    }
    printf("\n");
}

/*
 * Print the current command line.
 */
public pr_cmdline(quote, ix)
Boolean quote;
int	ix;
{
    int i;

    for (i = ix; i < argc; i++) {
	if (quote or index(argv[i], ' ')) {
	    printf("\"%s\" ", argv[i]);
	} else {
	    printf("%s ", argv[i]);
	}
    }
    if (infile != nil) {
	printf("< %s ", infile);
    }
    if (outfile != nil) {
	printf("> %s ", outfile);
    }
    if (appfile != nil) {
	printf(">> %s ", appfile);
    }
    printf("\n");
    fflush(stdout);
}

/*
 * Can the process be continued?
 * The process must be stopped, there must be a process and
 * the current reason for stopping can not be any sort of
 * addressing error.
 */
public can_continue()
{
    if (not isstopped or
	process == nil or
	process->signo == SIGILL or
	process->signo == SIGBUS or
	process->signo == SIGSEGV) {
	error("can't continue execution");
    }
}

/*
 * Grab hold of a non-child process.
 */
public grab_nonchild(pid)
int pid;
{
    int status;
    int r;

    nocerror = true;
    r = ptrace(PTRACE_ATTACH, pid, 0, 0, 0);
    nocerror = false;
    if (r == -1) {
	error("Could not attach to process %d", pid);
    }
    process->pid = pid;
    pwait(pid, &status);
    getinfo(process, status);
    pc = process->reg.r_pc;
    process->signo = 0;
    isstopped = true;
}

/*
 * Something went wrong during initialization.  We may have received
 * a SIGINT or there may have been an error in reading the symbols.
 * Anyway, here we wipe the slate clean so as to leave dbx up
 * but without any context.
 */
public clean_slate()
{
    process = nil;
    objname = nil;
    argv[0] = nil;
    argc = 0;
}

public process_is_stopped()
{
	return (process->status == STOPPED);
}
