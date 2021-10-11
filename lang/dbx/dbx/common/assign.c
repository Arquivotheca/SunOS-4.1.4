#ifndef lint
static	char sccsid[] = "@(#)assign.c 1.1 94/10/31 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Perform assignments to variables
 */

#include "defs.h"

#ifdef mc68000
#  include "cp68881.h"
#  include "fpa_inst.h"
#endif mc68000

#include "eval.h"
#include "machine.h"
#include "symbols.h"
#include "runtime.h"
#include "c.h"

#define BITSPERCHAR	8

/*
 * Assign the value of an expression to a variable (or term).
 */
public assign(var, exp)
Node var;
Node exp;
{
    Address addr;
    int varsize;
    char cvalue;
    short svalue;
    long lvalue;
    int expsize;
    Symbol s, expr;
    Frame frp;
    float f;
    double d, d2;

    if (not compatible(var->nodetype, exp->nodetype)) {
	warning("incompatible types");
    }
    varsize = size(var->nodetype);
    if (isbitfield(var->nodetype)) {
	bitfield_assign(var, exp);
	return;
    }
    if (var->op == O_SYM) {
	s = var->value.sym;
	if (s != retaddrsym and isvariable(s) and isreg(s)) {
	    frp = findframe(s->block);
	    if (frp == nil) {
		error("stack frame not accessible");
	    } else {
	    	addr = findregaddr(frp, s->symvalue.offset);
	    }
	    if (addr <= (Address) LASTREGNUM) {
	        if (addr == nil) {
			warning("Can not set that register");
		} else {
			assignreg(s, addr, exp);
		}
		return;
	    } else {	/* else addr contains address on stack of reg var */
		if (varsize < sizeof(Word)) {
		    addr += (sizeof(Word) - varsize);
		}
	    }
	} else {
    	    addr = lval(var);
	}
    } else {
    	addr = lval(var);
    }
    s = rtype(var->nodetype);
    if (s->class == READONLY) {	/* pascal conformant array dimension */
	error("can't assign to a read-only parameter");
    }
    eval(exp);
    
    /* D Teeple Mar 3 1988  part of bug 1005218 fix - see below - */
    
    expr = rtype(exp->nodetype);
    expsize = size(expr);
    /*
     * Adjust the stack so that the type (&size) of the expression
     * matches that of the variable.  If expsize < sizeof the variable,
     * we must adjust expsize accordionly, to prevent zero-fill.
     */
    if (sing_prec_complex(s, varsize)) {
	d = pop(double);
	d2 = pop(double);
	f = (float) d2;
	push(float, f);
	f = (float) d;
	push(float, f);	
    } else if( isfloat(s) ) {
	/* handle mixed mode assignment */
	if( isfloat( rtype(exp->nodetype) ) ) {
	    d = pop(double);
	} else { /* expression is integral */
	    if (expsize == sizeof(char)) {
		lvalue = pop(char);
	    } else if (expsize == sizeof(short)) {
		lvalue = pop(short);
	    } else {
		lvalue = pop(long);
	    }
	    d = lvalue;		/* widen */
	}
	if (four_byte_float(s, varsize)) {
	    f = (float) d;
	    push(float, f);
	    expsize = sizeof(float);
	} else {
	    push(double, d);
	    expsize = sizeof(double);
	}
    }
   	/* D Teeple Mar 3 1988 */	
	/* do string of length 2 (char+terminator) to char coercion */
	/* to fix bug # 1005218                                     */
	/* 880817 PEB added check if expr->type nil */
	/* (fixes 1012437 1012439) */
     if ( expr->type and istypename(expr->type,"$char") and 
 	    	expsize == sizeof(char)+1 ) {
    	pop(char);
    	expsize = sizeof(char);
    	}
  	
    /* process expr when var is a string */
    /* no guarantees, but we hope that the expr is also a string! */
    if ((s->class == ARRAY and istypename(s->type, "char"))
	or (s->class == VARYING)) {
	/* convert int to char */
	if (expsize == 4 and exp->nodetype == t_int) {
	    lvalue = pop(long);
	    push(char, (char) lvalue);
	    expsize = 1;
	} else if (cvalue = pop(char)) { /* convert asciiz to nstring */
	    push(char, cvalue);
	} else {
	    --expsize;		/* pop the '\0' appended by the scanner */
	}
	if (s->class == VARYING) { /* VARYING, write current size */
	    /* set varsize = number of bytes to actually copy */
	    if (expsize > s->symvalue.varying.length) 
		varsize = s->symvalue.varying.length;
	    else
		varsize = expsize; /* don't pad varying strings */
	    dwrite(&varsize, addr, sizeof(int));
	    addr += sizeof(int);
	} else {		/* space fill pascal and fortran */
	    if (((var->nodetype)->language == findlanguage(".p")) or
		((var->nodetype)->language == findlanguage(".f"))) {
		if (expsize < varsize) 
		    warning("padding string to length %d", varsize);
		while (expsize < varsize) {
		    expsize++;
		    push(char, ' ');
		}
	    } else {		/* otherwise zero terminate */
		expsize++;
		push(char, (char) 0);
	    }
	}
	/* expsize = varsize = min(varsize,expsize) */
	if (expsize < varsize) {
	    varsize = expsize;
	} else if (expsize > varsize) {
	    warning("truncating string to %d characters", varsize);
	    sp -= (expsize - varsize);
	    expsize = varsize;
	}
    }  /* end of special processing for strings */

    if ((varsize <= sizeof(long))) {  /* small (numeric) objects */
	/* allow for size coercion, sign extension, etc. */
	/* unfortunately, various other objects may fall into here..*/
	/* this is basically the same functionality as popsmall (see also) */
	/* but here we don't panic if the sizes mismatch or sets or whatever */
	if (expsize == sizeof(char)) {
	    lvalue = pop(char);
	    if (isunsigned(expr)) lvalue &= 0xff;
	} else if (expsize == sizeof(short)) {
	    lvalue = pop(short);
	    if (isunsigned(expr)) lvalue &= 0xffff;
	} else if (expsize == (sizeof(long) - 1)) {
	    sp += 1;
	    lvalue = pop(long);
#ifdef i386	    
	    lvalue <<= BITSPERBYTE;
#endif i386
	    lvalue >>= BITSPERBYTE;
	} else if (expsize == sizeof(long)) {
	    lvalue = pop(long);
	} else if (expsize > varsize) { /* if we get here, it must be true */
	    warning("%s holds only first %d bytes of expression",
		    ((var->op == O_SYM)? symname(var->value.sym): "var"),
		    varsize);
	    sp -= (expsize - varsize);
	    lvalue = pop(long);
	    /* expsize = varsize; */
	}
        if (s->class == RANGE and
            s->symvalue.rangev.upper >= s->symvalue.rangev.lower) {
            if (lvalue < s->symvalue.rangev.lower or
                lvalue > s->symvalue.rangev.upper)
                warning ("out of range assignment");
        }
	switch (varsize) {
	    case sizeof(char):
		cvalue = lvalue;
		dwrite(&cvalue, addr, varsize);
		break;

	    case sizeof(short):
		svalue = lvalue;
		dwrite(&svalue, addr, varsize);
		break;

	    case sizeof(long):
		dwrite(&lvalue, addr, varsize);
		break;

	    default: /* this must be sizeof(long) - 1 */
#ifndef i386
		lvalue <<= BITSPERBYTE;
#endif i386
		dwrite(&lvalue, addr, varsize);
		break;
	    }

    } else { /* for varsize > sizeof(long) */
	if (expsize > varsize) {
	    warning("%s holds only first %d bytes of expression",
		    ((var->op == O_SYM)? symname(var->value.sym): "var"),
		    varsize);
	    sp -= (expsize - varsize);
	} else if (expsize < varsize) {
	    warning("expression fills only %d of %d bytes in %s",
		    expsize, varsize,
		    ((var->op == O_SYM)? symname(var->value.sym): "var"));
	    varsize = expsize;
	}
#ifdef mc68000
	if (is68881reg(var->value.sym->symvalue.offset)) {
	    /*
	     * Assigning to a saved 68881 register.
	     * Pop the double off the stack and convert to extended.
	     */
	    d = pop(double);
	    dtofpr((double *) &d, (ext_fp *) sp);
	    varsize = sizeof(ext_fp);
	}
	else
#endif mc68000
	{
	    sp -= varsize;
	}
	dwrite(sp, addr, varsize);
    }
}

/*
 * Assign a value to a register.
 * Pop the current value off the stack.
 * Convert to the appropriate type, determined
 * by the variable's attributes from the symbol table.
 */
private assignreg(s, addr, exp)
Symbol s;
Address addr;
Node exp;
{
    double d;
    int i;
    Boolean floatsource;

    /* evaluate source, remembering its type */
    eval(exp);
    if (isfloat(rtype(exp->nodetype))) {
        d = pop(double);
	floatsource = true;
    } else {
	i = popsmall(exp->nodetype);
	floatsource = false;
    }
#ifdef mc68000
    if (is68881reg(addr)) {
	/* 68881 reg -- type is extended precision */
	if (not floatsource) {
	    d = i;
	}
    	dtofpr(&d, (ext_fp *) regaddr(addr));
    }
    else
#endif mc68000
    {
	/* other registers -- get the type from the symbol table */
	if (isfloat(rtype(s))) {
	    /* reg has floating type */
	    if (not floatsource) {
		d = i;
	    }
	    if (size(s) == sizeof(float)) {
		*((float*)regaddr(addr)) = (float)d;
	    } else {
		*((double*)regaddr(addr)) = d;
	    }
	} else {
	    /* reg has integral type */
	    if (floatsource) {
		i = (int)d;
	    }
	    setreg(addr, i);
	}
    }
}

/*
 * Bitfield assignment.
 * Round the address of the bitfield up to the nearest
 * byte boundary.  Read an integer encompassing the whole
 * bitfield.  Mask out the old value, or in the new value,
 * and replace the integer.
 */
bitfield_assign(var, exp)
Node var;
Node exp;
{
    Address addr;
    Symbol field, s;
    int val;
    int oldval;
    int shift_left;
    int mask;
    Stack *origsp;

    /*
     * Read the integer containing the bitfield.
     */
    field = var->value.arg[1]->value.sym;
    addr = lval(var);
    /* val = pop(int); */
    /* now that this is used by other than c-bitfields,
     * can't know the size of exp until it is computed, jp 08/11/89
     */
    origsp = sp;
    eval(exp);
    switch (sp - origsp) {
      case sizeof( char): val = pop( char); break;
      case sizeof(short): val = pop(short); break;
      case sizeof( long): val = pop( long); break;
      case sizeof( long) - 1:
	sp += 1;
	val = pop(long);
#ifdef i386	
	val <<= BITSPERBYTE;
#endif i386
	val >>= BITSPERBYTE; break;
      default:
	warning("attempt to assign %d bytes to bitfield", sp - origsp);
	sp = origsp + sizeof(long);
	val = pop( long); break;
    }
    dread((char *) &oldval, addr, sizeof(int));

    /*
     * Clear the old value of the bitfield.
     */
#ifdef i386
    shift_left = field->symvalue.field.offset % BITSPERCHAR;
#else
    shift_left = (sizeof(int) * BITSPERCHAR) - 
      field->symvalue.field.offset % BITSPERCHAR - 
      field->symvalue.field.length;
#endif
    mask = (1 << field->symvalue.field.length) - 1;
    if (val & ~mask) {
	warning("Bitfield `%s' not wide enough; %d truncated to %d",
	  symname(field), val, val & mask);
    }
    val &= mask;
    mask <<= shift_left;
    oldval &= ~mask;

    /*
     * Insert the new value.
     */
    val <<= shift_left;
    oldval |= val;
    dwrite((char *) &oldval, addr, sizeof(int));
}
