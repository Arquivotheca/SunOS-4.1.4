#ifndef lint
static  char sccsid[] = "@(#)eval.c 1.1 94/10/31 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Tree evaluation.
 */

#include "defs.h"
#include "tree.h"
#include "operators.h"
#include "eval.h"
#include "events.h"
#include "symbols.h"
#include "scanner.h"
#include "source.h"
#include "object.h"
#include "mappings.h"
#include "process.h"
#include "machine.h"
#include "runtime.h"
#include "dbxmain.h"

#ifdef mc68000
#  include "cp68881.h"
#  include "fpa_inst.h"
#elif i386
#  include "fpa_inst.h"
#endif mc68000

#include <signal.h>
#include <sys/time.h>

#define ITIMER_REAL 0
#define isroutine(s)	((s)->class == FUNC or (s)->class == PROC)

#ifndef public

#include "machine.h"

#define STACKSIZE 20000
#define TEMPSSIZE 200

typedef Char Stack;

/* recoverable error: no reason to punish users for our mistake. jpeck */
/* leave a buffer at the top of stack, for miscellaneous overusage, floats */
#define chksp0() \
    ((sp<&stack[0]) \
     ? stack_underflow() \
     : ((sp >= &stack[STACKSIZE-32]) ? stack_overflow() : 0))

#define chksp() {\
    if (sp < &stack[0]) { \
	stack_underflow(); \
    } else if (sp >= &stack[STACKSIZE-32]) { \
	stack_overflow(); \
    } \
}

/*  Push redefined, D. Teeple Dec 30 1987, remove memory fault on sparc
 *  due to pointer misalignment
*/
/*
 *#define push(type, value) { \
 *    ((type *) (sp += sizeof(type)))[-1] = (value); \
 *}
*/

#define push(type, value) {\
	type val = (value);\
	chksp();\
	mov(&val, sp, sizeof(type));\
	sp += sizeof(type); \
	}

/*  Pop redefined, D. Teeple, Jan 4 1987, remove memory fault problem
 *  due to misalignment.
*/
/*
 *#define pop(type) ( \
 *    (*((type *) (sp -= sizeof(type)))) \
 *)
*/

#define pop(type) (\
	(sp -= sizeof(type), \
	chksp0(),\
	mov(sp, vp, sizeof(type)),\
	(*(type *) (vp)))\
)

#define top(type) (\
	mov(sp-sizeof(type), vp, sizeof(type)),\
	(*(type *) (vp))\
)

#define alignstack() { \
    sp = (Stack *) (( ((int) sp) + sizeof(int) - 1)&~(sizeof(int) - 1)); \
}

#define isfloat(s) ( \
    s->class == RANGE and \
    s->symvalue.rangev.upper == 0 and s->symvalue.rangev.lower > 0 \
)

#define four_byte_float(s, len) ( \
    (len == 4) and isfloat(s) \
)

#define sing_prec_complex(s, len) ( \
    (len == 8) and isfloat(s) and istypename(s->type, "complex") \
)

#define round(x, m)		(((x) + (m) - 1)/(m)*(m))
#endif public
public stack_underflow()
{
    sp = &stack[0];
    error("stack underflow: indicates illegal usage or a dbx bug");
}
public stack_overflow()
{
    sp = &stack[0]; 
    error("stack overflow: expression too large or possible dbx bug");
}

private struct itimerval speed = {
	{ 0, 0 },			/* No interval */
	{ 0, 500000 }			/* 0.5 second timer */
};
public Stack stack[STACKSIZE];
public Stack tempstack[TEMPSSIZE];
public Stack *sp = &stack[0];
public Stack *vp = &tempstack[0]; /* pop temporary area */
public Boolean useInstLoc = false;
public int is_in_where;

private Symbol resolve_name();
private Symbol evalfunc();
private Boolean ifcond();
public	char *getenv();

/* repaired Feb 10 1988 DT - bug # 1008475 comparison of strings */
/* this is not a great solution, but the internal error goes     */
/* away.  The string values are popped & the address of sp is    */
/* returned in r.  This mimics in a way the C comparison of strings */
/* in which "abc"=="abc" is false since they are at different addresses */
/* other languages may require real string comparisons at some point */

#define poparg(n, r, fr) { \
    eval(p->value.arg[n]); \
    if (isreal(p->op)) { \
	fr = pop(double); \
    } else if (p->value.arg[n]->op == O_SCON) { \
        sp -= size(p->value.arg[n]->nodetype); \
	r = (int) sp; \
    } else if (isint(p->op)) { \
	r = popsmall(p->value.arg[n]->nodetype); \
    } \
}


#define Boolrep int	/* underlying representation type for booleans */
extern Boolean nocerror;

/*
 * Evaluate a parse tree leaving the value on the top of the stack.
 */

public eval(p)
register Node p;
{
    long r0, r1;
    double fr0, fr1;
    Address addr;
    long i, n;
    int len;
    Symbol s, f;
    Node n1, n2;
    Boolean b, b2;
    File file;
    Command action;
    char str[50];
    char *cp;
    float fl;

    checkref(p);
    if (debug_flag[2]) {
	printf(" evaluating %s at %d (0x%x)\n", opname(p->op), p, p );
    }
    switch (degree(p->op)) {
	case BINARY:
	    poparg(1, r1, fr1);
	    poparg(0, r0, fr0);
	    break;

	case UNARY:
	    poparg(0, r0, fr0);
	    break;

	default:
	    /* do nothing */;
    }
    switch (p->op) {
	case O_SYM:
	    s = p->value.sym;
	    if (s == retaddrsym) {
		i = return_addr();
		if (i != 0) {
		    push(long, i);
		} else {
		    push(long, fp_return_addr());
		}
	    } else {

		if ( s->class == IMPORT ) {
		  Symbol s_actual;

		    s_actual = findreal(s);
		    if( s_actual != nil ) {
			s = s_actual;
		    }
		}

		if (isvariable(s)) {
		    if (s != program and
			not isglobal(s) and
		        not isactive(container(s))) {
			error("\"%s\" is not active", symname(s));
		    }
		    if (isreg(s)) {
#ifdef mc68000
			if (is68881reg(s->symvalue.offset)) {
			    findregvar_68881(s, nil, &fr0);
			    push(double, fr0);
			} else if (isfpareg(s->symvalue.offset)) {
			    findregvar_fpa(s, nil, &fr0);
			    push(double, fr0);
			} else
#elif i386
			if (is80387reg(s->symvalue.offset)) {
			    fr0 = *((double *) regaddr(s->symvalue.offset));
			    push(double, fr0);
#ifdef WEITEK_1167
			} else if (isfpareg(s->symvalue.offset)) {
                            findregvar_fpa(s, nil, &fr0);
                            push(double, fr0);
#endif WEITEK_1167
			} else

#elif sparc
			if(isfpureg(s)) {
				find_fpureg(s, nil, &fr0);
			    push(double, fr0);
			} else if( rtype(p->nodetype)->class == RECORD ||
				  rtype(p->nodetype)->class == UNION) {
			    /* on a sparc if it's a register and an aggregate,
			     ** it had to be a parameter. The arg list actually
			     ** contains a pointer to the struct, space for the
			     ** struct was allocated by the caller. 
			     */
			    addr = findregvar(s, nil);
			    push(long, addr);
			} else
#endif sparc
			{
			    addr = findregvar(s, nil);
			    pushsmall(s, addr);
			    /* dbx handles floats as double internally */
	    		    if (isfloat(rtype(p->nodetype))) {
			        fl = pop(float);
			        fr0 = (double) fl;
			        push(double, fr0);
			    }
			}
		    } else {
		        addr = address(s, nil);
		    	push(long, addr);
		    }
		} else if (isblock(s)) {
		    push(Symbol, s);
		} else {
		    error("can't evaluate %s", aclassname(s));
		}
	    }
	    break;

	case O_LCON:
	    r0 = p->value.lcon;
	    pushsmall(p->nodetype, r0);
	    break;

	case O_FCON:
	    push(double, p->value.fcon);
	    break;

	case O_SCON:
	    len = size(p->nodetype);
/*  can't print a string unless the length is right, causes byte */
/*  alignment   DT  Feb 10 1988                                  */
/*	    len = round(len, sizeof(short)); removed Feb 10 '88  */
	    mov(p->value.scon, sp, len);
	    sp += len;
	    break;

	case O_INDEX:
    	    eval(p->value.arg[0]);
	    n = pop(long);
	    s = p->value.arg[0]->nodetype;
    	    eval(p->value.arg[1]);
	    i = evalindex(s, popsmall(p->value.arg[1]->nodetype));
	    push(long, n + i*size((rtype(s))->type));
	    break;

	case O_DOT:
	    s = p->value.arg[1]->value.sym;
	    n = lval(p->value.arg[0]);
	    push(long, n + (s->symvalue.field.offset div 8));
	    break;

	/*
	 * Get the value of the expression addressed by the top of the stack.
	 * Push the result back on the stack.
	 * For an expression like "*func()", "func()->mem", or "*(ptr + 1)"
	 * an extra level of indirection is built into the tree.
	 * This is because an expression node yeilds
	 * an actual pointer while a O_SYM node (of a pointer variable)
	 * yeilds a pointer to the pointer.  The special case here
	 * handles this situation.
	 */

	case O_INDIR:
	    n1 = nocasts(p->value.arg[0]);
	    if (n1->op != O_SYM and n1->op != O_DOT and
		n1->op != O_INDEX and n1->op != O_INDIR) {
		break;
	    }
	    /* Fall through */

	case O_RVAL:
	    addr = pop(long);
	    if (addr == 0) {
		error("reference through nil pointer");
	    }
	    if (p->op == O_INDIR) {
		len = sizeof(long);
	    } else {
		len = size(p->nodetype);
	    }
	    rpush(addr, len);
	    if (p->op == O_RVAL) {
		s = rtype(p->nodetype);
		push_aligned_bitfield(p->nodetype);	/* (if necessary) */
		push_double(s, len);			/* (if necessary) */
	    }
	    break;

	case O_AMPER:
	    if (p->value.arg[0]->op == O_SYM) {
		if (isblock(p->value.arg[0]->value.sym)) {
		    s = pop(Symbol);
		    push(long, rcodeloc(s));
		}
	    }
	    break;

        /*
         * Effectively, we want to pop n bytes off for the evaluated subtree
         * and push len bytes on for the new type of the same tree.
         */
        case O_TYPERENAME:
            n = size(p->value.arg[0]->nodetype);
            len = size(p->nodetype);
	    if (n <= sizeof(long) and len <= sizeof(long)) {
	        pushsmall(p->nodetype, popsmall(p->value.arg[0]->nodetype));
	    } else if (n == sizeof(double) and len == sizeof(long)) {
		push_double(p, len);
	    }
            break;

	case O_COMMA:
	    break;

	case O_ITOF:
	    push(double, (double) r0);
	    break;

	case O_FTOI:
	    push(long, (long) fr0);
	    break;

	case O_ADD:
	    /*
             * Pointer arithmetic is scaled, but do not scale
             * function addresses.
             */
            if (p->nodetype->class == PTR and
	      not isroutine(p->nodetype->type)) {
		/* already put in cannonical form */
		r1 *= size( p->nodetype->type );
	    }
	    push(long, r0+r1);
	    break;

	case O_ADDF:
	    push(double, fr0+fr1);
	    break;

	case O_SUB:
	    /*
             * Pointer arithmetic is scaled but do not scale
             * function addresses.
             */
	    if (p->value.arg[1]->nodetype->class == PTR and
	      not isroutine(p->nodetype->type) and
	      not isroutine(p->value.arg[1]->nodetype->type)) {
		/* must be PTR-PTR */
		push(long, (r0-r1)/size( p->nodetype->type ));
		break;
            } else if (p->nodetype->class == PTR and
	      not isroutine(p->nodetype->type)) {
		/* PTR - int */
		r1 *= size( p->nodetype->type );
	    }
	    push(long, r0-r1);
	    break;

	case O_SUBF:
	    push(double, fr0-fr1);
	    break;

	case O_QUEST:
	    if (isfloat(rtype(p->value.arg[0]->nodetype))) {
	        (pop(double))? eval(p->value.arg[1]): eval(p->value.arg[2]);
	    } else {
	        (pop(long))? eval(p->value.arg[1]): eval(p->value.arg[2]);
	    }
	    break;

	case O_BITAND:
	    push(long, r0 & r1);
	    break;

	case O_BITOR:
	    push(long, r0 | r1);
	    break;

	case O_BITXOR:
	    push(long, r0 ^ r1);
	    break;

	case O_LSHIFT:
	    push(long, r0 << r1);
	    break;

	case O_RSHIFT:
	    s = rtype(p->nodetype);
	    if (isunsigned(s)) {
		push(long, (unsigned) r0 >> r1);
	    } else {
	        push(long, r0 >> r1);
	    }
	    break;

	case O_COMPL:
	    push(long, ~r0);
	    break;

	case O_NEG:
	    push(long, -r0);
	    break;

	case O_NEGF:
	    push(double, -fr0);
	    break;

	case O_NOT:
	    push(long, !r0);
	    break;

	case O_SIZEOF:
	    push(long, size(p->value.arg[0]->nodetype));
	    break;

	case O_MUL:
	    push(long, r0*r1);
	    break;

	case O_MULF:
	    push(double, fr0*fr1);
	    break;

	case O_DIVF:
	    if (fr1 == 0) {
		error("error: division by 0");
	    }
	    push(double, fr0 / fr1);
	    break;

	case O_DIV:
	    if (r1 == 0) {
		error("error: div by 0");
	    }
	    s = rtype(p->nodetype);
	    if (isunsigned(s)) {
		push(long, (unsigned) r0 div r1);
	    } else {
	        push(long, r0 div r1);
	    }
	    break;

	case O_MOD:
	    if (r1 == 0) {
		error("error: mod by 0");
	    }
	    s = rtype(p->nodetype);
	    if (isunsigned(s)) {
		push(long, (unsigned) r0 mod r1);
	    } else {
	        push(long, r0 mod r1);
	    }
	    break;

	case O_LT:
	    push(Boolrep, r0 < r1);
	    break;

	case O_LTF:
	    push(Boolrep, fr0 < fr1);
	    break;

	case O_LE:
	    push(Boolrep, r0 <= r1);
	    break;

	case O_LEF:
	    push(Boolrep, fr0 <= fr1);
	    break;

	case O_GT:
	    push(Boolrep, r0 > r1);
	    break;

	case O_GTF:
	    push(Boolrep, fr0 > fr1);
	    break;

	case O_GE:
	    push(Boolrep, r0 >= r1);
	    break;

	case O_GEF:
	    push(Boolrep, fr0 >= fr1);
	    break;

	case O_EQ:
	    push(Boolrep, r0 == r1);
	    break;

	case O_EQF:
	    push(Boolrep, fr0 == fr1);
	    break;

	case O_NE:
	    push(Boolrep, r0 != r1);
	    break;

	case O_NEF:
	    push(Boolrep, fr0 != fr1);
	    break;

	case O_LOGAND:
	    push(Boolrep, r0 and r1);
	    break;

	case O_LOGOR:
	    push(Boolrep, r0 or r1);
	    break;

	case O_ASSIGN:
	    assign(p->value.arg[0], p->value.arg[1]);
	    break;

	case O_CHFILE:
	    if (p->value.scon[0] == '\0') {
		if (cursource != nil) {
		    printf("%s\n", shortname(cursource));
		} else {
		    error("No current source file");
		}
	    } else {
		file = opensource(p->value.scon);
		if (file != nil) {
		    fclose(file);
		    setsource(p->value.scon);
		    if (not file_isdebugged(p->value.scon)) {
			warning("File `%s' has not been compiled with the -g option",
			    p->value.scon);
		    } else  /* DTeeple March 21 1988 */
		    if (istool()) {
			dbx_printlines(cursrcline, cursrcline);
		    }
		} else {
		    error("can't read \"%s\"", p->value.scon);
		}
	    }
	    break;

	case O_CONT:
	    dbx_resume();
	    cont_cmd(p);
	    printnews();
	    break;

	case O_LIST:
	    if (p->value.arg[0]->op == O_SYM) {
		/* this is already done in check(p) */
		f = p->value.arg[0]->value.sym;
		addr = firstline(f);
		if (addr == NOADDR) {
		    error("no source lines for \"%s\"", symname(f));
		}
		setsource(srcfilename(addr));
		r0 = srcline(addr) - 5;
		r1 = r0 + 10;
		if (r0 < 1) {
		    r0 = 1;
		}
	    } else {
		/* this should be in check(p) */
		if ((p->value.arg[0]->op != O_LCON) or
		  (p->value.arg[1]->op != O_LCON)) {
		    error("list syntax is incorrect");
		}
		eval(p->value.arg[0]);
		r0 = pop(long);
		eval(p->value.arg[1]);
		r1 = pop(long);
	    }
	    /* D Teeple March 19 1988 fix bug 1008904
	     * test to see if we can print source lines before doing it
	     * should probably call "warning" instead of printf
	     */
	    if (!curfunc) {
		printf("warning: no current function.\n"); 
	    } else if (nosource(curfunc)) {
		printf("warning: no source for func \"%s\"\n",
		       idnt(curfunc->name));
	    } else {
		printlines((Lineno) r0, (Lineno) r1);
		cursrcline = r1 + 1;
	    }
	    break;

	case O_FUNC:
	    if (p->value.arg[0] == nil) {
		printname(curfunc);
		printf("\n");
	    } else { 		/* I know, this is horribly messy */
		f = p->value.arg[0]->value.sym;
		setcurfunc(f);
		addr = firstline(curfunc);
		if (addr != NOADDR) {
		    setsource(srcfilename(addr));
		    cursrcline = srcline(addr) - 5;
		    if (cursrcline < 1) {
			cursrcline = 1;
		    }
		}
		if (nosource(curfunc)) {
		    printf("warning: func \"%s\" is not compiled with the -g option\n",
		      idnt(f->name));
		} else if (istool()) {
		    dbx_printlines(cursrcline, cursrcline);
		}
	    }
	    break;

	case O_EXAMINE:
	    n1 = p->value.examine.beginaddr;
	    if (n1->op == O_AMPER and
	      n1->value.arg[0]->op == O_SYM and
	      isreg(n1->value.arg[0]->value.sym) and
	      not streq(p->value.examine.mode, "i")) {
		printreg(p);
	    } else {
	    	eval(n1);
	    	r0 = pop(long);
	    	if (p->value.examine.endaddr == nil) {
		    n = p->value.examine.count;
		    if (n == 0) {
		    	printvalue(r0, p->value.examine.mode);
		    } else if (streq(p->value.examine.mode, "i")) {
		    	printninst(n, (Address) r0);
		    } else {
		    	printndata(n, (Address) r0, p->value.examine.mode);
		    }
	    	} else {
		    eval(p->value.examine.endaddr);
		    r1 = pop(long);
		    if (streq(p->value.examine.mode, "i")) {
		    	printinst((Address)r0, (Address)r1);
		    } else {
		    	printdata((Address)r0, (Address)r1, p->value.examine.mode);
		    }
	    	}
	    }
	    break;

	case O_PRINT:
	    reset_indentlevel();
	    for (n1 = p->value.arg[0]; n1 != nil; n1 = n1->value.arg[1]) {
		if (n1->value.arg[0]->nodetype->class == FUNC or
		  n1->value.arg[0]->nodetype->class == PROC) {
		    error("Cannot print a function or procedure");
		}
		eval(n1->value.arg[0]);
		if (n1->value.arg[0]->op != O_LCON and
		  n1->value.arg[0]->op != O_FCON) {
		    prtree(n1->value.arg[0]);
		    printf(" = ");
		}
		printval(n1->value.arg[0]->nodetype);
	        printf("\n");
	    }
	    break;

	case O_PSYM:
	    if (p->value.arg[0] == nil) {
			dump_syms();
		} else if (p->value.arg[0]->op == O_SYM) {
		psym(p->value.arg[0]->value.sym);
	    } else {
		psym(p->value.arg[0]->nodetype);
	    }
	    break;

	case O_QLINE:
	    eval(p->value.arg[1]);
	    break;

	case O_STEP:
	    dbx_resume();
	    b = inst_tracing;
	    b2 = (Boolean) (not p->value.step.source);
	    n = p->value.step.stepamt;
	    multi_stepping = true;
	    for (i = 1; i <= n; i++) {
	        inst_tracing = b2;
	        if (p->value.step.skipcalls) {
		    next();
	        } else {
		    stepc();
	        }
	        inst_tracing = b;
	        useInstLoc = b2;
		if (i == n) {
		    multi_stepping = false;
		}
	        printnews();
	    }
	    break;

	case O_WHATIS:
	    reset_indentlevel();
	    if (p->value.arg[0]->op == O_SYM) {
		printdecl(p->value.arg[0]->value.sym);
	    } else {
		printdecl(p->value.arg[0]->nodetype);
	    }
	    break;

	case O_WHERE:
	    wherecmd(p->value.arg[0]->value.lcon);
	    break;

	case O_WHEREIS:
	    if (p->value.arg[0]->op == O_SYM) {
	    	printwhereis(p->value.arg[0]->value.sym);
	    } else {
	    	printwhereis(p->value.arg[0]->nodetype);
	    }
	    break;

	case O_WHICH:
	    if (p->value.arg[0]->op == O_SYM) {
	    	printwhich(p->value.arg[0]->value.sym);
	    } else {
	    	printwhich(p->value.arg[0]->nodetype);
	    }
	    printf("\n");
	    break;

	case O_ALIAS:
	    if (p->value.alias.newcmd == nil) {
		print_alias(nil);
	    } else if (p->value.alias.oldcmd == nil) {
		print_alias(p->value.alias.newcmd);
	    } else {
		enter_alias(p->value.alias.newcmd, p->value.alias.oldcmd);
	    }
	    break;

	case O_BUTTON:
	    dbx_button(p->value.button.seltype, p->value.button.string);
	    break;

	case O_MENU:
	    dbx_menu(p->value.button.seltype, p->value.button.string);
	    break;

	case O_UNBUTTON:
	    dbx_unbutton(p->value.scon);
	    break;

	case O_UNMENU:
	    dbx_unmenu(p->value.scon);
	    break;

	case O_CALL: {
	  Word *retregs;

	  if (!process_is_stopped()) {
		error("program not active");
	  }
	    s = evalfunc(p, true);
	    if ((s->language == findlanguage(".p") or
		 s->language == findlanguage(".mod")) and
	      istypename(s->type, "(void)")) {
		error("Can't call a Pascal or Modula2 procedure in an expression");
	    }
	    batchcmd = true;
	    callproc(s, p->value.arg[1]);
	    retregs = return_regs();

	    popenv();
	    s = s->type;
	    len = size(s);
	    if (canpush(len+1, "stack overflow: no space for call args")) {
		s = rtype(s);
		if (s->class == ARRAY or complex_type(s)) {
		    rpush(ss_return_addr(), len);
		    push_double(s, len);
		} else {
		    b = (Boolean) (s->class == RECORD or s->class == UNION);
		    pushretval(len, b, s, retregs);
		}
		if (len % 2 != 0) {
		    push(char, 0);	/* make sure sp is even */
		}
	    }
	    flushoutput();
	    if (len % 2 != 0) {
		pop(char);		/* pop extraneous byte */
	    }
  	    /* isstopped = true;  instead depend on popenv() up above (ivan) */
		restoretty(0);
	    break;
	}

	case O_CALLCMD:
	  if (!process_is_stopped()) {
		error("program not active");
	  }
	    s = evalfunc(p, true);
	    callproc(s, p->value.arg[1]);
	    procreturn();
		restoretty(0);
	    if (not batchcmd) {
		printstatus();
	    }
	    break;

	case O_CD:
	    cp = p->value.scon;
	    if (cp[0] == '\0') {
		cp = getenv("HOME");
	    }
	    nocerror = true;
	    if (chdir(cp) == -1) {
		perror(cp);
	    } else {
	        dbx_chdir(cp);
	    }
		remember_curdir();
	    nocerror = false;
	    break;

	case O_EDIT:
	    edit(p->value.scon);
	    break;

	case O_DETACH:
	    detach_process(process);
	    dbx_kill();
	    break;

	case O_KILL:
	    kill_process();
	    dbx_kill();
	    break;

	case O_PWD:
	    (void)system("pwd");
	    break;

	case O_DEBUG:
	    debug(p);
	    break;

	case O_DISPLAY:
	    display(p);
	    break;

	case O_UNDISPLAY:
	    undisplay(p);
	    break;

	case O_DUMP:
	    dump(p->value.arg[0]);
	    break;

	case O_HELP:
	    help((unsigned int) p->value.arg[0]->value.lcon);
	    break;

	case O_CATCH:
	case O_IGNORE: {
	    int signo;
	    int (*func)();
	    Boolean value;
	    char *msg;

	    if (p->op == O_CATCH) {
		value = true;
		func = pr_catch;
		msg = "ignore";
	    } else {
		value = false;
		func = pr_ignore;
		msg = "catch";
	    }
	    if (process == nil) {
		error("No active process to %s signals for", msg);
	    }
	    if (p->value.arg[0] == nil) {
		(*func)(process);
	    } else {
		for (n1 = p->value.arg[0]; n1 != nil; n1 = n1->value.arg[1]) {
		    signo = n1->value.arg[0]->value.lcon;
		    if(signo > 0 && signo < 32)
			psigtrace(process, signo, value);
		    else
			error("bad signal number: %d", signo);
	        }
	    }
	    break;
	}

	case O_RERUN:
	case O_RUN:
	    run();
	    break;

	case O_QUIT:
	    quit(0);
	    break;

	case O_SOURCE:
	    setinput(p->value.scon);
	    break;

	case O_SOURCES:
	    sources_cmd(p->value.scon);
	    break;

	case O_PRDBXENV:
	    dbx_prdbxenv();
	    break;

	case O_DBXENV:
	    eval_dbxenv(p);
	    break;

#ifdef mc68000
	case O_SET81:
	    set81(p->value.set81.fpreg->value.sym, p->value.set81.fp1,
	      p->value.set81.fp2, p->value.set81.fp3);
	    break;
#endif mc68000

	case O_STATUS:
	    status_cmd();
	    break;

	case O_TRACE:
	case O_TRACEI:
	    trace(p);
	    break;

	case O_STOP:
	case O_STOPI:
	    if (linesym == nil) {
		error("No active process to debug");
	    }
	    stop(p);
	    break;

	case O_ADDEVENT:
	    if (p->value.event.cond->value.arg[1]->value.sym == program) {
			/* when <exp> ... */
		action = build(O_TRACEON, 0, p->value.event.actions);
		action->value.trace.event = newevent();
		s = (Symbol) addtraceevent(action->value.trace.event,
		    p->value.event.cond, buildcmdlist(action));
		if (isstdin()) {
		    printevent(s);
		}
	    } else { 	/* when <proc> ..., when <line> ... */
	    	s = (Symbol) addstopevent(p->value.event.cond,
		    p->value.event.actions);
		if (isstdin()) {
		    printevent(s);
		}
	    }
	    break;

	case O_DELETE:
	    n2 = p->value.arg[0];
	    if (n2 == nil) {		/* delete all */
		delevent(0, true);
	    } else for (n1 = n2; n1 != nil; n1 = n1->value.arg[1]) {
	        eval(n1->value.arg[0]);
	        delevent(pop(long), false);
	    }
	    break;

	case O_ENDX:
	    endprogram();
	    break;

	case O_IF:
	    if (ifcond(p->value.event.cond)) {
		evalcmdlist(p->value.event.actions);
	    }
	    break;

	case O_ONCE:
	    event_once(p->value.event.cond, p->value.event.actions);
	    break;

	case O_STOPONCE:
	    stop_once(p->value.event.cond, p->value.event.actions);
	    break;

	case O_PRINTCALL_NOW:
	    /* really print the call after func has run prologue.
	     * this is called as the action in a STOPONCE command.
	     */
	    printtrcall(p->value.sym, whatblock(return_addr(), true));
	    delay(&speed);
	    break;

	case O_PRINTCALL:
	    /* This is before prologue, do nothing.
	     * We keep this opcode because printcmd relies
	     * on seeing it to flag a trace command.
	     */
	    break;

	case O_PRINTIFCHANGED:
	    seeifchanged(p->value.arg[0], true);
	    delay(&speed);
	    break;

	case O_PRINTRTN:
	    printrtn(p->value.sym);
	    break;
	case O_PRINTINSTPOS:
	    printf("tracei at 0x%04x\n", pc);
	    printinst(pc, pc);
	    printf("\n");
	    delay(&speed);
	    break;
	case O_PRINTSRCPOS:
	    getsrcpos();
	    if (p->value.arg[0] == nil) {
		printsrcpos();
		printf("\n");
		printlines(brkline, brkline);
	    } else if (p->value.arg[0]->op == O_QLINE) {
		if (p->value.arg[0]->value.arg[1]->value.lcon == 0) {
		    printf("tracei: ");
		    printinst(pc, pc);
		} else {
		    if (istool()) {
			dbx_trace();
		    } else {
		        printf("trace:  ");
		        printlines(brkline, brkline);
		    }
		    pr_display(false);
		}
	    } else {
		printsrcpos();
		printf(": ");
		eval(p->value.arg[0]);
		prtree(p->value.arg[0]);
		printf(" = ");
		printval(p->value.arg[0]->nodetype);
		printf("\n");
	    }
	    delay(&speed);
	    break;

	case O_STOPIFCHANGED:
	    seeifchanged(p->value.arg[0], false);
	    break;

	case O_STOPX:
	    if (!linelookup(pc)) /* force inst printing if needed */
	    	useInstLoc = true;
	    isstopped = true;
	    break;

	case O_TRACEON:
	    traceon(p->value.trace.inst, p->value.trace.event,
		p->value.trace.actions);
	    break;

	case O_TRACEOFF:
	    traceoff(p->value.lcon);
	    break;

	case O_CLEAR:
	    bp_clear(p->value.arg[0]);
	    break;

	case O_DOWN:
	    stack_down(p->value.arg[0]->value.lcon);
	    break;

	case O_UP:
	    stack_up(p->value.arg[0]->value.lcon);
	    break;

	case O_SEARCH:
	    search_src(p->value.scon, true);
	    break;

	case O_UPSEARCH:
	    search_src(p->value.scon, false);
	    break;

	case O_MAKE:
	    make(p);
	    break;

	case O_PRTOOLENV:
	    dbx_prtoolenv();
	    break;

	case O_BOTMARGIN:
	    dbx_botmargin(p->value.arg[0]->value.lcon);
	    break;

	case O_CMDLINES:
	    dbx_cmdlines(p->value.arg[0]->value.lcon);
	    break;

	case O_DISPLINES:
	    dbx_displines(p->value.arg[0]->value.lcon);
	    break;

	case O_FONT:
	    dbx_font(p->value.scon);
	    break;

	case O_SRCLINES:
	    dbx_srclines(p->value.arg[0]->value.lcon);
	    break;

	case O_TOPMARGIN:
	    dbx_topmargin(p->value.arg[0]->value.lcon);
	    break;

	case O_WIDTH:
	    dbx_width(p->value.arg[0]->value.lcon);
	    break;

	default:
	    panic("eval: bad op %d", p->op);
    }
    if (debug_flag[2]) {
	printf(" evaluated %s at %d (0x%x)\n", opname(p->op), p, p );
    }
}

/*
 * Evaluate a list of commands.
 */

public evalcmdlist(cl)
Cmdlist cl;
{
    Command c;

    foreach (Command, c, cl)
	evalcmd(c);
    endfor
}

/*
 * Evaluate an expression to yeild a function name
 */
private Symbol evalfunc(p, skip)
Node p;
Boolean skip;
{
    Node n;
    register Node n1;
    Symbol s;
    Address addr;

    n = p->value.arg[0];
    if (n->op != O_SYM) {
	n1 = n;
	if (n->op == O_RVAL) {
	    n1 = n->value.arg[0];
	}
	if (n1->nodetype->class != FUNC and
	    n1->nodetype->class != PROC and
            rtype(n1->nodetype)->class != FUNCT) {
	    error("Expression does not evaluate to a routine");
	}
	if (skip == false) n1 = n;
	eval(n1);
	addr = pop(Address);
	if (srcprocname(addr) == nil) {
	    error("No routine at address 0x%x", addr);
	}
	return whatblock(addr, true);
    } else {
	s = n->value.sym;
	if (s->class == VAR) {/* procedure variable */
	    p->value.arg[0] = build(O_RVAL, p->value.arg[0]);
	    return evalfunc(p, false);
	} else {
	    return(s);
	}
    }
}

/*
 * Push "len" bytes onto the expression stack from address "addr"
 * in the process.  If there isn't room on the stack, print an error message.
 */

public rpush(addr, len)
Address addr;
int len;
{
    if (canpush(len, "stack overflow: expression too large to evaluate")) {
	chksp();
	dread(sp, addr, len);
	if (read_err_flag) {
		if (is_in_where) {
			printf("bad data address=");
			mov(&addr, sp, len);
		} else {
			error("bad data address");
		}
	}
	sp += len;
    }
}

/*
 * Check if the stack has n bytes available.
 */

public Boolean canpush(n, msg)
Integer n;
String msg;
{
    return (Boolean)
	((sp + n < &stack[STACKSIZE-32])?1:(((Boolean) msg)?(error(msg),0):0));
}

/*
 * Push a small scalar of the given type onto the stack.
 */

public pushsmall(t, v)
Symbol t;
long v;
{
    register Integer s;

    s = size(t);
    switch (s) {
	case sizeof(char):
	    push(char, v);
	    break;

	case sizeof(short):
	    push(short, v);
	    break;

	case sizeof(long):
	    push(long, v);
	    break;

	default:
	    error("bad size %d in pushsmall", s);
    }
}

/*
 * Pop an item of the given type which is assumed to be no larger
 * than a long and return it expanded into a long.
 */

public long popsmall(t)
Symbol t;
{
    long r;
    Symbol base;

    base = rtype(t);
    switch (size(t)) {
	case sizeof(char):
	    r = (long) pop(char);
	    if (isunsigned(base)) {
		r &= 0xff;
	    }
	    break;

	case sizeof(short):
	    r = (long) pop(short);
	    if (isunsigned(base)) {
		r &= 0xffff;
	    }
	    break;

	case sizeof(long):
	    r = pop(long);
	    break;

	case sizeof(long) - 1:
	    sp += 1;
	    r = pop(long);
#ifdef i386	    
	    r <<= 8;
#endif i386
	    r >>= 8;
	    break;

	default:
	    /* see if they were trying to do an "illegal" operation on sets */
	    if (pascal_pgm or modula2_pgm) {
		if ((base != nil) and (base->class == SET)) {
		    r = size(t);
		    while (r--) {
			pop(char);
		    }
		    error("dbx does not currently handle this set operation");
		}
	    }
	    error("possible dbx bug. popsmall: size is %d", size(t));
    }
    return r;
}

/*
 * Evaluate a conditional expression.
 */

private Boolean ifcond(p)
Node p;
{
    register Boolean b;

    if (p == nil) {
	b = true;
    } else {
	eval(p);
	b = (Boolean) pop(Boolrep);
    }
    return b;
}

/*
 * Return the address corresponding to a given tree.
 */

public Address lval(p)
Node p;
{
    if (p->op == O_RVAL) {
	eval(p->value.arg[0]);
    } else {
	eval(p);
    }
    return (Address) (pop(long));
}

/*
 * Continue execution [at line_number] [signo]
 */

private cont_cmd(p)
Node p;
{
    register Node place;
    register Node sig;
    register int  line;
    register Address addr;
    int signo;
    String to, from;
    String tofile;

    place = p->value.cont.place;
    sig = p->value.cont.sig;
    if (sig != nil) {
	signo = sig->value.lcon;
    } else {
	signo = 0;
    }
    if (signo < 0 || signo > 32) {
	error("bad signal number: %d (can't continue)", signo);
    }
    if (place == nil) {
	cont(signo);
    } else {
	eval(place);
	line = pop(int);
	tofile = place->value.arg[0]->value.scon;
	if ((addr = objaddr(line, tofile, true)) != NOADDR) {
	    from = symname(whatblock(pc, true));
	    to = symname(whatblock(addr, true));
	    if (to != from) {
		printf("line #%d is in routine \"%s\" and current pc is in \"%s\" ",
		    line, to, from);
		error("(can't continue)");
	    }
	    cont_at(addr, signo);
	} else {
	    printf("no code at line #%d ", line);
	    error("(can't continue)");
	}
    }
}

/*
 * Process a trace command, translating into the appropriate events
 * and associated actions.
 */

public trace(p)
Node p;
{
    Node exp, place, cond;
    Node left;

    if (process == nil) {
	error("no active process");
    }
    exp = p->value.arg[0];
    place = p->value.arg[1];
    cond = p->value.arg[2];
    if (exp == nil && p->op == O_TRACE) {
	traceall(p->op, place, cond);		/* TRACE [in proc]: each line */
    } else if (exp == nil && p->op == O_TRACEI) {
	if(place->op == O_SYM)
	    traceall(p->op, place, cond);	/* TRACEI proc: each inst*/
	else
	    traceat(p->op, exp, place, cond);	/* TRACEI at ADDRESS */
    } else if (exp->op == O_QLINE or exp->op == O_LCON) {
	traceinst(p->op, exp, cond);		/* TRACE(i) line */
    } else if (place != nil and place->op == O_QLINE) {
	traceat(p->op, exp, place, cond);	/* TRACE exp AT line-number */
    } else {
	left = exp;
	if (left->op == O_RVAL or left->op == O_CALL) {
	    left = left->value.arg[0];
	}
	if (left->op == O_SYM and isblock(left->value.sym)) {
	    traceproc(left->value.sym, cond);	/* TRACE proc/func */
	} else {
	    tracedata(p->op, exp, place, cond);	/* TRACE(i) var [in proc] */
	}
    }
}

/*
 * Set a breakpoint that will turn on tracing.
 */

private traceall(op, place, cond)
Operator op;
Node place;
Node cond;
{
    Symbol s;
    Node event;
    Command action;
    Event e;

    if (place == nil) {
	s = program;
    } else {
	s = place->value.sym;
	if (s->class != FUNC and s->class != PROC) {
	    s = resolve_name(s);
	}
    }
    event = build(O_EQ, build(O_SYM, procsym), build(O_SYM, s));
    action = build(O_PRINTSRCPOS,
	build(O_QLINE, nil, build(O_LCON, (op == O_TRACE) ? 1 : 0)));
    if (cond != nil) {
	action = build(O_IF, cond, buildcmdlist(action));
    }
    action = build(O_TRACEON, (op == O_TRACEI), buildcmdlist(action));
    e = action->value.trace.event = newevent();
    addtraceevent(e, event, buildcmdlist(action));
    if (isstdin()) {
	printevent(e);
    }
}

/*
 * Set up the appropriate breakpoint for tracing an instruction.
 */

private traceinst(op, exp, cond)
Operator op;
Node exp;
Node cond;
{
    Node event, wh;
    Command action;
    Event e;

    if (exp->op == O_LCON and op != O_TRACEI) {
	wh = build(O_QLINE, build(O_SCON, cursource), exp);
    } else {
	wh = exp;
    }
    if (op == O_TRACEI) {
	event = build(O_EQ, build(O_SYM, pcsym), wh);
	action = build(O_PRINTINSTPOS, wh);
    } else {
	event = build(O_EQ, build(O_SYM, linesym), wh);
	action = build(O_PRINTSRCPOS, wh);
    }
    if (cond) {
	action = build(O_IF, cond, buildcmdlist(action));
    }
    e = addevent(event, buildcmdlist(action));
    if (isstdin()) {
	printevent(e);
    }
}

/*
 * Set a breakpoint to print an expression at a given line or address.
 */

private traceat(op, exp, place, cond)
Operator op;
Node exp;
Node place;
Node cond;
{
    Node event;
    Command action;
    Event e;

    if (op == O_TRACEI) {
	event = build(O_EQ, build(O_SYM, pcsym), place);
	action = build(O_PRINTINSTPOS, exp);
    } else {
	event = build(O_EQ, build(O_SYM, linesym), place);
	action = build(O_PRINTSRCPOS, exp);
    }
    if (cond != nil) {
	action = build(O_IF, cond, buildcmdlist(action));
    }
    e = addevent(event, buildcmdlist(action));
    if (isstdin()) {
	printevent(e);
    }
}

/*
 * Construct event for tracing a procedure.
 *
 * What we want here is
 *
 * 	when $proc = p do
 *	    if <condition> then
 *	        printcall; 
 *		stoponce p = $procsym do
 *		    printcallnow
 *	        once $pc = $retaddr do
 *	            printrtn;
 *	        end;
 *	    end if;
 *	end;
 *
 * Note that "once" is like "when" except that the event
 * deletes itself as part of its associated action.
 */

private traceproc(p, cond)
Symbol p;
Node cond;
{
    Node event;
    Command action;
    Cmdlist actionlist;
    Event e;

    if (p->class != FUNC and p->class != PROC) {
	p = resolve_name(p);
    }
    /* must make first action PRINTCALL, so printcmd isn't confused */
    actionlist = buildcmdlist(build(O_PRINTCALL, p));

    event = build(O_EQ, build(O_SYM, procsym), build(O_SYM, p));
    action = build(O_STOPONCE, event, buildcmdlist(build(O_PRINTCALL_NOW, p)));
    cmdlist_append(action, actionlist);

    event = build(O_EQ, build(O_SYM, pcsym), build(O_SYM, retaddrsym));
    action = build(O_ONCE, event, buildcmdlist(build(O_PRINTRTN, p)));
    cmdlist_append(action, actionlist);

    if (cond != nil) {
	actionlist = buildcmdlist(build(O_IF, cond, actionlist));
    }
    event = build(O_EQ, build(O_SYM, procsym), build(O_SYM, p));
    e = addevent(event, actionlist);
    if (isstdin()) {
	printevent(e);
    }
}

/*
 * Set up breakpoint for tracing data.
 */

private tracedata(op, exp, place, cond)
Operator op;
Node exp;
Node place;
Node cond;
{
    Symbol p;
    Node event;
    Command action;
    Event e;

    if (place == nil) {
	p = tcontainer(exp);
	if (p != nil and p->class == MODUS) {
            for (p != nil; p->class == MODUS; p = p->block);
        }
        if (p == nil)
            p = program;
    } else if (is_node_addr(place)) {
	event = build(O_EQ, build(O_SYM, pcsym), place);
    } else { 
	p = place->value.sym;
	if (p->class != FUNC and p->class != PROC) {
	    p = resolve_name(p);
	}
	event = build(O_EQ, build(O_SYM, procsym), build(O_SYM, p));
    }
    action = build(O_PRINTIFCHANGED, exp);
    if (cond != nil) {
	action = build(O_IF, cond, buildcmdlist(action));
    }
    action = build(O_TRACEON, (op == O_TRACEI), buildcmdlist(action));
    e = action->value.trace.event = newevent();
    event = build(O_EQ, build(O_SYM, procsym), build(O_SYM, p));
    addtraceevent(e, event, buildcmdlist(action));
    if (isstdin()) {
	printevent(e);
    }
}

/*
 * Setting and unsetting of stops.
 */

public stop(p)
Node p;
{
    Node exp, place, cond;
    Symbol s;
    Command action;
    Event e;
    String bpfile;
    Address addr;
    int	lineno;

    exp = p->value.arg[0];
    place = p->value.arg[1];
    cond = p->value.arg[2];
    if (exp != nil) {
	stopvar(p->op, exp, place, cond);
    } else if (cond != nil and (place == nil or place->op == O_SYM)) {
	if (place == nil) {
	    s = program;
	    action = build(O_IF, cond, buildcmdlist(build(O_STOPX)));
	    action = build(O_TRACEON, (p->op == O_STOPI), buildcmdlist(action));
	    cond = build(O_EQ, build(O_SYM, procsym), build(O_SYM, s));
	    e = action->value.trace.event = newevent();
	    addtraceevent(e, cond, buildcmdlist(action));
	    if (isstdin()) {
	        printevent(e);
	    }
	} else {
	    s = place->value.sym;
	    if (s->class == MODUS) {
		error("cannot stop in a module");
	    }
	    if (s->class != FUNC and s->class != PROC) {
		s = resolve_name(s);
	    }
	    action = build(O_IF, cond, buildcmdlist(build(O_STOPX)));
	    cond = build(O_EQ, build(O_SYM, procsym), build(O_SYM, s));
	    e = addstopevent(cond, buildcmdlist(action));
	    if (isstdin()) {
	        printevent(e);
	    }
	}
    } else if (place->op == O_SYM) {
	s = place->value.sym;
	if (s->class == MODUS) {
	    error("cannot stop in a module");
	}
	if (s->class != FUNC and s->class != PROC) {
	    s = resolve_name(s);
	}
	addr = firstline(s);
	if (addr == NOADDR) {
		addr = codeloc(s);
	}
	bpfile = srcfilename(addr);
	lineno = linelookup(addr);
	dbx_breakpoint(bpfile, lineno, true);
	cond = build(O_EQ, build(O_SYM, procsym), build(O_SYM, s));
	e = addstopevent(cond, buildcmdlist(build(O_STOPX)));
	if (isstdin()) {
	    printevent(e);
	}
    } else {
	stopinst(p->op, place, cond);
    }
}

private stopinst(op, place, cond)
Operator op;
Node place;
Node cond;
{
    Node event;
    Event e;
    Command action;
    char *bpfile;
    int lineno;

    if (op == O_STOP) {
	event = build(O_EQ, build(O_SYM, linesym), place);
    } else {
	if (place->op == O_QLINE) {
	    place = place->value.arg[1];
	}
	event = build(O_EQ, build(O_SYM, pcsym), place);
    }
    if (cond != nil) {
	action = build(O_IF, cond, buildcmdlist(build(O_STOPX)));
	e = addevent(event, buildcmdlist(action));
    } else {
    	e = addevent(event, buildcmdlist(build(O_STOPX)));
    }
    if (op == O_STOP) {
	next_executable(place);
	bpfile = place->value.arg[0]->value.scon;
	lineno = place->value.arg[1]->value.lcon;
	dbx_breakpoint(bpfile, lineno, true);
    }
    if (isstdin()) {
	printevent(e);
    }
}

/*
 * Given a place (file and lineno).
 * If the line does not contain executable code, find the next line
 * that does and patch the node.
 */
public next_executable(place)
Node place;
{
    char *bpfile;
    int lineno;
    Address addr;

    bpfile = place->value.arg[0]->value.scon;
    lineno = place->value.arg[1]->value.lcon;
    addr = objaddr(lineno, bpfile, true);
    place->value.arg[1]->value.lcon = srcline(addr);
}

/*
 * Implement stopping on assignment to a variable by adding it to
 * the variable list.
 */

private stopvar(op, exp, place, cond)
Operator op;
Node exp;
Node place;
Node cond;
{
    Symbol p;
    Node event;
    Command action;
    Event e;

    if (place == nil) {
	if (exp->op == O_LCON) {
	    p = program;
	} else {
	    p = tcontainer(exp);
	    if (p != nil and p->class == MODUS) {
	        for (p != nil; p->class == MODUS; p = p->block);
	    }
	    if (p == nil)
	        p = program;
	}
    } else {
	p = place->value.sym;
    }
    action = build(O_STOPIFCHANGED, exp);
    if (cond != nil) {
	action = build(O_IF, cond, buildcmdlist(action));
    }
    action = build(O_TRACEON, (op == O_STOPI), buildcmdlist(action));
    event = build(O_EQ, build(O_SYM, procsym), build(O_SYM, p));
    e = action->value.trace.event = newevent();
    addtraceevent(e, event, buildcmdlist(action));
    if (isstdin()) {
	printevent(e);
    }
}

/*
 * Divert output to the given file name.
 * Cannot redirect to an existing file.
 */

private int so_fd;
private Boolean notstdout;

public setout(filename)
String filename;
{
    File f;

    f = fopen(filename, "r");
    if (f != nil) {
	fclose(f);
	error("%s: file already exists", filename);
    } else {
	fflush(stdout);
	so_fd = dup(1);
	close(1);
	if (creat(filename, 0666) == -1) {
	    unsetout();
	    error("can't create %s", filename);
	}
	notstdout = true;
    }
}

/*
 * Revert output to standard output.
 */

public unsetout()
{
    fflush(stdout);
    close(1);
    if (dup(so_fd) != 1) {
	panic("standard out dup failed");
    }
    close(so_fd);
    notstdout = false;
}

/*
 * Determine if standard output is currently being redirected
 * to a file (as far as we know).
 */

public Boolean isredirected()
{
    return notstdout;
}

/*
 * All floats pushed on the stack are represented internally as
 * doubles.  Here's where the conversion takes place.
 */

public push_double(s, len)
Symbol s;
int len;
{
    float fl;
    double fr0, fr1;

    if (four_byte_float(s, len)) {
    	fl = pop(float);
    	fr0 = (double) fl;
    	push(double, fr0);
     } else if (sing_prec_complex(s, len)) {
	fl = pop(float);
	fr0 = (double) fl;
	fl = pop(float);
	fr1 = (double) fl;
	push(double, fr1);
	push(double, fr0);
    }
}

/*
 * If a bit field has been pushed; align before printing or evaluating
 * an expression.
 *
 * ivan Mon Feb 13 1989
 * The name 'push_aligned_bitfield()' is a MISNOMER! Nothing gets pushed,
 * just the top of the stack gets modified!
 */

#undef SHIFT_OFF
#ifdef i386
#define SHIFT_OFF (off)
#else
#define SHIFT_OFF ((- (len + off)) & (BITSPERBYTE - 1)) /* (-(len+off) mod BITSPERBYTE) */
#endif

/* see also `c.c`p_str_bitfield */

public push_aligned_bitfield(s)
Symbol s;
{
    register int i, j, k, len, off;

    if (isbitfield(s)) {
		len = s->symvalue.field.length;
		off = (s->symvalue.field.offset % BITSPERBYTE);
		j = (off + len + BITSPERBYTE - 1) div BITSPERBYTE;
		k = SHIFT_OFF;
		if (j <= sizeof(char)) {
			i = pop(char);
			i >>= k;
			i &= ((1 << len) - 1);
			push(char, i);
		} else if (j <= sizeof(short)) {
			i = pop(short);
			i >>= k;
			i &= ((1 << len) - 1);
			push(short, i);
		} else {
#ifndef i386
		    if (j < sizeof(long)) k += 8;
#endif i386
			i = pop(long);
			i >>= k;
			i &= ((1 << len) - 1);
			push(long, i);
		}
    }
}

private Symbol resolve_name(s)
register Symbol s;
{
    register Symbol s2;

    s2 = nil;
    if (s->class == MODULE) {
    	find(s2, s->name) where
	    ((s2->class == FUNC or s2->class == PROC) and
	      s2->block == s)
    	endfind(s2)
    } else if (s->class == FVAR) {
    	find(s2, s->name) where
 	    (s2->class == FUNC)
    	endfind(s2)
    }
    if (s2 == nil) {
	error("name clash for \"%s\" (set scope using \"func\")",
	  idnt(s->name));
	/* no return */
    }
    return s2;
}

/*
 * Delay for some amount of time.
 */
private delay(speedp)
struct itimerval *speedp;
{
    setitimer(ITIMER_REAL, speedp, nil);
    pause();
}

/*
 * Return the tracing speed
 */
public	double get_speed()
{
	double d;

	d = (double) speed.it_value.tv_sec +
	  (double) speed.it_value.tv_usec * 0.000001;
	return(d);
}

/*
 * Set the tracing speed.
 */
public set_speed(trace_speed)
double trace_speed;
{
	speed.it_value.tv_sec = (int) trace_speed;
	speed.it_value.tv_usec = (trace_speed - speed.it_value.tv_sec) *
	  1000000;
}

/*
 *	set one environment variable
 */
public setenv(var, value)
char *var, *value;
{
	char	*string;

	if (var == NULL) {
		var = "";
	}
	if (value == NULL) {
		value = "";
	}
	string = malloc(strlen(var) + strlen(value) + 10);
	if (string == 0)
		fatal("No memory available. Out of swap space?");
	(void)sprintf(string, "%s=%s", var, value);
	putenv(string);
}

