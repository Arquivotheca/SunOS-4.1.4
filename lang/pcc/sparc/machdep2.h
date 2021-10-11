/*	@(#)machdep2.h 1.1 94/10/31 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */


#define NESTCALLS /* try this out for fun */

/* stack allocation macros */
/*
 * a note on our non-traditional allocation scheme:
 * allocoff -- amount of allocated (local) storage for this block
 * maxalloc -- max over all allocoff's so far this procedure
 * baseoff -- base of pass 2 temporary allocation
 * tmpoff -- current temporary offset (per expression)
 * maxoff -- max over all tmpoff's so far this procedure.
 * strtmpoff -- current structured temporary offset (per expression)
 *	        [strtmpoffs are relative to a symbol LST<ftnno>,
 *		equated at eobl2 time to the offset of a secondary
 *		temp area for large structures.]
 * maxstrtmpoff -- max over all strtmpoff's so far this procedure.
 *
 * allocoff and maxalloc are managed in reader.c using these macros.
 * tmpoff is reset in reader.c, but the max is taken in the routine
 * freetemp in allo2.c
 */
# define TMP_BBEG   {\
	allocoff = (unsigned) aoff;\
}

# define TMP_BFN    {\
	maxalloc = allocoff;\
	maxoff = baseoff;\
	maxstrtmpoff = 0;\
}

# define TMP_NFN    {\
	if (allocoff>maxalloc) maxalloc = allocoff;\
	if (tmpoff>maxoff) maxoff = tmpoff;\
	if (strtmpoff>maxstrtmpoff) maxstrtmpoff = strtmpoff;\
}

# define TMP_BEND   {\
	SETOFF(maxoff, ALSTACK);\
	SETOFF(maxstrtmpoff, ALSTACK);\
}

# define TMP_EXPR   {\
	tmpoff = baseoff;\
	strtmpoff = 0;\
}

extern int maxalloc, allocoff;
extern int maxstrtmpoff, strtmpoff;

extern SUTYPE fregs;

/* evaluation of expensive subtrees into temporary locations */
extern int storall;
# define SETSTO(x,y) setsto(x,y)
# define ORDER_STOTREE { int tt; NODE * tp; tp = stotree; tt = tp->in.rall; tp->in.rall = storall; order( tp, stocook ); tp->in.rall = tt; }

# define BYTEOFF(x) ((x)&03)
# define wdal(k) (BYTEOFF(k)==0)
# define BITOOR(x) ((x)>>3)  /* bit offset to oreg offset */


# define REGSZ 64        /* 8 ins, 8 locals, 8 outs, 8 globals, 32 floating */
# define TMPREG SPREG	 /* temps off of sp (rather than fp) */
# define TMPMAX (0xfff)  /* temps must be addressable by 13-bit offset */
char tmpname[20];	 /* symbolic part of temp offsets */
# define R2REGS		 /* have 2-reg OREG's */
# define STOARG(p)
# define STOFARG(p)
# define STOSTARG(p)
# define genfcall(a,b) gencall(a,b)
# define MYREADER	myreader
# define OPEREG( typ ) (ISFLOATING(typ)?(INBREG|INTBREG):(INAREG|INTAREG))

void   mktempnode(/*p,t*/);	/* make node (*p) into a temp of type t */
NODE * mktempaddr();
NODE * build_in(); /* build interior tree node */
NODE * build_tn(); /* build terminal tree node */

# define MEMADDR SOREG|STARNM	/* directly addressable things */
# define SICON (SPECIAL+101) 	/* constants from -2^13..2^13-1 */
# define SSOURCE (SPECIAL+102)	/* SICON or REGISTER */
# define SSNAME  (SPECIAL+103)	/* NAME of small object */
# define SXCON   (SPECIAL+104)	/* address of small object */
# define SFCON	 (SPECIAL+105)	/* f-p constant */
# define SINAME	 (SPECIAL+106)	/* small NAME of object */
# define SONES	 (SPECIAL+107)	/* enought 1s to fill a field */
#define SNONPOW2 (SPECIAL+108)
#define SPEC_PVT (SPECIAL+110)
# define SDOUBLE (SPECIAL+111)	/* something I can use as the address of a ldd/std */
# define SDOUBLEO (SPECIAL+112)	/* something I can use as the address of a ldd/std */
# define SFLD8_16 (SPECIAL+113)	/* field with {byte|short} size and alignment */
# define SCOMPLR  (SPECIAL+114) /* COMPL over AREG (for andn instruction) */
# define SAREGPAIR (SPECIAL+115) /* even-aligned AREG pair */

/*
 * flags for in and tn node variants.
 */
#define FLAG_DOUBLE_ALIGNED	1

/*
 * option flags
 */
extern int chk_ovfl;		/* 1 => overflow checking is enabled */

/* 
 * this stuff is for dealing with compound su-numbers: they are 4 chars
 * in a struct. Three are currently used.
 * They represent the number of registers of each of the (three) types
 * necessary to carry out a calculation.
 */

#define SUPRINT( v ) printf(", SU = (%d,%d,%d)\n", v.d, v.f, v.calls)
#define SUTOOBIG( v ) (v.d>fregs.d || v.f>fregs.f || v.calls!=0)
#define SUSUM( v ) (v.d + v.f)
#define SUTEST( v ) (SUSUM(v) != 0)
#define SUGT( a, b) (SUSUM(a) > SUSUM(b))

#define NNCON(p)\
    ((p)->in.op == ICON && ((p)->tn.name == NULL || (p)->tn.name[0] == '\0'))
