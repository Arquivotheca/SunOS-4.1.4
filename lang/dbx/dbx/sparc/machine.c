#ifndef lint
static	char sccsid[] = "@(#)machine.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Target machine dependent stuff.
 */

#include "defs.h"
#include "machine.h"
#include "process.h"
#include "events.h"
#include "dbxmain.h"
#include "symbols.h"
#include "source.h"
#include "mappings.h"
#include "object.h"
#include "tree.h"
#include "runtime.h"
#include <signal.h>

#ifndef public	>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

typedef unsigned int Address;
typedef unsigned char Byte;
typedef unsigned int Word;

#include <sys/types.h>
#include <machine/reg.h>	/* need reg windows */

#include <machine/pcb.h>	/* need reg windows */

#ifdef sparc

/*
 * The address corresponding to the beginning of a function is recorded
 * as the address after the "save" instruction, so that local information
 * is available when you "stop in" that function.
 */

/*
 * Note:  these #defines do differ from those in <machine/reg.h>.
 * The REG_* are defined a ways further on.
 */

#   define REG_FRP REG_FP
#   undef PC
#   undef SP

/*
 * dbx' "call" command is built in the first few bytes of text space.
 * the +4 is magic
 */

# define CALL_LOC	codestart+4	/* loc of start of call */
# define CALL_RETADDR	codestart+16	/* Return address for 'call' command */
# define CALL_SIZE	12		/* size of three-instruction 'call' */

extern char *disassemble();

#endif sparc

#define BITSPERBYTE 8
#define BITSPERWORD (BITSPERBYTE * sizeof(Word))

Address pc;
Address prtaddr;


#ifndef rw_fp
	/* Damn.  We got an old reg.h, with struct window instead of rwindow */
	/* This is temporary !!!!! */
#	define rwindow window
#	define rw_local w_local
#	define rw_in    w_in
#	define rw_fp    rw_in[6]
#	define rw_rtn   rw_in[7]
#endif

#endif !public	<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<

private Address printop();

#define IMDF(sz) "#0x%lx"

/*
 * Decode and print the instructions within the given address range.
 */

public printinst(lowaddr, highaddr)
Address lowaddr;
Address highaddr;
{
    register Address addr;

    for (addr = lowaddr; addr <= highaddr; ) {
	addr = printop(addr);
	if (addr < lowaddr) {
	    break;
	}
    }
    prtaddr = addr;
}

/*
 * Another approach:  print n instructions starting at the given address.
 */

public printninst(count, addr)
int count;
Address addr;
{
    register Integer i;
    register Address newaddr;

    if (count <= 0) {
	error("non-positive repetition count");
    } else {
	newaddr = addr;
	for (i = 0; i < count; i++) {
	    newaddr = printop(newaddr);
	}
	prtaddr = newaddr;
    }
}

/*
 * Print the contents of the addresses within the given range
 * according to the given format.
 */

struct Format {
    String name;
    String printfstring;
    int length;
    int verbose;
    int (*prtfunc)();
};
typedef struct Format *Format;

private	int	print_char(), print_short(), print_long(), print_float();
private int	print_double(), print_string();

private struct Format fmt[] = {
    { "d", " %d",     sizeof(short),  0, print_short },
    { "D", " %ld",    sizeof(long),   0, print_long },
    { "o", " 0%o",    sizeof(short),  0, print_short },
    { "O", " 0%lo",   sizeof(long),   0, print_long },
    { "x", " 0x%04x", sizeof(short),  0, print_short },
    { "X", " 0x%08x", sizeof(long),   0, print_long },
    { "b", " \\%o",   sizeof(char),   0, print_char },
    { "c", " '%c'",   sizeof(char),   0, print_char },
    { "s", "%c",      sizeof(char),   0, print_string },
    { "f", " %+.6e",  sizeof(float),  1, print_float },
    { "g", " %+.14e", sizeof(double), 1, print_double },
    { "F", " %+.14e", sizeof(double), 1, print_double },
    { nil, nil, 0 }
};

private Format findformat(s)
String s;
{
    register Format f;

    f = &fmt[0];
    while (f->name != nil and not streq(f->name, s)) {
	++f;
    }
    if (f->name == nil) {
	error("bad print format \"%s\"", s);
    }
    return f;
}

public Address printdata(lowaddr, highaddr, format)
Address lowaddr;
Address highaddr;
String format;
{
    register int n;
    register Address addr;
    register Format f;

    if (lowaddr > highaddr) {
	error("first address larger than second");
    }
    f = findformat(format);
    n = 0;
    for (addr = lowaddr; addr <= highaddr; addr += f->length) {
	if (n == 0) {
	    printf("0x%08x: ", addr);
	}
	f->prtfunc(f, addr, true);
	++n;
	if (f->verbose || n >= (16 div f->length)) {
	    printf("\n");
	    n = 0;
	}
    }
    if (n != 0) {
	printf("\n");
    }
    prtaddr = addr;
    return addr;
}

/*
 * The other approach is to print n items starting with a given address.
 */

public printndata(count, startaddr, format)
int count;
Address startaddr;
String format;
{
    register int i, n;
    register Address addr;
    register Format f;

    if (count <= 0) {
	error("non-positive repetition count");
    }
    f = findformat(format);
    n = 0;
    addr = startaddr;
    for (i = 0; i < count; i++) {
	if (n == 0) {
	    printf("0x%08x: ", addr);
	}
	f->prtfunc(f, addr, true);
        ++n;
        if (f->verbose || n >= (16 div f->length)) {
	    printf("\n");
	    n = 0;
        }
        addr += f->length;
    }
    if (n != 0) {
	printf("\n");
    }
    prtaddr = addr;
}

/*
 * Print out a value according to the given format.
 */

public printvalue(v, format)
long v;
String format;
{
    Format f;
    Address addr;

    f = findformat(format);
    if (streq(f->name, "s")) {
	error("Cannot print a value with `s'");
    }
    addr = (Address) &v;
    if (f->length < sizeof(long)) {
	addr += f->length - sizeof(long);
    }
    f->prtfunc(f, addr, false);
    printf("\n");
}

/*
 * Print register(s)
 */

public printreg(n)
register Node n;
{
    register Format f;
    register Integer i, count, m, regvalue, value;
    Node begin, end;
    String mode;
    int perrow;
    Symbol sym;
    char regname[10];

    if (n->op != O_EXAMINE) {
	panic("bad node in printreg()");
    }
    begin = n->value.examine.beginaddr;
    end = n->value.examine.endaddr;
    mode = n->value.examine.mode;
    count = n->value.examine.count;
    regvalue = begin->value.arg[0]->value.sym->symvalue.offset;
    if (end == nil) {
    	if (count < 0) {
	    error("non-positive repetition count");
    	} else if (count == 0) {
	    count = 1;
	} else if (regvalue == REG_PC) {
	    count = 1;
	} else if (count + regvalue > LASTREGNUM) {
	    count = LASTREGNUM - regvalue + 1;
	}
    } else {
	count = (int)end->value.arg[0];
    }
    f = findformat(mode);
    perrow = 4;
    if (streq(f->name, "F") or streq(f->name, "f")) {
	perrow = 1;
    }
    sym = begin->value.arg[0]->value.sym;
    for (i = 0; i < count; i++) {
	if ((i % perrow) == 0) {
	    if (i != 0) {
		printf("\n");
	    }
	    print_regnum(regvalue);
	    m = count - i;	/* how many more to print */
	    if (m > perrow) {
		m = perrow;
	    }
	    m--;
	    if (m > 0) {
		printf("-");
		print_regnum(regvalue + m);
	    }
	    printf("\t");
	}
	value = findregvar(sym, nil);
	printf(f->printfstring, value);
	regvalue++;
	if (regnames[regvalue] == 0) {
		break;
	}
	sprintf(regname, "$%s", regnames[regvalue]);
	sym = lookup(identname(regname, true));
    }
    printf("\n");
}

/*
 * return_regs -- get the (two) registers that might contain
 * the value returned by the function that just returned.
 */
public Word *return_regs ( )
{
  static Word ret_reg[ 2 ];

    ret_reg[ 0 ] = reg( REG_O0 );
    ret_reg[ 1 ] = reg( REG_O1 );

    return ret_reg;
}


#ifndef public
	/* (actually, "ifndef public" exports these defs out to "machine.h") */
/*
 * REG_* are indices for the register names that are in regnames
 */

/* Register definitions for the sun-4 "sparc" */
/* Integer Unit (IU)'s "r registers" */
#define REG_G0		 0
#define REG_G1		 1
#define REG_G2		 2
#define REG_G3		 3
#define REG_G4		 4
#define REG_G5		 5
#define REG_G6		 6
#define REG_G7		 7

#define REG_O0		 8
#define REG_O1		 9
#define REG_O2		10
#define REG_O3		11
#define REG_O4		12
#define REG_O5		13
#define REG_O6		14
#define REG_O7		15

/*
 * The rest of the registers aren't in separate fields anywhere,
 * so we only need some reference points
 */
#define REG_L0		16
#define REG_L7		23
#define REG_I0		24
#define REG_I5		29
#define REG_I7		31	/* (Return address minus eight) */

#define REG_SP		14	/* Stack pointer == REG_O6 */
#define	REG_FP		30	/* Frame pointer == REG_I6 */

/* Floating Point Unit (FPU)'s "f registers" */
#define	REG_F0		32
#define	REG_F1		33
#define REG_F31		63

/* Miscellaneous registers */
#define	REG_Y		64
#define	REG_PSR		65
#define	REG_WIM		66
#define	REG_TBR		67
#define	REG_PC		68
#define	REG_NPC		69

#define REG_FSR		70	/* FPU status */
#define REG_FQ		71	/* FPU queue */


#define LASTREGNUM	REG_FQ	/* index of last avail "register" */
#define	NREGISTERS	REG_FQ+1
#define IS_WINDOW_REG(n) ((n) <= REG_I7 && (n) >= REG_L0)

#endif !public

/*
 * Symbolic names for each of the registers.
 */
public char *regnames[] = {
	/* IU general regs */
	"g0", "g1", "g2", "g3", "g4", "g5", "g6", "g7",
	"o0", "o1", "o2", "o3", "o4", "o5", "o6", "o7",
	"l0", "l1", "l2", "l3", "l4", "l5", "l6", "l7",
	"i0", "i1", "i2", "i3", "i4", "i5", "i6", "i7",
	/* (o6 & i6 are given synonyms sp & fp in process_init) */

	/* FPU regs */
	"f0", "f1", "f2", "f3", "f4", "f5", "f6", "f7",
	"f8", "f9", "f10", "f11", "f12", "f13", "f14", "f15",
	"f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
	"f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",

	/* Miscellaneous */
	"y", "psr", "wim", "tbr", "pc",
	"npc", "fsr", "fq",
	0	/* stopper */
   } ;

/*
 * Print the name of a register symbolically.
 */
private print_regnum(regvalue)
int	regvalue;
{
    if (regvalue >= 0 and regvalue <= LASTREGNUM) {
	printf("%s", regnames[regvalue]);
    } else {
	panic("Invalid register number (%d) in print_regnum", regvalue);
    }
}


/*
 * Print out an execution time error.
 * Assumes the source position of the error has been calculated.
 * This routine is only called from printstatus().
 * printstatus has already checked if the -r option was specified;
 * if so then the object file information hasn't been read in yet.
 */

public printerror()
{
    Integer err;

    if (isfinished(process)) {
	printf("\"%s\" exits with code %d\n", objname, exitcode(process));
	erecover();
    }
    err = errnum(process);
    if (err == SIGINT) {
	printf("\n\ninterrupt ");
	printloc();
    } else if (err == SIGTRAP) {
	printf("\nUnexpected SIGTRAP ");
	printloc();
    } else {
	psig(err, errcode(process));
	printf(" ");
	printloc();
    }
    printf("\n");
    if (brkline > 0) {
	printlines(brkline, brkline);
    } else {
	printinst(pc, pc);
    }
    pr_display(false);
    erecover();
}

typedef struct {
	int	sig_code;
	char	*sig_name;
} sigtag;

sigtag	illinames[] = {
	{ ILL_STACK,		"bad stack" },
	{ ILL_ILLINSTR_FAULT,	"illegal instruction fault" },
	{ ILL_PRIVINSTR_FAULT,	"privileged instruction fault" },
	{ ILL_TRAP_FAULT(3),	"flush windows trap" },
	{ ILL_TRAP_FAULT(4),	"clean windows trap" },
	{ ILL_TRAP_FAULT(5),	"range check" },
};

#define	NILLINAMES	(sizeof illinames/sizeof illinames[0])

sigtag	emtnames[] = {
	{ EMT_TAG,		"tag overflow" },
};

#define	NEMTNAMES	(sizeof emtnames/sizeof emtnames[0])

sigtag	fpenames[] = {
	{ FPE_INTDIV_TRAP,	"integer divide by zero" },
	{ FPE_FLTINEX_TRAP,	"[floating inexact result]" },
	{ FPE_FLTDIV_TRAP,	"[floating divide by zero]" },
	{ FPE_FLTUND_TRAP,	"[floating underflow]" },
	{ FPE_FLTOPERR_TRAP,	"[floating operand error]" },
	{ FPE_FLTOVF_TRAP,	"[floating overflow]" },
};

#define	NFPENAMES	(sizeof fpenames/sizeof fpenames[0])

sigtag	busnames[] = {
	{ BUS_HWERR,		"hardware error" },
	{ BUS_ALIGN,		"alignment error" },
};

#define	NBUSNAMES	(sizeof busnames/sizeof busnames[0])

sigtag	segvnames[] = {
	{ SEGV_NOMAP,		"no mapping at the fault address" },
	{ SEGV_PROT,		"access to address exceeded protections" },
};

#define	NSEGVNAMES	(sizeof segvnames/sizeof segvnames[0])

extern int sys_nerr;
extern char *sys_errlist[];

public char *sigcodename(signo, sigcode)
int	signo;
int	sigcode;
{
    register sigtag *tagp, *tagend;
    static char msgbuf[128+1];
    register int errnum;

    switch (signo) {

    case SIGFPE:
	tagp = &fpenames[0];
	tagend = &fpenames[NFPENAMES];
	break;

    case SIGEMT:
	tagp = &emtnames[0];
	tagend = &emtnames[NEMTNAMES];
	break;

    case SIGILL:
	tagp = &illinames[0];
	tagend = &illinames[NILLINAMES];
	break;

    case SIGBUS:
	tagp = &busnames[0];
	tagend = &busnames[NBUSNAMES];
	break;

    case SIGSEGV:
	if (SEGV_CODE(sigcode) == SEGV_OBJERR) {
	    errnum = SEGV_ERRNO(sigcode);
	    (void) strcpy(msgbuf, "error paging from object: ");
	    if (errnum > 0 && errnum <= sys_nerr)
		(void) strcat(msgbuf, sys_errlist[errnum]);
	    else
		(void) sprintf(msgbuf + strlen(msgbuf), "Error %d", errnum);
	    return (msgbuf);
	}
	tagp = &segvnames[0];
	tagend = &segvnames[NSEGVNAMES];
	break;

    default:
	return (NULL);
    }

    while (tagp < tagend) {
	if (tagp->sig_code == sigcode) {
	    return (tagp->sig_name);
	}
	tagp++;
    }

    if (signo == SIGILL) {
	(void) sprintf(msgbuf, "trap 0x%2x fault", sigcode);
	return (msgbuf);
    }

    return (NULL);
}


/*
 * Note the termination of the program.  We do this so as to avoid
 * having the process exit, which would make the values of variables
 * inaccessible.  We do want to flush all output buffers here,
 * otherwise it'll never get done.
 */

public endprogram()
{
    Integer exitcode;
    register Address addr;

    printnews();
    exitcode = reg( REG_O0 );
    printf("\nexecution completed, exit code is %d\n", exitcode);
    show_main();
    addr = firstline(curfunc);
    if (addr != NOADDR) {
	setsource(srcfilename(addr));
	cursrcline = brkline = srcline(addr) - 5;
	if (cursrcline < 1) {
	    cursrcline = brkline = 1;
	}
    }
    erecover();
}

/*
 * Single step the machine a source line (or instruction if "inst_tracing"
 * is true).  If "isnext" is true, skip over procedure calls.
 */

private Address getcall();

public dostep(isnext)
Boolean isnext;
{
    register Address addr, rexit_addr;
    register Lineno line;
    String filename;
    Boolean call_addr;
    private Address nextaddr( /* Address startaddr, Boolean isnext */ );

    call_addr = false;
    addr = nextaddr(pc, isnext);
    if (not inst_tracing and nlhdr.nlines != 0) {
	line = linelookup(addr);
	rexit_addr = exit_addr;		/* put in reg to make loop faster */
	while ((line == 0 or line == -1) and addr != rexit_addr) {
	    addr = nextaddr(addr, isnext);
	    if (is_call_retaddr(addr)) {
		addr = saved_pc(1) - 1;
		call_addr = true;
	    }
	    line = linelookup(addr);
	}
	brkline = line;
    } else {
	brkline = 0;
    }
    if (not call_addr) {
	stepto(addr);
    } else {
	stepto(CALL_RETADDR);
    }
    filename = srcfilename(addr);
    setsource(filename);
}


#ifdef sparc
/*
 * Hacked version of sparc adb's hacked version of sas' instruction
 * decoding routines.
 *
 * The shared boolean variable "printing" is true if the decoded
 * instruction is to be printed, false if not.  In either case,
 * the address of the next instruction after the given one is returned.
 */


#ifndef public
Boolean printing;
Address instaddr;

#define instread(var) \
{ \
    iread(&var, instaddr, sizeof(var)); \
    instaddr += sizeof(var); \
}
#endif

/*
 * a less tricky interface to iread() -- transfer
 * size is explicit, not gleaned from the type of
 * the destination!
 */
public long instfetch(length)
    int length;
{
    char c;
    short w;
    long l;

    switch(length){
    case 1:
	instread(c);
	return(c);
    case 2:
	instread(w);
	return(w);
    case 4:
	instread(l);
	return(l);
    default:
	error("bad length in instfetch(%d)\n", length);
	/*NOTREACHED*/
    }
}

private Address printop(addr)
Address addr;
{
    long inst;
    char *dres ;

    psymoff(addr);
    printf(":\t");
    iread(&inst, addr, sizeof(inst));

    dres = disassemble( inst, addr ); /* call the sas disassembler */
    instaddr = addr + 4;

    if( strcmp(dres, "???") == 0 ) {
	printf("\tbadop");
    } else {
	printing = true;

	printf( "%s", dres );

	printing = false;
    }
    printf("\n");
    return instaddr;
}


/*
 * convert a signed displacement to string form
 */
private void dispstr(disp, buf)
long disp;
char *buf;
{
    if (disp < -8)
	sprintf( buf, "-0x%lx", -disp);
    else if (disp >= -8 && disp <= -1)
	sprintf( buf, "-%lx", -disp);
    else if (disp >= 0 && disp <= 8)
	sprintf( buf, "%lx", disp);
    else
	sprintf( buf, "0x%lx", disp);
}

private long dispsize[] = { 0, 0, sizeof(short), sizeof(long) };

private mapsize(inst)
register long inst;
{
    int m;

    inst >>= 6;
    inst &= 03;
    switch (inst) {
	case 0:
	    m = 1;
	    break;

	case 1:
	    m = 2;
	    break;

	case 2:
	    m = 4;
	    break;

	default:
	    m = -1;
	    break;
    }
    return m;
}

private char suffix(size)
int size;
{
    char c;

    switch (size) {
	case 1:
	    c = 'b';
	    break;

	case 2:
	    c = 'w';
	    break;

	case 4:
	    c = 'l';
	    break;

	default:
	    panic("bad size %d in suffix", size);
    }
    return c;
}

/*
 * Print an address offset into a (local, static) string buffer.
 * This attempts to be symbolic.
 */

public char * ssymoff(destaddr)
Address destaddr;
{
    register long disp;
    register int len;
    register char *name;
    Symbol destblock;
    static char symbuff[ 80 ];

    if (destaddr > codestart + objsize) {
	sprintf( symbuff, "0x%lx", destaddr);
    } else {
        destblock = whatblock(destaddr, true);
        if (destblock == nil) {
	    sprintf( symbuff, "0x%lx", destaddr);
        } else {
	    disp = destaddr - rcodeloc(destblock);
	    name = symname(destblock);
	    len = strlen( name );
	    sprintf( symbuff, "%s", name);

	    if( disp ) {
		if (disp > 0) {
		    strcpy( &symbuff[len++], "+" );
		}
		dispstr(disp, &symbuff[len] );
	    }
	}
    }
    return symbuff;
}

/*
 * Print an address offset.  This attempts to be symbolic.
 */
public psymoff(destaddr)
Address destaddr;
{
    printf( "%s", ssymoff( destaddr ) );
}

/*
 * call_next_addr computes the return address of a call, given the
 * location of the call instruction.  The complications arise in
 * functions which return structures.
 *
 * Such a function returns the structure in a location specified by the
 * caller (via the "hidden struct parameter" field of the stack frame).
 * In order to make sure that the callee agrees with the caller about the
 * structure's size, calls to such functions are followed by "unimp"
 * instructions that indicate the length of structure the caller is
 * expecting.  If we have such a call, we must skip the unimp
 * instruction.
 */
private Address call_next_addr( startaddr , gothere)
Address startaddr;
Boolean gothere;
{
    Address rtn_addr ;
    long inst;

    rtn_addr = startaddr + 8;   /* assume a normal call */
    iread(&inst, rtn_addr, sizeof(inst));

    if( is_unimp_instr( inst ) ) {

        rtn_addr += 4;
    }

    if(gothere==true)
		stepto( rtn_addr ); /* skip to bkpt after call */
    return rtn_addr;
}


/*
 * Compute the next address that will be executed from the given one.
 * If "isnext" is true then consider a procedure call as straight line code.
 *
 * For the 68k, we just follow unconditional branches; for conditional branches
 * we continue execution to the current location and then single step
 * the machine.  (In the previous sentence, "we" means the opcode's
 * o->opfun, in particular, for the m68k, obranch, jsrop, jmpop, orts
 * and ortspop).
 *
 * The reason we go to all this trouble (rather than simply single-stepping
 * no matter what kind of instruction we have) is to speed things up, by
 * minimizing the number of process switches.
 *
 * This is both simpler and more difficult on a sparc (vs a Vax or
 * m68k).  The "real" nextaddr is always nPC and the instructions are
 * always 4 bytes long; however, in addition to simulating the travels of
 * the startaddr (PC), I must also simulate the next-addr (nPC).
 */

private Address nextaddr(startaddr, isnext)
Address startaddr;
Boolean isnext;
{
    Address new_addr ;
    long inst;
    Boolean act_like_call = false;
    Boolean act_like_toc = false;
	Address retaddr;

    iread(&inst, startaddr, sizeof(inst));

    if( is_jmpl_instr( inst ) ) {
	if( is_jmpl_call( inst ) ) {
	    act_like_call = true;
	} else {
	    act_like_toc = true;
	}
    } else if( is_call_instr( inst ) ) {
	act_like_call = true;
    } else if( is_br_instr( inst ) || is_trap_instr( inst ) ) {
	act_like_toc = true;
    }

    if( act_like_call ) {
	if( isnext ) {
	    return call_next_addr( startaddr , true);
	} else {
		retaddr = call_next_addr( startaddr , false);
	    act_like_toc = true;
	}
    }

    /*
     * Any other transfer-of-control instruction gets
     * executed for real.
     */
    if( act_like_toc ) {
		steppast( startaddr );
		new_addr = reg( REG_PC );
		if(act_like_call) {
			steppast(new_addr);         /* do the no-op */
			new_addr = reg( REG_PC );
    		if (not isbperr()) {
				printstatus();
    		}
			setcurfunc(whatblock(new_addr, true));
			bpact(false);
			if(inst_tracing == false and nosource(curfunc)
			  and canskip(curfunc) and nlhdr.nlines != 0
			and not is_pas_fort_main_rtn(curfunc)) {
				stepto(retaddr);
				new_addr = reg( REG_PC );
				bpact(false);
			}
		}
    } else {
	if( startaddr == reg(REG_PC)  &&  reg(REG_NPC) != reg(REG_PC) + 4 ) {
	    stepto( reg(REG_NPC) );
	    new_addr = reg( REG_PC );
	} else {
	    new_addr = startaddr + 4;
	}
    }

    return new_addr;
}

/*
 * Step to the given address and then execute one instruction past it.
 * Set instaddr to the new instruction address.
 */

public steppast(addr)
Address addr;
{
    stepto(addr);
    pstep(process);
    pc = reg(REG_PC);
    instaddr = pc;
}

/*
 * Enter a procedure by creating and executing a call instruction.
 * beginproc does not continue through the called routine.
 * If the "procedure" is a function which returns a structure, we
 * will get a nonzero struct_length.  When this happens, we must place
 * a "unimp #LEN" after the nop after the call.  Because of this, we
 * always have a 3-instruction calling sequence so that the return
 * address is always the same.
 *
 * For SPARC, this cannot be done quite as cleanly as on the m68k.
 * We cannot replace the original startup code before continuing to
 * run the called proc, because if the proc returns a struct, we will
 * have overwritten the length info (the unimp instruction) that .stret
 * requires.
 */

#define CALL_OP 1	/* value of opcode for CALL's "op" field */

public beginproc(p, struct_length, save_loc)
Symbol p;
int struct_length;
char *save_loc;
{
    char save_here[CALL_SIZE];
    struct {
	unsigned op : 2 ;
	int pc_rel : 30 ;
	long nop ;	/* the call always needs a nop for the delay slot */
	long unimp;	/* the call sometimes needs a struct-length */
    } call;
    long rel_dest;
    Address orig_pc;

    orig_pc = pc = CALL_LOC;

    if( save_loc == 0 ) save_loc = save_here;
    iread(save_loc, CALL_LOC, CALL_SIZE );

    /* The call instruction */
    call.op = CALL_OP ;
    rel_dest = rcodeloc(p) - pc;
    call.pc_rel = (rel_dest >> 2 );	/* it's a word offset */

    /* The delay-slot no-op */
    call.nop = 0x01000000 ; 		/* no-op instruction */

    /* The other no-op (or unimp, for structures) */
    if( struct_length ) {
	call.unimp = struct_length;
    } else {
	call.unimp = call.nop;
    }

    iwrite(&call, pc, sizeof(call));

    setreg(REG_PC, pc);
    setreg(REG_NPC, pc+4);
    pstep(process);			/* do the call itself */
    pc = reg(REG_PC);
    if (not isbperr()) {
	printstatus();
    }
    pstep(process);			/* do the no-op */

    /*
     * If no save loc was provided, then we can clean up
     * the saved stuff here; else, it's done in call_return,
     * in runtime.c
     */
    if( save_loc == save_here )
	iwrite(save_loc, CALL_LOC, CALL_SIZE );

    pc = reg(REG_PC);
    if (not isbperr()) {
	printstatus();
    }
    bpact(true);
}

typedef long Bpinst;

#if lint || cross
    /*  cross-debuggers don't need a "real" breakpoint. */
    Bpinst BP_OP;
#else
    extern Bpinst BP_OP;

    asm("_BP_OP: ta 1");
#endif lint

#endif sparc

#define BP_ERRNO    SIGTRAP     /* signal received at a breakpoint */

/*
 * Setting a breakpoint at a location consists of saving
 * the word at the location and poking a BP_OP there.
 *
 * We save the locations and words on a list for use in unsetting.
 */

typedef struct Savelist *Savelist;

struct Savelist {
    Address location;
    Bpinst save;
    short refcount;
    Savelist link;
};

private Savelist savelist;

/*
 * Set a breakpoint at the given address.  Only save the word there
 * if it's not already a breakpoint.
 */

public setbp(addr)
Address addr;
{
    Bpinst w, save;
    register Savelist newsave, s;

    for (s = savelist; s != nil; s = s->link) {
	if (s->location == addr) {
	    s->refcount++;
	    return;
	}
    }
    iread(&save, addr, sizeof(save));
    newsave = new(Savelist);
    if (newsave ==  0)
	fatal("No memory available. Out of swap space?");
    newsave->location = addr;
    newsave->save = save;
    newsave->refcount = 1;
    newsave->link = savelist;
    savelist = newsave;
    w = BP_OP;
    iwrite(&w, addr, sizeof(w));
}

/*
 * Unset a breakpoint; unfortunately we have to search the SAVELIST
 * to find the saved value.  The assumption is that the SAVELIST will
 * usually be quite small.
 */

public unsetbp(addr)
Address addr;
{
    register Savelist s, prev;

    prev = nil;
    for (s = savelist; s != nil; s = s->link) {
	if (s->location == addr) {
	    iwrite(&s->save, addr, sizeof(s->save));
	    s->refcount--;
	    if (s->refcount <= 0) {
		if (prev == nil) {
		    savelist = s->link;
		} else {
		    prev->link = s->link;
		}
		dispose(s);
	    }
	    return;
	}
	prev = s;
    }
    panic("unsetbp: couldn't find address %d", addr);
}

/*
 * Go through savelist, setting breakpoints in the code.  Because
 * the savelist entry exists, the code should contain these bp's.
 * However, because of the peculiarity of the process exit (dirty_exit
 * flag is set) that may not be so.  dirty_exit is set when
 * the process has exited abnormally and the status is NOT STOPPED.
 * Not all bp's may be set in the code at that time since we
 * may have been single-stepping and thus setting only the next bp.
 * When the exit occurs, we don't know what bp's are set and what
 * are not.  We would like to reset all of them according to savelist
 * but an attempt to write any instructions back into the
 * process will fail in pio().  Thus set the dirty_flag and since the
 * only thing the user can do at this stage is to run again, handle
 * the situation here.  The weird thing is that if one assumes
 * the bp's are set, one assumes wrong - they aren't.  However,
 * if one simply resets all bp's, the problem is that when the
 * instruction is read in from the code, it is a bp instruction!
 * I don't understand what's going on but believe it is related
 * to not waiting for the process to die after killing it in
 * pstart() [see Bugs_fixed3, #107].
 */

public resetanybps()
{
/*@*@*@*@*@*@*@*@*@*@*@*@*@*@*
    register Savelist s;
    Bpinst w;

    w = BP_OP;
    for (s = savelist; s != nil; s = s->link) {
    	iwrite(&w, s->location, sizeof(w));
	s->refcount--;	/* will be set back to whatever it was in setbp -
			   so this is really a no-op *@*
    }
*@*@*@*@*@*@*@*@*@*@*@*@*@*@*/
    register Savelist s, t;
    Bpinst w;

    for (s = savelist; s != nil; s = t->link) {
	w = s->save;
    	iwrite(&w, s->location, sizeof(w));
	t = s;
	dispose(s)
    }
    savelist = nil;
/*@*@*@*@*@*@*@*@*@*@*@*@*@*@*/
}

/*
 * Predicate to test if the reason the process stopped was because
 * of a breakpoint.
 */

public Boolean isbperr()
{
    /* TEMP! * printf( "isbperr says isfin %d and errnum %d (trap is %d)\n",
	/* TEMP! * isfinished(process), errnum(process), SIGTRAP );
	*/
    return (Boolean) (not isfinished(process) and errnum(process) == SIGTRAP);
}

public stepbug(addr)
{
	stepto(addr);
	pstep(process);
	pc = reg(REG_PC);
	instaddr = pc;
}

/*
 * Get some data and print it as a 'char'
 */
private print_char(f, addr, procread)
Format f;
Address addr;
Boolean procread;
{
	unsigned char	c = 0;

	if (addr != nil) {
		get_data(&c, f, addr, procread);
	}
	printf(f->printfstring, c);
}

/*
 * Get some data and print it as a 'short'
 */
private print_short(f, addr, procread)
Format f;
Address addr;
Boolean procread;
{
	unsigned short	s = 0;

	if (addr != nil) {
		get_data(&s, f, addr, procread);
	}
	printf(f->printfstring, s);
}

/*
 * Get some data and print it as a 'long'
 */
private print_long(f, addr, procread)
Format f;
Address addr;
Boolean procread;
{
	long	l = 0;

	if (addr != nil) {
		get_data(&l, f, addr, procread);
	}
	printf(f->printfstring, l);
}

/*
 * Get some data and print it as a 'float'
 */
private print_float(f, addr, procread)
Format f;
Address addr;
Boolean procread;
{
	union {
	    float	fval;
	    long	lval;
	} fu; /*sic*/

	if (addr != nil) {
		get_data(&fu, f, addr, procread);
	} else {
		fu.fval = 0;
	}
	printf(" 0x%08x   ", fu.lval);
	printf(f->printfstring, fu.fval);
}

/*
 * Get some data and print it as a 'double'
 */
private print_double(f, addr, procread)
Format f;
Address addr;
Boolean procread;
{
	union {
	    double	dval;
	    long	lval[2];
	} du;

	if (addr != nil) {
		get_data(&du, f, addr, procread);
	} else {
		du.dval = 0;
	}
	printf(" 0x%08x 0x%08x   ", du.lval[0], du.lval[1]);
	printf(f->printfstring, du.dval);
}

/*
 * Get some data and print in string format.
 * ARGSUSED
 */
private print_string(f, addr, procread)
Format f;
Address addr;
Boolean procread;
{
	char	c;

	if (procread == false) {
		error("Cannot print registers with `s'");
	}
	printf("\"");
	dread(&c, addr, sizeof(c));
	while (c != '\0') {
		printchar(c);
		addr++;
		dread(&c, addr, sizeof(c));
	}
	printf("\"");
}

/*
 * Get some data to be print out.
 * The 'procread' flag determines whether 'addr' is an address
 * within dbx's address space or an address within the debuggee's
 * address space.
 */
private get_data(buf, f, addr, procread)
char	*buf;
Format	f;
Address addr;
Boolean procread;
{
	if (procread) {
		dread(buf, addr, f->length);
	} else {
		bcopy((char *) addr, buf, f->length);
	}
}
