/*	@(#)defs.h 1.1 94/10/31 1984 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Public definitions, common to all.
 */

#include <stdio.h>
#include <vfork.h>

/*
 * <sys/core.h> requires the kernel's names (sun2, sun3 or sun4)
 * instead of the user-land predefines (mc68000/mc68010/mc68020/sparc).
 */
#ifdef mc68020
#  define sun3 1
#else
#  ifdef mc68000
#     define sun2 1
#  endif
#endif

#ifdef sparc
#   define sun4 1
#endif sparc

#define isreg(s)	    ((char)s->level < (char)0)

#undef new
#define new(type)           ((type) calloc(sizeof(struct type), 1))
#define newarr(type, n)     ((type *) malloc((unsigned) (n) * sizeof(type)))
#define dispose(ptr)        { free((char *) ptr); ptr = 0; }

#define public
#define private static

#define ord(enumcon) ((unsigned int) enumcon)
#define nil 0
#define and &&
#define or ||
#define not !
#define div /
#define mod %
#undef max
#undef min
#define max(a, b)    ((a) > (b) ? (a) : (b))
#define min(a, b)    ((a) < (b) ? (a) : (b))

#define assert(b) { \
    if (not(b)) { \
	panic("assertion failed at line %d in file %s", __LINE__, __FILE__); \
    } \
}

#define badcaseval(v) { \
    panic("unexpected value %d at line %d in file %s", v, __LINE__, __FILE__); \
}

#define checkref(p) { \
    if (p == nil) { \
	panic("reference through nil pointer at line %d in file %s", \
	    __LINE__, __FILE__); \
    } \
}

typedef int Integer;
typedef char Char;
typedef double Real;
typedef enum { false, true } Boolean;
typedef char *String;

#define streq(s1, s2)   (strcmp(s1, s2) == 0)

typedef FILE *File;
typedef int Fileid;
typedef String Filename;

typedef int Lineno;

#define get(f, var) fread((char *) &(var), sizeof(var), 1, f)
#define put(f, var) fwrite((char *) &(var), sizeof(var), 1, f)

#undef FILE

extern long atol();
extern double atof();
extern char *malloc();
extern char *calloc();
extern char *realloc();
extern char *strncpy();
extern String strcpy(), index(), rindex();
extern int strlen();

extern String cmdname;
extern String errfilename;
extern short errlineno;
extern int debug_flag[];
