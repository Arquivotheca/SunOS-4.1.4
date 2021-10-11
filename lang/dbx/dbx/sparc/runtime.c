#ifndef lint
static	char sccsid[] = "@(#)runtime.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 Sun Microsystems, Inc.
 */

/*
 * Runtime organization dependent routines, mostly dealing with
 * activation records.
 */

#include "defs.h"
#include "display.h"
#include "runtime.h"
#include "process.h"
#include "machine.h"
#include "dbxmain.h"
#include "events.h"
#include "mappings.h"
#include "symbols.h"
#include "tree.h"
#include "eval.h"
#include "operators.h"
#include "object.h"
#include "kernel.h"
#include "languages.h"
#include <signal.h>

#ifndef public
typedef struct Frame *Frame;
public Boolean batchcmd;

#include "machine.h"
/*
 * Values returned by setjmp/longjmp.
 */
enum	Setjmp	{
	SETJMP = 0,			/* Value returned by setjmp */
	CMD_LEVEL,			/* Return to command level */
	CALL_RETURN,			/* 'call' command has returned */
};
typedef enum Setjmp Setjmp;

#endif

#define NSAVEREG 16
#define SSAVEREG (NSAVEREG*4)

#define NLOCALREG 8
#define NINREG    8
#define SAVEREG_BASE REG_L0

/*
 * MAX_PROLOG_LEN tells how far "findbeginning" should search after
 * the actual start of a function, before deciding that the function
 * has no "save" instruction.  Gotta watch out for "trap"s, too.
 * ( "_exit" is really short. )
 */
#define MAX_PROLOG_LEN 12
#define MAX_PROLOG_WORDS (MAX_PROLOG_LEN/sizeof (Word))


/*
 * 
 *	struct real_Frame  : the fixed-size header of each activation record
 *
 * The order of (most of the) fields in this data structure is important
 * since it matches how activation records are stored by the compiler:
 *
 *	  sp ->	local regs	\__ a "struct rwindow"
 *	     -> in regs		/
 *	     -> hidden arg for struct-valued functions (address of the
 *			struct, which the caller has to allocate)
 *	     -> area where the callee can save register arguments,
 *		e.g., because the user took their address.
 *
 * After the regArgsDump point in the stack frame, we have:
 *	     -> outgoing parameters past the sixth, if any;
 *	     -> compiler temps and saved FP regs,
 *	     -> dynamically allocatable area (does alloca work?),
 *	     -> and finally, addressable automatics (either those that
 *		explicitly must be addressable, or we ran out of "L"
 *		registers).
 *	  fp -> old sp
 * All of these are optional, so they're not given fields here.
 */

#define NREGS_IN_FRAME	6
#define NARG_REGS	6

typedef struct {
	Word	args_save[ NREGS_IN_FRAME ];
} Reg_args;

struct real_Frame {
    struct rwindow  fr_w;		/* ins & locals */
    Address	    fr_struct_arg;	/* hidden arg for struct return */
    Reg_args	    fr_reg_args;
};

#define MIN_FRAME_SIZE sizeof(struct real_Frame)
#define ARG_REGS_SIZE sizeof(Reg_args)

/*
 * Find out how many words of arguments were passed to
 * the current procedure.
 */
#define findnumargs(frp) (frp->nargwords = NARG_REGS)

/*
 * struct Frame:
 *
 * this structure is dbx's description of an activation record
 * we keep a pointer to the start of real_Frame in the debuggee's
 * address space and the values of sundry fields
 */

struct Frame {
	struct	real_Frame *header; /* in the debuggee's address space*/
    /* Other fields are not really in the frame */
    Address	    active_pc;  /* pc when this frame is active */
    Address	    active_sp;  /* sp when this frame is active */
    Address	    active_fp;  
    Address	    caller;		/* return address */

    Boolean	    is_leaf;	/* true when this is a leaf proc */
    Boolean	    tramp_caller; /* " " this proc was called by _sigtramp */
    int		    nargwords;	/* number of words of arguments */
};

#define save_fp(frp)   &(frp->header->fr_w.rw_in[6])  /* "fp" == i6 */
#define rtn_addr_reg(frp)  &(frp->header->fr_w.rw_in[7]) 



/*
 * frame_read is a macro to "dread" from a stack frame:
 *	sp_fptr is a SubProcess Frame PoinTeR;
 *	field is a field ("member") of that struct, and
 *	dbx_dest is the destination (in dbx' address space) where the
 *		data will be deposited.
 * The size is determined by the size of "field".
 *
 * frame_field is a helper macro.
 */

#define frame_field(var,field) ( ((struct real_Frame *)(var))->field )

#define frame_whole(dbx_fptr,sp_fptr)	\
	dread( (dbx_fptr), sp_fptr, MIN_FRAME_SIZE )

#define frame_read(dbx_dest,sp_fptr,field)	\
	dread( &(dbx_dest),			\
		& frame_field(sp_fptr,field),	\
		sizeof( frame_field(sp_fptr,field) ) )

private Frame	curframe;		/* Ptr to current frame */
private struct	Frame 	curframerec;	/* Buffer for current frame */
private int	dbxcalls;		/* # of debugger "calls" found */
private Boolean walkingstack = false;

#define frameeq(f1, f2) ((f1)->active_pc == (f2)->active_pc)

extern Boolean nocerror;

private Boolean printcall();
private Boolean find_some_source1();
private Boolean is_leaf_proc( /* Frame *, current_o7 */ );

/*
 * Set a frame to the current activation record.
 */

private getcurframe(frp)
register Frame frp;
{
    register int i;

    checkref(frp);

    frp->active_pc = reg(REG_PC);
    frp->active_sp = reg(REG_SP);
    frp->active_fp = reg(REG_FP);

	frp->header = (struct real_Frame*) frp->active_sp;
    frp->is_leaf = is_leaf_proc( frp->active_pc, reg(REG_O7), frp->active_sp );
    if( frp->is_leaf ) {
		frp->caller = reg( REG_O7 );
    } else {
		frp->caller = reg( REG_I7 );
	}
    frp->tramp_caller = (Boolean) in_caught_sig_rtn(frp->caller);

    set_nargwords( frp );
}

private set_nargwords ( frp )
Frame frp;
{
    if (frp->active_fp == nil  or  frp->is_leaf) {
	frp->nargwords = 0;
    } else if( frp->tramp_caller ) {
	frp->nargwords = 3;
    } else {
	frp->nargwords = 6;
    }
}

/*
 * Return a pointer to the next activation record up the stack.
 * Return nil if there is none.
 * Writes over space pointed to by given argument.
 *
 * The pc in the stack frame for a caught signal routine is incorrect;
 * the correct pc is retrieved by first getting the pointer which is
 * the third parameter to the caught signal routine.  This is a
 * pointer to struct sigcontext (in signal.h).  Then take the pc in
 * that structure and you've got it.  sigtramp is the name of the
 * library routine that calls the caught signal routine.
 */

private Frame nextframe(frp)
register Frame frp;
{
    Address caller_sp;

    if (frp->active_fp == nil) {
	frp = nil;
    } else {

	if(frp->is_leaf) {
		caller_sp = frp->active_sp;
	} else {
		caller_sp = frp->active_fp;
	}

	if (caller_sp == nil or
	  (kernel and not kernel_frame(caller_sp))) {
	    frp = nil;
	} else {
	    if (in_caught_sig_rtn(frp->active_pc)) {
		fix_caught_sig_frame(frp);
	    } else {
		frp->active_pc = frp->caller;
		frp->is_leaf = false;
		extractframeinfo(frp, caller_sp);
	    }
	}
	if( frp->active_fp == nil ) {
	    frp = nil;
	}
    }
    return frp;
}

/*
 * Finding the saved registers and number of arguments passed to
 * the current procedure is painful for the 68000; the saved registers
 * are easy (always there) on the sparc, but the number of arguments
 * is impossible.
 */


private extractframeinfo(newfrp, caller_sp)
Frame newfrp;
Address caller_sp;
{
	int i;
    findnumargs(newfrp);
   	newfrp->active_sp = caller_sp;
   	newfrp->header = (struct real_Frame*) caller_sp;

	dread(&(newfrp->active_fp), save_fp(newfrp), sizeof(newfrp->active_fp));
	if (read_err_flag) {
		error("attempt to read stack failed - bad frame pointer");
	}

	if(newfrp->active_fp) {	
		/* assume a non-leaf routine */
		dread(&(newfrp->caller), rtn_addr_reg(newfrp), sizeof(newfrp->caller));
		if (read_err_flag) {
			error("attempt to read stack failed - bad frame pointer");
		}
    	newfrp->tramp_caller = (Boolean) in_caught_sig_rtn(newfrp->caller);
    	set_nargwords( newfrp );
	}
}

/*
 * If nextframe decides we're in _sigtramp, we need to fix the frame
 * of the routine that "called" _sigtramp.
 * It's similar to getcurframe in that we need to check again whether
 * the signal interrupted a leaf proc.
 */
private fix_caught_sig_frame(frp)
Frame frp;
{
    struct sigcontext *scp; /* addr in the subprocess.  Don't dereference! */
			/* values of fp,sp and i7 when sigtramp was executing */
    Address sig_fp, sig_sp, sig_i7; 
	struct real_Frame frame_header;
	int i;


	sig_fp = frp->active_fp;
	sig_sp = frp->active_sp;
	/* when the kernel passes control to _sigtramp, it sets up sp to look
	** as if the function executing at the time the signal was delivered
	** had called _sigtramp, passing it signo, sigcode, and a pointer
	** to sigcontext. _sigtramp first does a save to get a new window
	** (see comments in /usr/src/lib/libc/sys/common/sparc/sigtramp.s
	*/
	dread(&frame_header, sig_fp, sizeof(frame_header));
	if (read_err_flag) {
		error("attempt to read stack failed - bad frame pointer");
	}

    /*
     * Get the sigcontext pointer (_sigtramp's third parameter).
     */

    /* (sigtramp was broken on the older machines). */
    /* (I don't know whether this'll work on Xenon yet.)  */
#ifdef NEW_SIGTRAMP
    scp = (struct sigcontext *) frame_header.fr_reg_args.args_save[2] ;
#else
    scp = (struct sigcontext *) frame_header.fr_reg_args.args_save[1] ;
#endif

    /*
     * Find out where the signal occurred.  (scp->sc_pc)
     *      (Get the pc, sp and o7 (see above) at that point.)
     */
    dread( &( frp->active_pc ), &(scp->sc_pc), sizeof(Word));
    dread( &( frp->active_sp ), &(scp->sc_sp), sizeof(Word));

					/* fp of interrupted routine is in its i6 */
	dread(&(frp->active_fp), save_fp(frp), sizeof(frp->active_fp));
	if (read_err_flag) {
		error("attempt to read stack failed - bad frame pointer");
	}

	/* get o7 value at the time the signal was delivered : since
	** sigtramp did a save, this value is in sigtramp's i7
	*/
	dread(&frame_header, sig_sp, sizeof(frame_header));
	if (read_err_flag) {
		error("attempt to read stack failed - bad frame pointer");
	}
    sig_i7 = frame_header.fr_w.rw_in[7];

    frp->is_leaf = is_leaf_proc( frp->active_pc, sig_i7, frp->active_sp );
    if( frp->is_leaf ) {
		frp->caller = sig_i7;
    } else {
		/* if a non-leaf was interrupted the return value is in its i7 */
		dread(&(frp->caller), rtn_addr_reg(frp), sizeof(frp->caller));
		if (read_err_flag) {
			error("attempt to read stack failed - bad frame pointer");
		}
    }
}


/*
 * Get the current frame information in the given Frame and store the
 * associated function in the given value-result parameter.
 */

private getcurfunc (frp, fp)
Frame frp;
Symbol *fp;
{
    getcurframe(frp);
    *fp = whatblock(frp->active_pc, true);
}


/*
 * Get value of a register variable.
 * On sparc, register variables are always "stored",
 * so there's no need for an excursion up the stack,
 * except the one done by symframe, which finds the
 * closest frame that contains a symbol that matches s.
 */

public findregvar(s, frp)
Symbol s;
Frame frp;
{
    register Frame lfrp;
    int value;
    register int r;
    struct Frame frame;
	Address addr;

    if (!isreg(s)) {
	panic("Non-register variable in findregvar()");
    }
    if (frp == nil) {
	/* NYI -- shouldn't do this if it's a "real" register name! */
	frp = symframe(s);
    }
    r = s->symvalue.offset;
	addr = findregaddr(frp, r);
	if(addr <= (Address) LASTREGNUM) {
    	value = reg(addr);
	} else {
    	dread( &value, addr, sizeof(int));
	}
    return value;
}

/*
 * Given a symbol, find the frame that contains it.
 * Check the current frame first (in case an 'up', 'down', or
 * 'func' command has changed it) and, if that does not match,
 * use findframe() to search from the current stopping point
 * until the begining of the program.
 */
public Frame symframe(s)
Symbol s;
{
    Symbol cur;
    Frame frp;

    cur = s->block;
    while (cur != nil and cur->class == MODULE) {
	cur = cur->block;
    }
    if (cur == nil) {
	cur = whatblock(pc, true);
    }
    frp = curfuncframe();
    if (frp == nil or cur != whatblock(frp->active_pc, true)) {
        frp = findframe(cur);
        if (frp == nil) {
	    error("Could not find a stack frame for `%s'", symname(s));
        }
    }
    return(frp);
}


/*
 * Find the address of a saved general purpose register.
 */
public Address findregaddr(frp, r)
Frame frp;
int r;
{
    Address addr;

	if(! IS_WINDOW_REG(r) ) {
		addr = (Address) r;
	} else if( r >= REG_L0 and r <= REG_L7 ) {
		addr = (Address) &(frp->header->fr_w.rw_local[r - REG_L0]);
	} else if( r >= REG_I0 and r <= REG_I7 ) {
		addr = (Address) &(frp->header->fr_w.rw_in[r - REG_I0]);
	}
    return addr;
}


/*
 * Return the frame associated with the given function.
 * If the function is nil, return the most recently activated frame.
 *
 * Static allocation for the frame.
 */

public Frame findframe(f)
Symbol f;
{
    register Frame frp;
    static struct Frame frame;
    Symbol p;
    Boolean done;
    int save_dbxcalls;

    frp = &frame;
    save_dbxcalls = dbxcalls;
    dbxcalls = 0;
    getcurframe(frp);
    if (f != nil) {
	done = false;
	p = whatblock(frp->active_pc, false);
	do {
		/*
		 * In Fortran, stack frame "main" <==> stack frame "program"
		 * In Fortran, the last frame is really "main", not
		 * the program name since main is defined in the ST
		 * and a jmp is done to there (after 0x8000) followed
		 * by a jsr to MAIN.  This leaves the old pc pointing
		 * into "main", not "program".  Note that "main" is
		 * not defined in the block linkage leading to an
		 * identifier -- therefore a trace back from an identifier
		 * will end with "program" rather than "main".
		 *
		 * The same is true for Pascal.
		 */
	    if (p == f or (is_pas_fort_main_rtn(p) and f == program)) {
		done = true;
	    } else if (p == program or is_pas_fort_main_rtn(p)) {
		done = true;
		frp = nil;
	    } else {
		while ((done == false) and isinline(p)) {
		    p = container(p);
		    if (p == f) {
			done = true;
		    } else if (p == program) {
			done = true;
			frp = nil;
		    }
		}
		if (done == false) {
		    frp = nextfunc(frp, &p);
		    if (frp == nil) {
		        done = true;
		    }
		}
	    }
	} while (not done);
    }
    dbxcalls = save_dbxcalls;
    return frp;
}

	/* get the value of a floating point rergister. Doing this right requires
	** knowing where the compiler has saved it - not to be fixed for 4.0beta
	** will it ever ?
	*/
public find_fpureg(s, frp, dp)
Symbol s;
Frame frp;
double *dp;
{
	Frame lfrp;
	struct Frame frame;
	double d;

	if (frp == nil) {
		frp = symframe(s);
	}
	lfrp = &frame;
	getcurframe(lfrp);
	if(!frameeq(frp, lfrp)) {
		error("cannot acccess an fpu register outside the current procedure");
	}
	if( size(s) == sizeof(float)) {
		d = *((float*) regaddr(s->symvalue.offset));
	} else {
		d = *((double*) regaddr(s->symvalue.offset));
	}
	*dp = d;
}

public Boolean isfpureg(s) 
Symbol s;
{
	return ((Boolean) (isreg(s) && 
		(s->symvalue.offset >= REG_F0) ) && (s->symvalue.offset <= REG_F31) );
}


/*
 * Find the return address of the current procedure/function.  Since
 * we are single stepping, the unlink has already occurred and the sp
 * points to the return address.  Just get *sp.
 *
 * This is wrong for sparc but I do not yet understand why it's used
 * in the places where it's used.  (I.e., as the first parameter in
 * "rpush" calls, in "`eval.c`eval`case O_CALL:" and `printsym.c`printrtn).
 */

public Address ss_return_addr()
{
    Address retaddr;

    dread(&retaddr, reg(REG_SP), sizeof(retaddr));
    if (read_err_flag) {
        error("attempt to read stack failed - bad stack pointer");
    }
    return(retaddr);
}

/*
 * Find the return address of the current procedure/function.  Since
 * we have just entered the routine and the save is already done, the
 * return address is the contents of %i7 + 8.
 */

public Address fp_return_addr()
{
    return( reg( REG_I7 ) + 8 );
}

/*
 * Find the return address of the current procedure/function.
 */

public Address return_addr()
{
    Frame frp;
    Address addr;
    struct Frame frame;

    frp = &frame;
    getcurframe(frp);
    frp = nextframe(frp);
    if (frp == nil) {
	addr = 0;
    } else {
	addr = frp->active_pc +8;
    }
    return addr;
}

/*
 * Push the value associated with the current function.
 */

public pushretval(len, isindirect, ret_type, retreg)
Integer len;
Boolean isindirect;
Symbol ret_type;	/* the type that the function returns */
Word *retreg;
{
    float reg_f0, reg_f1;
    Address rpa, struct_return_address();

    if (isindirect) {
	if( ret_type->class == RECORD ) {
	    rpa = struct_return_address();
	} else {
	    rpa = (Address) retreg[0];
	}
	rpush( rpa, len );
    } else {
	/*
	 * On sparc, floating results are in FPU registers,
	 * not in the normal (IU) ones.
	 */
	if( isfloat( ret_type ) ) {
	    reg_f0 = reg( REG_F0 );
	    push( Word, reg_f0 );

	    if( len > sizeof(float) ) {
		reg_f1 = reg( REG_F1 );
		push( Word, reg_f1 );
	    }
	} else switch (len) {
	    case sizeof(char):
		push(char, retreg[0]);
		break;

	    case sizeof(short):
		push(short, retreg[0]);
		break;

	    default:
		if (len == sizeof(Word)) {
		    push(Word, retreg[0]);
		} else if (len == 2*sizeof(Word)) {
		    push(Word, retreg[0]);
		    push(Word, retreg[1]);
		} else {
		    panic("not indirect in pushretval?");
		}
		break;
	}
    }
    /* Convert to double if appropriate */
    push_double(ret_type, len);
}

/*
 * Return the base address for locals in the given frame.
 */

public Address locals_base(frp)
register Frame frp;
{
    return (frp == nil) ? reg(REG_FP) : frp->active_fp;
}

/*
 * Return the base address for arguments in the given frame.
 */

public Address args_base(frp)
register Frame frp;
{
    register Address base;

#   ifdef sun
	/* FIXME need to deal with multiple args_bases : (args 1-6 are
	in I reg save are, additional args are further up stack*/
	base = (frp == nil) ? reg(REG_FP) : frp->active_fp;
#   else ifdef vax
	base = (frp == nil) ? reg(ARGP) : frp->save_ap;
#   endif
    return base;
}

/*
 * Return the nth argument to the current procedure.
 */

public Word argn(n, frp)
Integer n;
Frame frp;
{
    register Address argaddr;
    Word w;

#   ifdef sun
	argaddr = args_base(frp) + 4 + (n * sizeof(Word));
#   else
	argaddr = args_base(frp) + (n * sizeof(Word));
#   endif
    dread(&w, argaddr, sizeof(w));
    return w;
}

/*
 * Return the number of words of arguments passed to the procedure
 * associated with the given frame.
 */

public int nargspassed(frp)
Frame frp;
{
    int n;

#   ifdef sun
    register Frame lfrp;
    struct Frame frame;

	if (frp == nil) {		/* for current procedure */
	    lfrp = &frame;
	    getcurframe(lfrp);
            n = lfrp->nargwords;
	}
	else {
            n = frp->nargwords;
	}
#   else ifdef vax
	n = argn(0, frp);
#   endif
    return n;
}

/*
 * Continue execution up to the next source line.
 *
 * There are two ways to define the next source line depending on what
 * is desired when a procedure or function call is encountered.  Step
 * stops at the beginning of the procedure or call; next skips over it.
 */

/*
 * Executed when the "next" command is given.
 */

public next()
{
    register Symbol old_routine;
    struct Frame oldframe, frame;
    register Address pc_after_call;
    register Lineno oldline;

    can_continue();
    oldline = brkline;
    getcurframe(&oldframe);
    old_routine = whatblock(oldframe.active_pc, true);
    isstopped = false;

    dostep(true);
    pc_after_call = reg(REG_PC);
    getcurframe(&frame);
    /*
     * See if we've reached the same line number (e.g. bottom of switch code)
     * and if so, call dostep again.
     */
    if (not inst_tracing and oldline == brkline) {
	if (oldline > 0 and brkline > 0) {
	    if (oldframe.active_fp == frame.active_fp) {
		isstopped = false;
		dostep(true);
		isstopped = true;
		return;
	    }
	}
    }
    /*
     * Handle case where we just executed a recursive call and landed
     * in the called routine.
     */
    while (inst_tracing == false and
      (oldframe.active_fp > frame.active_fp) and
      (old_routine == whatblock(frame.active_pc, true))) {
	pstep(process);
	stepto(pc_after_call);
	getcurframe(&frame);
    }
    isstopped = true;
}

/*
 * Stepc is what is called when the step command is given.
 * It has to play with the "isstopped" information.
 */

public stepc()
{
    register Lineno oldline;
    struct Frame oldframe, newframe;

    can_continue();
    getcurframe(&oldframe);
    oldline = brkline;
    isstopped = false;

    dostep(false);
    isstopped = true;
    if (not inst_tracing and oldline == brkline) {
	if (oldline > 0 and brkline > 0) {
	    getcurframe(&newframe);
	    if (oldframe.active_fp == newframe.active_fp) {
    		isstopped = false;
    		dostep(false);
    		isstopped = true;
	    }
	}
    }
}

/*
 * Print a list of currently active blocks starting with most recent.
 */

public wherecmd(n)
int n;
{
    walkstack(n, printcall);
}

/*
 * Print all the variables local to a given function.
 * The default is the current function.
 */
public dump(p)
Node p;
{
    Frame frp;
    Symbol func;

    if (notstarted(process)) {
		error("program is not active");
    } else if (p == nil) {
	func = curfunc;
	frp = curfuncframe();
    } else {
	func = p->value.sym;
	frp = findframe(func);
    }
    if (frp == nil) {
	error("Function `%s' not active", symname(func));
    }
    dumpvars(func, frp);
}

/*
 * Walk the stack of active procedures, performing some action
 * (printing information, looking for sources, etc.) for each
 * active procedure.
 */

private walkstack(levels, callback)
int levels;
Boolean	(*callback)();
{
    Frame frp;
    Boolean save;
    Symbol f;
    struct Frame frame;
    Address sv_pc;
    int save_dbxcalls;
    Boolean brk;

    if (notstarted(process)) {
	error("program is not active");
    } else {
	save = walkingstack;
	save_dbxcalls = dbxcalls;
	dbxcalls = 0;
	walkingstack = true;
	frp = &frame;
	getcurframe(frp);
	sv_pc = frp->active_pc;
	f = whatblock(sv_pc, true);
	do {
	    if (is_call_loc(frp->active_pc)) {
		sv_pc = saved_pc(dbxcalls);
	    } else {
		sv_pc = frp->active_pc;
	    }
	    brk = callback(f, frp, sv_pc);
	    frp = nextfunc(frp, &f);
	    levels--;
	} while (frp != nil and (program == nil or f != program) and
	    levels != 0 and brk == false);
	walkingstack = save;
	dbxcalls = save_dbxcalls;
    }
}

/*
 * Return the frame for the next function.
 * Given a frame to begin from.
 */
public Frame nextfunc(frp, fp)
Frame frp;
Symbol *fp;
{
    Symbol t;
    Frame nfrp;

    t = *fp;
    nfrp = nextframe(frp);
    if (nfrp != nil) {
	if (not is_call_loc(nfrp->active_pc)) {
	    t = whatblock(nfrp->active_pc, true);
	} else {
	    dbxcalls++;
	    t = whatblock(saved_pc(dbxcalls), true);
	}
    } else {
	t = nil;
    }
    *fp = t;
    return(nfrp);
}

/* -- -- -- Some routines imported from adb -- -- -- */
/*
 *	is_leaf_proc
 *	has_save
 *	firstword
 *	calltarget
 *	ijmpreg
 *	jmpcall
 *	splitjmp
 *
 * Here are some of the instruction op-code #defines they need.
 *	These defines are for those few places outside the disassembler
 *	that need to decode a few instructions.  They must be public
 *	so that dbx_sstep can see them.
 */


#ifndef public	/* this makes them "public" */

/* Values for the main "op" field, bits <31..30> */
#define SR_FMT2_OP              0
#define SR_CALL_OP              1
#define SR_FMT3a_OP             2

/* Values for the tertiary "op3" field, bits <24..19> */
#define SR_JUMP_OP              0x38
#define SR_TICC_OP              0x3A

/* A value to compare with the cond field, bits <28..25> */
#define SR_ALWAYS               8

#endif public

/* Temp, Kluge to allow sas defines to coexist with those in dbx. */
#define Word sas_Word
#include "sas_includes.h"
#undef  Word
#undef  reg

/*
 * is_leaf_proc -- figure out whether the routine we are in SAVEd its
 * registers.  If it did NOT, is_leaf_proc returns true and sets the entry
 * and caller fields of frp.   Here's why we have to know it:
 *
 *	Normal (non-Leaf) routine	Leaf routine
 * sp->		"my" frame		   caller's frame
 * i7->		caller's PC		   caller's caller's PC
 * o7->		invalid			   caller's PC
 *
 * I.e., we don't know who our caller is until we know if we're a
 * leaf proc.   (Note that for dbx's purposes, we are considered to be
 * in a leaf proc even if we're stopped in a routine that will, but has
 * not yet, SAVEd its registers.)
 *
 * The way to find out if we're a leaf proc is to find our own entry point
 * and then check the following few instructions for a "SAVE" instruction.
 * If there is none that are < PC, then we are a leaf proc.
 *
 * We find our own entry point by looking for a the largest symbol whose
 * address is <= the PC.  If the executable has been stripped, we will have
 * to do a little more guesswork; if it's been stripped AND we are in a leaf
 * proc, AND the call was indirect through a register, we may be out of luck.
 */

/*
 * return_if_bad returns from is_leaf_proc if the address (of the
 * next frame to look at) isn't even plausible (not in address space,
 * or greater than previous frame pointer).
 */
#define return_if_bad( addr, prev ) \
	dread( &trash, addr, sizeof(Word)); \
	if( read_err_flag  ||  addr > prev ) return false;
#define CALL_INDIRECT -1 /* flag from calltarget for an indirect call */

private Boolean is_leaf_proc ( upc, cur_o7, usp )
	Address cur_o7;		/* caller's PC if leaf proc (rtn addr) */
				/* invalid otherwise */
	Address	usp,		/* user's stack pointer */
			upc;		/* user's program counter */
{
	Address sv_i7,		/* if leaf, caller's PC ... */
				/* else, caller's caller's PC */
		cto7,		/* call target of call at cur_o7 */
				/* (if leaf, our entry point) */
		cti7,		/* call target of call at sv_i7 */
				/* (if leaf, caller's entry point) */
		near_addr;	/* nearest symbol below PC; we hope it ... */
				/* ... is the address of the proc we're IN */
	Address trash;
	Symbol  near_sym;
	int offset, leaf;

	if( cur_o7 == nil  ||  usp == nil ) {
	    return false;
	}
		/* Collect the info we'll need */
	frame_read( sv_i7, usp, fr_w.rw_in[7]); /* normally,  the caller's PC */
	near_sym = whatblock( upc, true ); /* whatfunc is upc in? */
	if( near_sym == nil ) {
	    /* Without a sym, must assume no leaf proc */
	    return false;
	}
	near_addr = rcodeloc(near_sym);

	/*
	 * has_save will look at the first few instructions
	 * at near_addr, unless upc is within there.
	 */
	if( has_save( near_addr, upc ) ) {
	    /* Not a leaf proc.  This is the most common case. */
	    return false;
	}


	/*
	 * OK, we either had no save instr or we have no symbols.
	 * See if the saved o7 could possibly be valid.  (We could
	 * get fooled on this one, I think, if we're really in a non-leaf,
	 * have no symbols, and o7 still (can it?) has the address of
	 * an indirect call (a call to ptrcall or a jmp [%reg], [%o7]).)
	 *
	 * Also, if we ARE a leaf, and have no symbols, and o7 was an
	 * indirect call, we *cannot* find our own entry point.
	 */

	/*
	 * Is there a call at o7?  (or jmp w/ r[rd] == %o7)
	 */
	cto7 = calltarget( cur_o7 );
	if( cto7 == 0 ) return false;		/* nope */

	/*
	 * Is that call near (but less than) where the pc is?
	 * If it's indirect, skip these two checks.
	 */
	/* db_printf( "Is_leaf_proc cur_o7 %X, cto7 %X\n", cur_o7, cto7 ); */

	if( cto7 == CALL_INDIRECT ) {
	    if( near_addr != -1 ) {
		cto7 = near_addr;	/* best guess */
	    } else {
		error( "Cannot trace stack" );
		return false;
	    }
	} else {
	    return_if_bad( cto7, upc );

	    /*
	     * Is the caller's call target near that call?
	     */
	    cti7 = calltarget( sv_i7 );

	    /* db_printf( "Is_leaf_proc caller's call sv_i7 %X, cti7 %X\n",
		    sv_i7, cti7 ); */

	    if( cti7 != CALL_INDIRECT ) {
		if( cti7 == 0 ) return false;
		return_if_bad( cti7, cur_o7 );
	    }

	    /*
	     * check for a SAVE instruction
	     */
	    if( has_save( cto7, upc ) ) {
		/* not a leaf. */
		return false;
	    }
	}

	return true;

} /* end is_leaf_proc */


/* Should be local, within has_save */
private Boolean firstword ( ofthis, isthis )
	char *ofthis, *isthis;
{
	char *ws;

	while( *ofthis == ' '  ||  *ofthis == '\t' )
		++ofthis;
	ws = ofthis;
	while( *ofthis  &&  *ofthis != ' '  &&  *ofthis != '\t' )
		++ofthis;

	*ofthis = 0;
	return (Boolean) ( strcmp( ws, isthis ) == 0 );
}


/*
 * The "save" is typically the first (optimized) or third (unoptimized)
 * instruction in a routine (if it even exists).
 * SAVE_INS is set to the farthest (in bytes!, EXclusive) we should reasonably
 * look for it.  "xlimit" (if it's between procaddr and procaddr+SAVE_INS)
 * is an overriding upper limit (again, exclusive) -- i.e., it is the
 * address after the last instruction we should check.
 */
#define SAVE_INS (5*4)

private int has_save ( procaddr, xlimit )
	Address procaddr, xlimit;
{
	char *asc_instr, *disassemble();
	Word instr;

	if( procaddr > xlimit  ||  xlimit > procaddr + SAVE_INS ) {
	    xlimit = procaddr + SAVE_INS ;
	}

	/*
	 * Find the first three (or so) instructions of the current proc:
	 * If none is a SAVE, then we are in a leaf proc and will
	 * have trouble finding caller's PC.
	 */
	while( procaddr < xlimit ) {
	    iread( &instr, procaddr, sizeof instr );
	    asc_instr = disassemble( instr, procaddr );

	    if( firstword( asc_instr, "save" ) ) {
		return 1;	/* yep, there's a save */
	    }

	    procaddr += 4;	/* next instruction */
	}

	return 0;	/* nope, there's no save */
} /* end has_save */


/*
 * calltarget returns 0 if there is no call there or if there is
 * no "there" there.  A sparc call is a 0 bit, a 1 bit and then
 * the word offset of the target (I.e., one fourth of the number
 * to add to the pc).  If there is a call but we can't tell its
 * target, we return "CALL_INDIRECT".
 *
 * Two complications:
 * 1-	it might be an indirect jump, in which case we can't know where
 *	its target was (the register was very probably modified since
 *	the call occurred).
 * 2-	it might be a "jmp [somewhere], %o7"  (r[rd] is %o7).
 *	if somewhere is immediate, we can check it, but if it's
 *	a register, we're again out of luck.
 */
private int calltarget ( calladdr )
	Address calladdr;
{
	Word instr;
	int offset, ct;

	dread( &instr, calladdr, sizeof instr );
	if( read_err_flag ) {
	    return 0;		/* no "there" there */
	}

	if( X_OP(instr) == SR_CALL_OP ) {
		/* Normal CALL instruction */
		offset = SR_WA2BA( instr );
		ct = offset + calladdr;

		/*
		 * If the target of that call (ct) is an indirect jump
		 * through a register, then say so.
		 */
		dread( &instr, ct, sizeof instr );
		if( read_err_flag ) {
			return 0;
		}

		if( ijmpreg( instr ) )	return CALL_INDIRECT;
		else			return ct;
	}

	/*
	 * Our caller wasn't a call.  Was it a jmp?
	 */
	return  jmpcall( instr );
}


private struct {
	int op, rd, op3, rs1, imm, rs2, simm13;
} jmp_fields;

private int ijmpreg ( instr ) long instr ; {
	if( splitjmp( instr ) ) {
		return jmp_fields.imm == 0  ||  jmp_fields.rs1 != 0 ;
	} else {
		return 0;
	}
}

private int jmpcall ( instr ) long instr; {
  int dest ;

	if(  splitjmp( instr ) == 0 		/* it ain't a jump */
	   || jmp_fields.rd != REG_O7 ) {
					/* Or it doesn't save pc into %o7 */
		return 0;
	}

	/*
	 * It is a jump that saves pc into %o7.  Find its target, if we can.
	 */
	if( jmp_fields.imm == 0  ||  jmp_fields.rs1 != 0 )
		return CALL_INDIRECT;	/* can't find target */

	/* target is simm13, sign extended.  not even pc-relative. */
	return SR_SEX13(instr);
}

private int splitjmp ( instr ) long instr; {

	jmp_fields.op     = X_OP( instr );
	jmp_fields.rd     = X_RD( instr );
	jmp_fields.op3    = X_OP3( instr );
	jmp_fields.rs1    = X_RS1( instr );
	jmp_fields.imm    = X_IMM( instr );
	jmp_fields.rs2    = X_RS2( instr );
	jmp_fields.simm13 = X_SIMM13( instr );

	if( jmp_fields.op == SR_FMT3a_OP )
		return  jmp_fields.op3 == SR_JUMP_OP ;
	else
		return 0;
}
/* -- -- -- end of routines imported from adb -- -- -- */

/*
 * Print out a call, function name, args, and linenumber and source file.
 * Check to see if our caller's callee is us.  This checks for being
 * in a routine that doesn't have or hasn't executed a save instruction.
 * In this case, print the name of the func passed in but no args can
 * be printed.  Then change the func to our callers callee and continue.
 */
private Boolean printcall(func, frp, oldpc)
Symbol func;
Frame frp;
Address oldpc;
{
    Lineno line;
    Address addr;
    Address retaddr;
    Address sv_fp;
    int instruc;
    int nextword;
    Symbol s;
    Symbol s1;
    struct Frame frame;

    if (oldpc != frp->active_pc) {
	printf("--- called from debugger ---\n");
    }

    if (func != nil) {
        printname(func);
        printparams(func, frp);
        line = srcline(oldpc);
        if (line != 0 and line != -1 and func->symvalue.funcv.src) {
	    printf(", line %d", line);
	    printf(" in \"%s\"\n", shortname(srcfilename(oldpc)));
        } else {
	    printf(" at 0x%x\n", oldpc);
        }
    } else {
	printf(" 0x%x", addr);
        c_printparams(func, frp);
	printf(" at 0x%x\n", oldpc - 1);
    }
    return false;
} /* end printcall */

/*
 * Find the entry point of a procedure or function.  (After the frame setup
 * instruction[s].) Search only as far as MAX_PROLOG_LEN -- also, stop
 * if you see a "trap".  This routine is called during dbx startup -- once
 * for EVERY routine in the debuggee!  Surely this is unnecessary.
 *
 * Note:  this can *still* fail, if you run into a routine
 * that is less than MAX_PROLOG_LEN bytes long, has no trap
 * in it, and is followed by a routine that starts with a
 * "save".
 */

public findbeginning(f)
Symbol f;
{
    Address startaddr, pastend;
    Word prolog[ MAX_PROLOG_WORDS ];
    int ix;

    startaddr = f->symvalue.funcv.beginaddr;
    f->symvalue.funcv.beginoffset = 0;
    pastend = startaddr + MAX_PROLOG_LEN;

    /* Can we even iread yet? */
    iread( prolog, startaddr, MAX_PROLOG_LEN );

    for( ix = 0; startaddr < pastend; ++ix ) {
	if( is_trap_instr( prolog[ix] ) ) {
	    /* f-> ... beginaddr is already right */
	    return;
	}
	startaddr += sizeof (Word);
	if( is_save_instr( prolog[ix] ) ) {
	    f->symvalue.funcv.beginoffset = startaddr - f->symvalue.funcv.beginaddr;
	    f->symvalue.funcv.beginaddr = startaddr;
	    return;
	}
    }
    /*
     * Fall out of the for ==> we don't know what's going on,
     * so we leave f->symvalue.funcv.beginaddr alone.
     */
}

/*
 * Return the address corresponding to the first line in a function.
 */

public Address firstline(f)
Symbol f;
{
    register Address addr, endaddr;

    addr = codeloc(f);
    endaddr = lastaddrproc(f);
    if (endaddr == NOADDR) {
	addr = -1;
    } else {
	while (linelookup(addr) == 0 and addr < endaddr) {
	    ++addr;
    	}
    	if (addr == endaddr) {
	    addr = -1;
    	}
    }
    return addr;
}

/*
 * Return the address corresponding to the end of the program.
 *
 * We look for the entry to "exit".
 */

public Address lastaddr()
{
    register Symbol s;

    if (pascal_pgm) {
	s = lookup(identname("PCEXIT", true));
	if (s == nil) {
	    s = lookup(identname("_exit", true));
	}
    } else {
    	s = lookup(identname("_exit", true));
    }
    if (s == nil) {
	    return nil;
    }
    return codeloc(s);
}

/*
 * Decide if the given function is currently active.
 *
 * We avoid calls to "findframe" during a stack trace for efficiency.
 * Presumably information evaluated while walking the stack is active.
 */

public Boolean isactive(f)
Symbol f;
{
    register Boolean b;

    while (f != nil and f->class == MODUS) f = f->block;
    if (process == nil or isfinished(process)) {
	b = false;
    } else {
	if (walkingstack or f == nil or f == program or
	  (ismodule(f) and isactive(container(f)))) {
	    b = true;
	} else {
	    b = (Boolean) (findframe(f) != nil);
	}
    }
    return b;
}

#define MAX_CALLSTACK	10

struct	Callstack {
	Boolean call_active;
    Word oldpc, oldnpc;		/* Save pc/npc values */
    Boolean batchcmd;		/* Is this a batch or interactive command */
    jmp_buf save_env;		/* Save the state of the world */
    char     call_save[ CALL_SIZE ];

    /* If the function at this level returns a struct: */
    /* If ret_str_size is zero, it's not a struct */
    unsigned ret_str_size;      /* If zero, it's not a struct */
    unsigned real_str_size;     /* The unimp should match exactly */
    Address  ret_str_address;	/* where (on the stack) we put it */
};

typedef struct	Callstack *Callstack;
private struct Callstack callstack[MAX_CALLSTACK];
private Callstack callsp = callstack;

/*
 * Call and execute a procedure or function.
 * For a 'call' command, we set up nested command levels in dbx.
 * This is because the call may trip over a breakpoint.  When
 * it does trip over a breakpoint we want to return to this
 * command level, not the top most one started from dbxmain().
 *
 * Save the current setjmp buffer, and initialize a new one.
 * Setjmp() returns "r == SETJMP" and then the arguments are
 * pushed and the routine is called.  A break point is placed
 * at the location where the call will return to and cont() executes
 * the routine.
 *
 * If any breakpoints are encountered, erecover() will be called
 * which will longjmp() back to here with "r == CMD_LEVEL".
 * Control then falls into yyparse() where the user can issue
 * dbx commands.  Bpact() will recognize when we have
 * hit the breakpoint at the return address of the called routine.
 * It will do a longjmp() back to "r == CALL_RETURN" and at that
 * point we will exit this command level and return to where
 * we were called.  In the "r == CALL_RETURN" case, we return the
 * value of register o0 (most commonly the value that the called
 * function returned), so that it may be restored by popenv.
 *
 * This scheme allows for arbitrarily nested call/command levels.
 */
public callproc(proc, arglist)
Symbol proc;
Node arglist;
{
    Setjmp r;
    int rval = 0;

    if (proc->class != FUNC and proc->class != PROC) {
	error("name clash for \"%s\" (set scope using \"func\")",symname(proc));
    }
	callsp->call_active = true;
    copy_jmpbuf(env, callsp->save_env);
    callsp->batchcmd = batchcmd;
    r = (Setjmp) setjmp(env);
    switch(r) {
    case SETJMP:
  	/* isstopped = true; moved under pushenv() down below (ivan) */
        if (callsp > &callstack[MAX_CALLSTACK]) {
	    error("dbx 'call's nested too deeply - max is %d", MAX_CALLSTACK);
        }
        callsp->oldpc = reg(REG_PC) + 1;
        callsp->oldnpc = reg(REG_NPC);
        catch_retaddr();
        callsp++;
        pushenv();
	isstopped = true;
        pc = rcodeloc(proc) ;
	prepare_struct(proc);
        pushparams(proc, arglist);
	hidden_struct_param();
        beginproc(proc, callsp[-1].real_str_size, callsp[-1].call_save );
        cont(0);
	/* NOTREACHED */
    case CMD_LEVEL:
	batchcmd = false;
	yyparse();
	/* NOTREACHED */
    case CALL_RETURN:
	call_return();
	break;
    }
    reset_retaddr();
}

/*
 * The following two routines (and the three "ret_*" fields of the
 * "struct Callstack") allow dbx to call functions which return structs.
 *
 *	prepare_struct -- allocate space for it by modifying SP
 *	    This must be done before pushing the other parameters.
 *
 *	hidden_struct_param -- set up the "hidden" parameter (a pointer
 *	    to the struct we just allocated).  This is done after
 *	    pushing the parameters.
 *
 * The other essentials are
 * (1) "eval" will (after performing the call and popping the "normal"
 *	parameters, copy the struct onto dbx's own stack, and pop the
 *	subprocess's stack; and
 * (2)  an "unimp #LEN" instruction must be made a part of the call,
 *	where #LEN is the length of the structure.  This is done in
 *	beginproc (machine.c).
 */

/* sparc stack must be aligned to a double-word boundary */
#define dalign_mask 7
#define dalign_size 8


private prepare_struct(proc)
Symbol proc;
{
  Symbol t;
  Address old_sp;
  Callstack cp = callsp -1;

    t = rtype( proc->type );
    if( t->class == RECORD ) {
	cp->real_str_size = size( t );
	if( cp->real_str_size & dalign_mask )
	    cp->ret_str_size = (cp->ret_str_size + dalign_size) & ~dalign_mask;
	else
	    cp->ret_str_size = cp->real_str_size ;
	old_sp = reg(REG_SP);
	cp->ret_str_address = old_sp - cp->ret_str_size;
	setreg( REG_SP, cp->ret_str_address );
    } else {
	cp->ret_str_size = 0;
    }
}

private hidden_struct_param()
{
  struct real_Frame *rfp;
  Callstack cp = callsp -1;

    if( cp->ret_str_size ) {
	rfp = (struct real_Frame *) reg(REG_SP);
	dwrite( & cp->ret_str_address, &rfp->fr_struct_arg, sizeof(Address) );
    }
}


/*
 * At the point when this is called, we have "popped" dbx's callstack.
 */
private Address struct_return_address() {
    return callsp->ret_str_address;
}


/*
 * Push the arguments on the process' stack.  We do this by first
 * evaluating them on the "eval" stack, then copying into the process'
 * space.  Whether or not any arguments are given, we create a whole
 * stack frame; if there are more than 6 words (sizeof (Reg_args)) of
 * arguments, then it must be a bigger-than "normal" frame.  This
 * determination is complicated by the structure values and string
 * constants that c_evalparams can generate and push onto the stack.
 * Their values cannot go into the registers.
 *
 * "pushenv" has already saved the six normal O-regs.
 */

private pushparams(proc, arglist)
Symbol proc;
Node arglist;
{
    Stack *savesp, *excess_start;
    int frame_size = MIN_FRAME_SIZE;
    int args_size, min_size, size_left, reg_last;
    int excess_size, sss, diff;
    int align_stack;
    Address old_sp, new_sp;

    savesp = sp;
    evalparams(proc, arglist);
    args_size = sp - savesp;
    excess_size = args_size - ARG_REGS_SIZE;

    if( excess_size < 0 ) {
	min_size = args_size;
	excess_size = 0;
    } else {
	min_size = ARG_REGS_SIZE;
    }
    excess_start = savesp + min_size;

    /*
     * Check for stacked strings -- split args into which go into
     * regs vs which must be on the stack.  c_evalparams (we don't
     * support this in other languages yet) guarantees that sss and
     * the stack pointer are both aligned on a four-byte boundary.
     */
    sss = stk_str_len( );
    if( sss  &&  sss > excess_size ) {
	/*
	 * Need to split the arguments -- some of the ones we were
	 * about to put into registers are really supposed to be
	 * string constants -- on the stack.
	 */
	diff = sss - excess_size;
	excess_start -= diff;
	excess_size = sss;
    } else {
	/*
	 * We have enough actual parameters so that even if we do
	 * have any stacked strings, they'll go on the stack
	 * without any extra effort.
	 */
	diff = 0;
    }

    if( excess_size > 0 ) {
	frame_size += excess_size;
    }


    align_stack = frame_size & dalign_mask ;
    if( align_stack ) {	/* Frame size is not double-aligned */
	align_stack = dalign_size - align_stack;
	frame_size += align_stack;
    }

    /* Make the new frame */
    old_sp = reg(REG_SP);
    new_sp = old_sp - frame_size;
    setreg(REG_SP, new_sp );
    /* args may be in regs or on stack, need to set the stack version for F77 */
    dwrite(savesp, old_sp - args_size, args_size);
    if( excess_size > 0 ) {	/* write (&pop) the extra parameters */
	dwrite(excess_start, new_sp + MIN_FRAME_SIZE + align_stack, excess_size);
	sp = excess_start;
	size_left = min_size - diff;
    } else {
	size_left = args_size;
    }

    reg_last = REG_O0 + (size_left/sizeof(Word)) -1;

    while ( size_left > 0 ) {
	setreg( reg_last, pop( Word ) );
	reg_last--;
	size_left -= sizeof( Word );
    }

    sp = savesp;	/* should be a no-op */
}

/*
 * Return the saved pc value for a call that has been stacked.
 * The argument indicates how far back into the stack to go.
 */
public Address saved_pc(ncalls)
int ncalls;
{
    Callstack csp;

    csp = callsp - ncalls;
    if (csp < callstack) {
	return(nil);
    }
    return(csp->oldpc);
}

/*
 * Set at breakpoint at the return address of a function that we're
 * going to call.  Check if there is already a breakpoint there.
 */
private catch_retaddr()
{
    if (callsp == callstack) {
        setbp(CALL_RETADDR);
    }
}

/*
 * Unset the breakpoint at the return address of a function called from
 * dbx.  Unset it only if this is the last function call waiting on
 * that address.
 */
private reset_retaddr()
{
    if (callsp == callstack) {
        unsetbp(CALL_RETADDR);
    }
}

/*
 * A called routine has returned.
 * Pop the setjmp buffer and restore pc to the location
 * from which it was called.
 * Also, restore the original instructions where the call
 * was installed.
 */
public call_return()
{
    callsp--;
    copy_jmpbuf(callsp->save_env, env);

    pc = callsp->oldpc - 1;
    setreg(REG_PC, pc );
    setreg(REG_NPC, callsp->oldnpc );

    /* restore original startup code */
    iwrite(callsp->call_save, CALL_LOC, CALL_SIZE );

    batchcmd = callsp->batchcmd;
	callsp->call_active = false;
}

public Boolean in_dbxcall()
{
	return callstack->call_active;
}
/*
 * Reset the pointer into the call stack.
 */
reset_callstack()
{
    if (callsp != callstack) {
        callsp = callstack;
        unsetbp(CALL_RETADDR);
    }
}
/*
 * Test whether a given PC value is the address of the call or not.
 * If no call has been planted, this is ipso facto false.
 */
private is_call_loc(addr)
Address addr;
{
    return (callsp != callstack and addr == CALL_LOC);
}

/*
 * Test whether a given PC value is the return address from the call or not.
 * If no call has been planted, this is ipso facto false.
 */
public is_call_retaddr(addr)
Address addr;
{
    return (callsp != callstack and addr == CALL_RETADDR);
}

public procreturn()
{
    flushoutput();
    popenv();
}

/*
 * Push the current environment.
 */

private pushenv()
{
    push(Address, pc);
    push(Lineno, cursrcline);
    push(Lineno, brkline);
    push(String, cursource);
    push(Boolean, isstopped);
    push(Symbol, curfunc);

    push(Word, reg(REG_G1));
    push(Word, reg(REG_G2));
    push(Word, reg(REG_G3));
    push(Word, reg(REG_G4));
    push(Word, reg(REG_G5));
    push(Word, reg(REG_G6));
    push(Word, reg(REG_G7));

    /* save the outgoing parameter registers */
    push(Word, reg(REG_O0));
    push(Word, reg(REG_O1));
    push(Word, reg(REG_O2));
    push(Word, reg(REG_O3));
    push(Word, reg(REG_O4));
    push(Word, reg(REG_O5));
    push(Word, reg(REG_O6));
    push(Word, reg(REG_O7));

    push(Word, reg(REG_NPC));
    push(Word, reg(REG_PC));
    /* must save an even number of regs to maintian sp double aligned */
    /* this is a bit of a kludge to paper over the symptom.    J.Peck */
    /* push(Word, reg(REG_SP)); is already REG_O6 */
    

}

/*
 * Pop back to the real world.
 */

public popenv()
{
    register String filename;

    /* setreg(REG_SP, pop(Word)); is already REG_SP */
    setreg(REG_PC, pop(Word));
    setreg(REG_NPC, pop(Word));

    /* restore the outgoing parameter registers */
    setreg(REG_O7, pop(Word));
    setreg(REG_O6, pop(Word));
    setreg(REG_O5, pop(Word));
    setreg(REG_O4, pop(Word));
    setreg(REG_O3, pop(Word));
    setreg(REG_O2, pop(Word));
    setreg(REG_O1, pop(Word));
    setreg(REG_O0, pop(Word));

    setreg(REG_G7, pop(Word));
    setreg(REG_G6, pop(Word));
    setreg(REG_G5, pop(Word));
    setreg(REG_G4, pop(Word));
    setreg(REG_G3, pop(Word));
    setreg(REG_G2, pop(Word));
    setreg(REG_G1, pop(Word));

    curfunc = pop(Symbol);
    isstopped = pop(Boolean);
    filename = pop(String);
    brkline = pop(Lineno);
    cursrcline = pop(Lineno);
    pc = pop(Address);
    cursource = filename;
}

/*
 * Flush the debuggee's standard output.
 *
 * This is VERY dependent on the use of stdio.
 * We don't need a whole new stack frame -- fflush has only
 * the single FILE* parameter.
 */

public flushoutput()
{
    register Symbol p, iob;
    register Stack *savesp;
    Address iob_stdout;

    p = lookup(identname("fflush", true));
    while (p != nil and not isblock(p)) {
	p = p->next_sym;
    }
    if (p != nil) {
	iob = lookup(identname("_iob", true));
	if (iob != nil) {
	    pushenv();
	    pc = rcodeloc(p);
	    iob_stdout = address(iob, nil) + sizeof(struct _iobuf);
	    setreg( REG_O0, iob_stdout );
	    beginproc(p, 0, 0);
	    catch_retaddr();
	    resume(0);
	    reset_retaddr();
	    popenv();
	}
    }
}

/*
 * Return the frame for the current function.
 * The space for the frame is allocated statically.
 */
public Frame curfuncframe ()
{
    static struct Frame frame;
    Frame frp;

    if (curframe == nil) {
	frp = findframe(curfunc);
	if (frp != nil) {
	    curframe = &curframerec;
	    *curframe = *frp;
	}
    } else {
	frp = &frame;
	*frp = *curframe;
    }
    return frp;
}


/*
 * Walk up the call stack (towards main) 'nlevels'.
 */
public stack_up(nlevels)
int nlevels;
{
    int i;
    Symbol f;
    Symbol nf;
    Frame frp;
    Frame nfrp;
    Boolean more;

    if (not isactive(program)) {
	error("program is not active");
    } else if (curfunc == nil) {
	error("no current function");
    } else if (nlevels > 0) {
	i = 0;
	nf = curfunc;
	frp = curfuncframe();
	if (frp == nil) {
		fprintf(stderr, "No active stack frames\n");
		return;
	}
	more = true;
	do {
	    if (frp == nil or f == program) {
		more = false;
		error("not that many levels");
	    } else if (i >= nlevels) {
		more = false;
	    } else {
		nfrp = nextfunc(frp, &nf);
		if (nfrp != nil) {
		    frp = nfrp;
		    f = nf;
	            ++i;
		} else {
		    more = false;
		    if (i == 0) {
			fprintf(stderr, "Already at the top call level\n");
			return;
		    } else {
			fprintf(stderr, "Moved up %d call%s\n", i,
			  (i == 1)? "": "s");
		    }
		}
	    }
	} while (more);
	curfunc = f;
	curframe = &curframerec;
	*curframe = *frp;
        frame_move(frp);
    } else if (nlevels < 0) {
	stack_down(-nlevels);
    }
}

/*
 * Walk down the call stack (away from 'main') 'nlevels'.
 */
public stack_down(nlevels)
int nlevels;
{
    int i, depth;
    Frame frp;
    Symbol f;
    struct Frame frame;
    Frame curfrp;

    if (not isactive(program)) {
	error("program is not active");
    } else if (curfunc == nil) {
	error("no current function");
    } else if (nlevels > 0) {
	depth = 0;
	frp = &frame;
	getcurfunc(frp, &f);
	if (curframe == nil) {
	    curfrp = findframe(curfunc);
	    if (curfrp == nil) {
		fprintf(stderr, "No active stack frames\n");
		return;
	    }
	    curframe = &curframerec;
	    *curframe = *curfrp;
	}
	dbxcalls = 0;
	while ((f != curfunc or !frameeq(frp, curframe)) and f != nil) {
	    frp = nextfunc(frp, &f);
	    ++depth;
	}
	if (f == nil or nlevels > depth) {
	    if (depth == 0) {
		fprintf(stderr, "Already at the bottom call level\n");
		return;
	    } else {
	        fprintf(stderr, "Moved down %d call%s\n", depth,
		  (depth == 1)? "": "s");
	    }
	    nlevels = depth;
	}
	depth -= nlevels;
	frp = &frame;
	dbxcalls = 0;
	getcurfunc(frp, &f);
	for (i = 0; i < depth; i++) {
	    frp = nextfunc(frp, &f);
	    assert(frp != nil);
	}
	curfunc = f;
	*curframe = *frp;
        frame_move(frp);
    } else if (nlevels < 0) {
	stack_up(-nlevels);
    }
}

/*
 * We have moved the notion of the current frame.
 * Print a line describing where this frame came from.
 */
private frame_move(frp)
Frame frp;
{
    Lineno line;
    Address oldpc;

    printf("Current function is %s\n", symname(curfunc));
    if (is_call_loc(frp->active_pc)) {
	oldpc = saved_pc(dbxcalls);
    } else {
	oldpc = frp->active_pc - 1;
    }
    setsource(srcfilename(oldpc));
    line = srcline(oldpc);
    if (istool()) {
	dbx_callstack(line);
    } else {
        printlines(line, line);
    }
}

/*
 * Set the current function.
 * Clear the current frame here so that the right one will be
 * found when needed.
 */
public setcurfunc(func)
Symbol func;
{
    curfunc = func;
    curframe = nil;
    dbxcalls = 0;
}

/*
 * Walk the stack looking for a routine with some source
 */
public find_some_source()
{
        walkstack(0, find_some_source1);
}

/*
 * If the given function has source associated with it,
 * make this the current source and file and then return
 * true telling the stack walker not to go any farther.
 */
private Boolean find_some_source1(f, frp, oldpc)
Symbol f;
Frame frp;
Address oldpc;
{
	if( f == nil  ||  ! f->symvalue.funcv.src ) {
                return false;
	} else {
                setsource(srcfilename(oldpc));
                cursrcline = srcline(oldpc);
                setcurfunc(f);
                return true;
        }
}

/*
 * To be called by the parent dbx when debugging stack frame stuff.
 */
private dump_frame ( frp )
Frame frp;
{
   int *ip;

	printf( "struct frame at 0x%08x = {\n", frp );
	printf( "\theader     	= 0x%08x\n", frp->header );
	printf( "\tactive_pc     = 0x%08x\n", frp->active_pc );
	printf( "\tactive_sp     = 0x%08x\n", frp->active_sp );
	printf( "\tactive_fp     = 0x%08x\n", frp->active_fp );
	printf( "\tcaller        = 0x%08x\n", frp->caller );
	printf( "\tis_leaf       = %s\n", frp->is_leaf ? "true" : "false" );
	printf( "\ttramp_caller  = %s\n", frp->tramp_caller ? "true" : "false" );
	printf( "\tnargwords     = 0x%08x\n", frp->nargwords );
	printf( "}\n" );
}
