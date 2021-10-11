#ifndef lint
static char sccsid[] = "@(#)inline.c 1.1 94/10/31 SMI";
static char RELEASE[] = "@(#)RELEASE SunOS compilers 4.1";
#endif

#include <stdio.h>
#include <ctype.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>

/*
 *  inline [-w] [-v] [-o outputfile] [-i inlinefile]
 *	[<CPU options>] [<FPU options>] [sourcefile(s)] ...
 *
 *	This program is little more than an over-embellished sed script.
 *	It inline-expands call instructions in one or more <sourcefiles>
 *	from one or more <inlinefiles>.  If no <inlinefiles> are specified,
 *	the <sourcefiles> are simply concatenated to stdout.  If no
 *	<sourcefiles> are specified, the default input is stdin.
 *
 *	If [-w] is specified, inline displays warnings for duplicate
 *	definitions.
 *
 *	If [-v] is specified, inline displays names of routines that
 *	were actually inline-expanded in the sourcefile.
 *
 *	Inline expands the subroutine call instructions of the target
 *	machine architecture.  The distinction between target machine
 *	architectures is determined at compile time by the cpp symbol
 *	TARGET.  If TARGET == MC68000, further distinctions are made
 *	at run time, based on command line options and host machine type.
 *
 *	If TARGET == MC68000, legal <CPU options> are:
 *
 *		-mc68010	(expand .mc68010 templates)
 *		-mc68020	(expand .mc68020 templates)
 *
 *	If TARGET == MC68000, legal <FPU options> are:
 *
 *		-fsoft		(expand .fsoft templates)
 *		-fswitch	(expand .fswitch templates)
 *		-fsky		(expand .fsky templates)    [-mc68010 only]
 *		-f68881		(expand .f68881 templates)  [-mc68020 only]
 *		-ffpa		(expand .ffpa templates)    [-mc68020 only]
 *
 *	In the absence of command line options, the default CPU type is
 * 	that of the host; the default FPU type is -fsoft.
 *
 *	If TARGET == SPARC, there are no CPU or FPU options (yet).
 *
 *	Each <inlinefile> contains one or more labeled assembly language
 *	fragments of the form:
 *
 *		<inline directive>
 *		...
 *		<instructions>
 *		...
 *		.end
 *
 *	where the <instructions> constitute an inline expansion of
 *	the named routine, and <inline directive> defines the attributes
 *	of the inline code.  Legal forms of <inline directives> and their
 *	meanings are as follows:
 *
 *	If TARGET == MC68000 or SPARC:
 *
 *		.inline    <identifier>,<argcount>
 *
 *	This form declares an unrestricted inline expansion template
 *	for the routine named by <identifier>, with <argcount> bytes
 *	of arguments.  Calls to the named routine are replaced by the
 *	code in the inline template.
 *
 *	If TARGET == MC68000, the following additional forms are recognized:
 *
 *		.mc68010   <identifier>,<argcount>
 *		.mc68020   <identifier>,<argcount>
 *		.fsoft	   <identifier>,<argcount>
 *		.fswitch   <identifier>,<argcount>
 *		.fsky	   <identifier>,<argcount>
 *		.f68881	   <identifier>,<argcount>
 *		.ffpa	   <identifier>,<argcount>
 *
 *	These forms are similar to .inline, with the addition of a
 *	cpu or fpu specification.  The template is only expanded if
 *	the specified target machine matches the value of the target
 *	CPU or FPU type.
 *
 *	Multiple templates are permitted; the second and subsequent
 *	matching templates are ignored.  Duplicate templates may be
 *	placed in order of decreasing performance of the corresponding
 *	hardware; thus the most efficient usable version will be selected.
 *
 *	Inline expansion templates must follow the following rules:
 *
 *	if TARGET == MC68000:
 *
 *	-- Arguments are passed in 32-bit aligned memory locations
 *	   starting at sp@.  (Note that there is no return address on
 *	   the stack, since no jbsr instruction is executed)
 *	-- Results are returned in d0 or d0/d1.
 *	-- Registers a0/a1/d0/d1/fp0/fp1/fpa0-fpa3 may be used freely.
 *	-- The routine must delete <argsize> bytes from the stack.
 *	   (this is a hack to enable inline to deal crudely with
 *	   autoincrement/autodecrement addressing modes)
 *	-- The stack should not underflow the level of the last argument.
 *
 *	if TARGET == SPARC:
 *
 *	-- Arguments are passed in registers %o0-%o5, followed by
 *	   memory locations starting at [%sp+0x5c].  %sp is guaranteed
 *	   to be 64-bit aligned.
 *	-- Results are returned in %o0 or %f0/%f1.
 *	-- Registers %o0-%o5 and %f0-%f31 may be used freely.
 *	-- Integral and single precision floating point arguments are
 *	   32-bit aligned.
 *	-- Double precision floating point arguments are guaranteed to
 *	   be 64-bit aligned if the offset is a multiple of 8.
 *
 *	   Note that on Sparc, the instruction following the 'call'
 *	   will be inserted BEFORE the inline expansion, to preserve
 *	   semantics of the call delay slot.
 *
 *	if TARGET == MC68000 or SPARC:
 *
 *	-- Registers other than those noted above must be saved and restored
 *	   by the expanded routine.
 *
 *	Warning: inline does not check for violations of the above rules.
 */

#define MAXARGS 100

char   *filename;
int     lineno;
int     nerrors;
char	*sourcenames[MAXARGS];
int	nsources = 0;
int	verbose = 0;
int	warnings = 0;

typedef enum {
    M_undef,	/* matches nothing */
    M_inline,	/* matches anything */
    M_68010,
    M_68020,
    F_soft,
    F_switch,
    F_sky,
    F_68881,
    F_fpa,
} Machine_type;

Machine_type cpu = M_undef;
Machine_type fpu = M_undef;

struct machtype {
    char *name;
    Machine_type type;
};

struct machtype inlinetype = {
    "inline",  M_inline		   /* not a command line option */
};

#if TARGET == I386
char   *callops[] = {			   /* i386 call instruction mnemonics */
	"call",
	NULL
};
#endif TARGET == I386

#if TARGET == MC68000
char   *callops[] = {			   /* m68k call instruction mnemonics */
    "jbsr",
    "jsr",
    NULL
};

struct machtype cpu_options[] = {	   /* m68k cpu types */
    {"mc68010", M_68010},
    {"mc68020", M_68020},
    NULL
};

struct machtype fpu_options[] = {	   /* m68k fpu types */
    {"fsoft",   F_soft  },
    {"fswitch", F_switch},
    {"fsky",    F_sky   },
    {"f68881",  F_68881 },
    {"ffpa",    F_fpa   },
    NULL
};

static Machine_type cputype(/*cpu, arg*/); /* parse arg string for CPU type */
static Machine_type fputype(/*fpu, arg*/); /* parse arg string for FPU type */

#endif

#if TARGET == SPARC
char *callops[] = {		/* sparc call instruction mnemonics */
    "call",
    NULL
};
#endif

struct pats {			/* inline expansion template descriptor */
    char   *name;
    char   *replace;
    short  namelen;
    short  used;
    int    argsize;
    struct pats *left, *right;
    Machine_type mtype;		/* note: can specify cpu or fpu, but not both */
};

/*
 * DUP is an error-recovery kludge for duplicate
 * routine definitions.  The second and subsequent
 * occurrences of of an inline routine are ignored.
 */
static struct pats duplicate;
#define DUP &duplicate

struct pats *ptab = NULL;	/* table of inline template definitions */
FILE *input, *output;		/* source input & output streams */

extern char *malloc();
static char *alloc();		/* malloc with out-of-memory check */
extern int errno;
extern char *sys_errlist[];

/*VARARGS*/
extern int fprintf();

/*VARARGS1*/
static void
cmderr(s, x1, x2, x3, x4)
    char *s;
{
    (void)fprintf(stderr, "inline: ");
    (void)fprintf(stderr, s, x1, x2, x3, x4);
    nerrors++;
}

/*VARARGS1*/
static void
error(s, x1, x2, x3, x4)
    char *s;
{
    (void)fprintf(stderr, "inline: %s, line %d: ", filename, lineno);
    (void)fprintf(stderr, s, x1, x2, x3, x4);
    nerrors++;
}

/*VARARGS1*/
static void
werror(s, x1, x2, x3, x4)
    char *s;
{
    if (warnings) {
	(void)fprintf(stderr, "inline: %s, line %d: ", filename, lineno);
	(void)fprintf(stderr, s, x1, x2, x3, x4);
    }
}

/*
 * add pattern to the set of expandable routines.
 * Return a pointer to the new pattern descriptor.
 * If a duplicate name is encountered, return DUP.
 */
static struct pats *
insert(mtype, name, len)
    Machine_type mtype;
    register char  *name;
    register int len;
{
    register struct pats **pp;
    register struct pats *p;
    register int n;

    pp = &ptab;
    p = *pp;
    while (p != NULL) {
	n = strncmp(name, p->name, len);
	if (n < 0) {
	    pp = &p->left;
	} else if (n > 0) {
	    pp = &p->right;
	} else if (len < p->namelen) {
	    pp = &p->left;
	} else {
	    werror("Duplicate definition: %s (ignored)\n", p->name);
	    return DUP;
	}
	p = *pp;
    }
    /* check that machine type matches cpu or fpu */
    if (mtype != M_inline && mtype != cpu && mtype != fpu ) {
	return DUP;
    }
    p = (struct pats*)alloc((unsigned)sizeof(struct pats));
    bzero((char*)p, sizeof(struct pats));
    p->name = strncpy(alloc((unsigned)len+1), name, len);
    p->namelen = len;
    p->mtype = mtype;
    *pp = p;
    return (p);
}

/*
 * look up a name in the table of inline routines
 */
static struct pats *
lookup(name, len)
    register char  *name;
    register int    len;
{
    register int    n;
    register struct pats   *p;

    p = ptab;
    while (p != NULL) {
	n = strncmp(name, p->name, len);
	if (n < 0) {
	    p = p->left;
	} else if (n > 0) {
	    p = p->right;
	} else if (len < p->namelen) {
	    p = p->left;
	} else {
	    break;
	}
    }
    return (p);
}

char *patternbuf = NULL;
char linebuf[BUFSIZ];

#define nextch(cp,c) { c = *(cp)++; }
#define skipbl(cp,c) { while (isspace(c)) nextch(cp,c); }
#define findbl(cp,c) { while (!isspace(c)) nextch(cp,c); }

/*
 * scan constant
 * constant is passed by reference
 * function returns the number of chars scanned
 */

static int
scanconst(cp,c,const)
    register char *cp;
    register char c;
    int *const;
{
    register int value, digit;
    char *temp;

    temp = cp;
    value = 0;
    if(c == '0' && *cp == 'x') {
	nextch(cp,c);
	nextch(cp,c);
	while (isxdigit(c)) {
	    c = tolower(c);
	    digit = (isdigit(c)? c - '0' : c - 'a' + 10);
	    value = value * 16 + digit;
	    nextch(cp,c);
	}
    } else if (c == '0') {
	nextch(cp,c);
	while (c >= '0' && c <= '7') {
	    value = value * 8 + (c - '0');
	    nextch(cp,c);
	}
    } else {
	while (isdigit(c)) {
	    value = value * 10 + (c - '0');
	    nextch(cp,c);
	}
    }
    *const = value;
    return(cp-temp);
}

/*
 * look up an <inline_directive> keyword.  If the keyword is
 * recognized, return the associated cpu or fpu type; otherwise
 * return M_undef. The type of ".inline" is M_inline; it matches
 * any cpu or fpu type.
 */
static Machine_type
inline_directive(s)
    char *s;
{
    char *name;

    name = inlinetype.name;
    if (strncmp(s, name, strlen(name)) == 0) {
	return inlinetype.type;
    }
#   if TARGET == MC68000
    {
	int i;
	for (i = 0; cpu_options[i].name != NULL; i++) {
	    name = cpu_options[i].name;
	    if (strncmp(s, name, strlen(name)) == 0) {
		return ( cpu_options[i].type );
	    }
	}
	for (i = 0; fpu_options[i].name != NULL; i++) {
	    name = fpu_options[i].name;
	    if (strncmp(s, name, strlen(name)) == 0) {
		return ( fpu_options[i].type );
	    }
	}
    }
#   endif
    return M_undef;
}

/*
 * read inline expansion templates from
 * a user-supplied template file.
 */
static void
readpats(fname)
    char   *fname;
{
    register    FILE * f;
    register char   c;
    register char  *cp;
    register char  *bufp;
    struct pats *pattern;
    struct stat statbuf;
    char *name;
    int len;
    unsigned filesize;
    Machine_type temptype;

    filename = fname;
    lineno = 1;
    f = fopen(filename, "r");
    if (f == NULL) {
	cmderr("cannot open inline file '%s'\n", filename);
	return;
    }
    if (stat(filename, &statbuf) < 0) {
	cmderr("cannot stat inline file '%s': %s\n",
	    filename, sys_errlist[errno]);
	return;
    }
    filesize = statbuf.st_size;
    patternbuf = alloc(filesize + (BUFSIZ/2));
    pattern = NULL;
    while (fgets(linebuf, sizeof(linebuf), f)) {
	lineno++;
	cp = linebuf;
	nextch(cp,c);
	skipbl(cp,c);
	if (c == '.') {
	    temptype = inline_directive(cp);
	    if ( temptype != M_undef ) {
		/* <inline directive> starts a new routine */
		if (pattern != NULL) {
		    error("inline routines cannot be nested\n");
		    continue;
		}
		/* skip keyword and white space */
		findbl(cp,c)
		skipbl(cp,c);
		/* scan routine name */
		if (c == '_' || c == '$' || isalpha(c)) {
		    name = cp - 1;
		    nextch(cp,c);
		    while (c == '_' || c == '$' || isalnum(c)) {
			nextch(cp,c);
		    }
		    pattern = insert(temptype, name, cp-name-1);
		    bufp = patternbuf;
		    skipbl(cp,c);
		} else {
		    error("identifier expected\n");
		}
		/*
		 * argument count indicates how many bytes of
		 * arguments are expected by the called routine.
		 * This is a kludge to enable inline to compensate
		 * crudely for the effects of autoincrement addressing.
		 * In the normal 68000 calling sequence, arguments
		 * are popped off the stack by the caller; in an inline
		 * template, the "callee" is expected to pop them.
		 */
		if(c == ',') {
		    nextch(cp,c);
		    skipbl(cp,c);
		} else {
		    error("',' expected\n");
		}
		/* scan argument count */
		if(isdigit(c)) {
		    cp += scanconst(cp,c,&pattern->argsize);
		} else {
		    error("argument count expected\n");
		}
		continue;
	    } else if (strncmp(cp, "end", 3) == 0) {
		/* .end terminates the current routine */
		if (pattern == NULL) {
		    error(".end without matching .inline\n");
		    continue;
		}
		*bufp++ = '\0';
		len = bufp - patternbuf;
		if (pattern != DUP) {
		    /*
		     * store pattern, but only the first time;
		     * i.e., ignore duplicate definitions
		     */
		    pattern->replace =
			strncpy(alloc((unsigned)len+1), patternbuf, len);
		}
		pattern = NULL;
		continue;
	    }
	}
	/* default action: stash the line */
	if (pattern != NULL) {
	    cp = linebuf;
	    while (c = *cp++) {
		*bufp++ = c;
	    }
	}
    }
    if (pattern != NULL) {
	error(".inline without matching .end\n");
    }
    free(patternbuf);
}

/*
 * Parse a source line, look for a call instruction,
 * and return an inline template descriptor if a
 * call to an inline-expandable routine is found.
 */
static struct pats *
callinst(sourceline)
    char   *sourceline;
{
    register char *cp, **op;
    register char c;
    register char *name;
    register len;

    cp = sourceline;
    nextch(cp,c);
    skipbl(cp,c);
    if (!isalpha(c))
	return NULL;
    name = cp-1;
    while(isalnum(c))
	nextch(cp,c);
    len = cp-name-1;
    for (op = callops; *op != NULL; op++) {
	if (strncmp(*op, name, len) == 0) {
	    skipbl(cp,c);
	    name = cp-1;
	    while (isalnum(c) || c == '$' || c == '_')
		nextch(cp,c);
	    return lookup(name, cp-name-1);
	}
    }
    return (NULL);
}

#if TARGET == MC68000

/*
 * return CPU option selected by a command-line argument
 * string.  Issue a warning if arg conflicts with an earlier
 * option (i.e., cpu != M_undef)
 */
static Machine_type
cputype(cpu, arg)
    Machine_type cpu;
    char *arg;
{
    int i;

    for (i = 0; cpu_options[i].name != NULL; i++) {
	if (strcmp(cpu_options[i].name, arg+1) == 0) {
	    /*
	     * arg matches option from table.  Return
	     * the latter; string address is significant
	     */
	    if (cpu != M_undef && cpu != cpu_options[i].type) {
		cmderr("conflicting cpu option \"-%s\" ignored\n",
		    cpu_options[i].name);
		return cpu;
	    }
	    return cpu_options[i].type;
	}
    }
    return M_undef;
}

/*
 * return the FPU option selected by a command-line argument
 * string.  Similar to cputype().
 */
static Machine_type
fputype(fpu, arg)
    Machine_type fpu;
    char *arg;
{
    int i;

    for (i = 0; fpu_options[i].name != NULL; i++) {
	if (strcmp(fpu_options[i].name, arg+1) == 0) {
	    /*
	     * arg matches option from table.  Return
	     * the latter; string address is significant
	     */
	    if (fpu != M_undef && fpu != fpu_options[i].type) {
		cmderr("conflicting fpu option \"-%s\" ignored\n",
		    fpu_options[i].name);
		return fpu;
	    }
	    return fpu_options[i].type;
	}
    }
    return M_undef;
}

/*
 * default CPU type = what we're running on
 */
static Machine_type
default_cputype()
{
#   ifdef mc68020
	return M_68020;
#   endif
#   ifdef mc68010
	return M_68010;
#   endif
}

/*
 * default FPU type = fsoft
 */
static Machine_type
default_fputype()
{
    return F_soft;
}

/*
 * check that FPU and CPU types form
 * a sensible combination
 */
static void
check_fputype()
{
    if (cpu == M_undef)
	cpu = default_cputype();
    if (fpu == M_undef)
	fpu = default_fputype();
    if (cpu == M_68010 && fpu == F_fpa
      || cpu == M_68010 && fpu == F_68881
      || cpu == M_68020 && fpu == F_sky) {
	cmderr("%s and %s are not a supported combination\n", cpu, fpu);
    }
}

/*
 * emit code to adjust the stack pointer following
 * the inline expansion, to  compensate for the fact
 * that the template (presumably) has deleted the arguments
 * from the stack (which usually is done by the caller).
 *
 * 1e+308 on the vomit meter.
 */
static void
adjust(pattern, output)
    struct pats *pattern;
    FILE *output;
{
    int n;

    n = pattern->argsize;
    if (n > 0) {
	(void)fprintf(output, "	%s	#%d,sp\n",
	    n <= 8 ? "subqw" : "subw", n);
    } else if (n < 0) {
	error(".inline routine %s has illegal argument size (%d)\n",
	    pattern->name, n);
    }
}
#endif MC68000

#if TARGET == I386
/* see comment for MC68000 version */
static void
adjust(pattern, output)
    struct pats *pattern;
    FILE *output;
{
    int n;

    n = pattern->argsize;
    if (n > 0) {
	(void)fprintf(output, "	subl	$%d,\%%esp\n",n);
    } else if (n < 0) {
	error(".inline routine %s has illegal argument size (%d)\n",
	    pattern->name, n);
    }
}
#endif TARGET == I386

static void
summary(p)
    register struct pats *p;
{
    if (p != NULL) {
	summary(p->left);
	summary(p->right);
	if (p->used) {
	    (void)fprintf(stderr,"\t%s\n", p->name);
	}
    }
}

static void
expand(input,output)
    register FILE *input;
    register FILE *output;
{
    register struct pats *pattern;

    while (fgets(linebuf, sizeof(linebuf), input)) {
	pattern = callinst(linebuf);
	if (pattern == NULL || pattern->replace == NULL) {
	    fputs(linebuf, output);
	} else {
	    pattern->used = 1;
#	    if TARGET == SPARC
		/* write out the "delay slot" instruction first */
		if (fgets(linebuf, sizeof(linebuf), input) == NULL) {
		    error("no delay slot in call");
		}
		fputs(linebuf,output);
#	    endif
	    fputs(pattern->replace, output);
#	    if (TARGET == MC68000) || (TARGET == I386)
		adjust(pattern, output);
#	    endif
	}
    }
    if (verbose) {
	(void)fprintf(stderr,"Expanded:\n");
	summary(ptab);
    }
}

static void
init(argc, argv)
    int argc;
    char *argv[];
{
    char *arg;
    int i;

    output = stdout;
    for (i = 1; i < argc; i++) {
	arg = argv[i];
	if (arg[0] == '-') {
	    switch(arg[1]) {
	    case 'i':
		/* argv[i+1] is the name of an inline file */
		if (i+1 < argc) {
		    readpats(argv[i+1]);
		    i++;
		} else {
		    cmderr("filename expected after -i\n");
		}
		break;
	    case 'o':
		/* argv[i+1] is the name of an output file */
		if (i+1 < argc) {
		    i++;
		    output = fopen(argv[i], "w");
		    if (output == NULL) {
			cmderr("cannot open output file '%s'\n", argv[i]);
			output = stdout;
		    }
		} else {
		    cmderr("filename expected after -o\n");
		}
		break;
	    case 'v':
		verbose++;
		break;
	    case 'w':
		warnings++;
		break;
#	    if TARGET == MC68000
		case 'm':
		    cpu = cputype(cpu, arg);
		    break;
		case 'f':
		    fpu = fputype(fpu, arg);
		    break;
#	    endif MC68000
	    default:
		cmderr("unknown option (%s)\n", arg);
		break;
	    }
	} else {
	    /* arg is the name of a source file */
	    if (nsources < MAXARGS) {
		sourcenames[nsources++] = arg;
	    } else {
		cmderr("too many arguments\n");
		break;
	    }
	}
    }
}

main(argc, argv)
    int argc;
    char *argv[];
{
    int i;

    init(argc, argv);
#   if TARGET == MC68000
	check_fputype();
#   endif
    if (nsources) {
	for(i = 0; i < nsources; i++) {
	    /* process source files in sequence */
	    input = fopen(sourcenames[i], "r");
	    if (input == NULL) {
		cmderr("cannot open source file '%s'\n", sourcenames[i]);
		continue;
	    }
	    expand(input, output);
	}
    } else {
	/* source defaults to standard input */
	input = stdin;
	expand(input, output);
    }
    exit(nerrors);
}

static char *
alloc(n)
    unsigned n;
{
    char *p;

    p = malloc(n);
    if (p == NULL) {
	perror("inline");
	exit(1);
    }
    return p;
}
