.\"  @(#)arch.t	1.1 10/31/94
.\"----------------------------------------------------------------------------
.\" Internal Architecture of /lib/compile
.\"----------------------------------------------------------------------------
.\"  :!	<%      tbl | troff -ms -Tsun &
.\"	<arch.t tbl | troff -ms -Tlw  &
.\"	<arch.t tbl | troff -ms -t    >arch.dit && lpr -n  arch.dit  &
.\"	<arch.t tbl | troff -ms -t    >arch.dit && preview arch.dit  &
.\"	<arch.t tbl | $HOME/bin/pstroff -ms -t >arch.ps && lpr -h arch.ps
.\"
.\"	<arch.t tbl | $HOME/bin/pstroff -ms -t >arch.ps
.\"		rcp arch.ps welchs:/tmp
.\"		rsh welchs 'cd /tmp; lpr -h arch.ps; lpq'
.\"----------------------------------------------------------------------------
.ds Lc "\fL/lib/compile\fP
.ds Cc "\fLcc\fP
.\"
.ds Vr "\fLVIRTUAL_ROOT\fP
.ds Ch "\fLCROSS_COMPILATION_HOME\fP
.ds Nv "\fLNSE_VARIANT\fP
.ds Ta "\fLTARGET_ARCH\fP
.\"
.ds Ua "\fL/usr/arch\fP
.ds Us "\*(Ua\fL.SERVER\fP
.ds Ct "\*(Ua\fL/common/\fP\fItarget_arch\fP
.ds Ha "\*(Ua\fL/\fP\fIhost_arch\fP
.ds Hc "\*(Ha\fL/common\fP
.ds Ht "\*(Ha\fL/\fP\fItarget_arch\fP
.ds Ho "\*(Ha\fL/\fP\fIother_arch\fP
.ds Hh "\*(Ha\fL/\fP\fIhost_arch\fP
.ds Vv "\fIvirtual_root\fP
.\"
.\" In -- incomplete section
.de In
.br \""".ce
\fI\(em\(em\(em this section is incomplete \(em\(em\(em\fP
.br
..
.\"
.\" Fi -- source FIle information
.de Fi
[\s-4see file \fL\\$1\fP\s+4]\\$2
..
.\" Nb -- Note section Begin
.de Nb
.ll    -6n
.nr LL -6n
.in +3n
.\""" .nr Nl \\w'\\fI\\$1  \\fP'u
.nr Nl \w'\\fINote:  \\fP'u
.nr Nv 3
.nr Np 2
.sp \\n(Nvp
\fI\\$1  \fP
'in +\\n(Nlu
'ps    -\\n(Npp
'vs    -\\n(Nvp
'nr PS -\\n(Np
'nr VS -\\n(Nv
..
.\" Ne -- Note section End
.de Ne
.nr PS +\\n(Np
.nr VS +\\n(Nv
.ps    +\\n(Npp
.vs    +\\n(Nvp
.in -\\n(Nlu
.in -3n
.ll    +6n
.nr LL +6n
..
.\" Sc -- Centered Section header
.de Sc
.bp
.SH
.ce
\s+4\fB\\$1\fP\s-4
.LP
.XS
.if \\n(.$<2 \\$1
.if \\n(.$>=2 \\$2
.XE
..
.\" Sl -- Left-justified Section header
.de Sl
.SH
\s+2\fB\\$1\fP\s-2
.LP
.XS 
.if \\n(.$>=2 \h'0.5i'\\$2\"----use 2nd arg for TOC, if present.
.if \\n(.$<2 \h'0.5i'\\$1\"-----otherwise use 1st arg
.XE
..
.\"
.nr PS 12
.nr VS 16
.LP
.\"----------------------------------------------------------------------------
.\"			Reviewer's cover sheet
.\"----------------------------------------------------------------------------
 \s2.\s0
.nr PS 36
.nr VS 36
.LP
.ce
.BX "\fHREVIEW COPY\fP"
.\""" .sp 0.2i
.nr PS 28
.nr VS 36
.LP
.ce 2
\f(LB/lib/compile\fP
Internal Architecture
.sp 0.5i
.nr PS 12
.nr VS 14
.LP
Mark Scott has offered your help in reviewing the attached tome
(don't you just love being volunteered?!).
Please review it as soon as possible.
The target date for review returns is Tuesday, Jan 19\u\s-4th\s+4\d
(I'm in office #104, near the front lobby).

If you wish to read source code along with this,
you can find SCCS-gotten versions of the source in directory:
.ti +8n
\fLearth:/home/buca/dweaver/lang/compile\fP
.br
RoadRunner-merged source code can be found in directory:
.ti +8n
\fLearth:/home/buca/dweaver/lang/compile/386\fP
.br
The SCCS files can be found in directory:
.ti +8n
\fLrogue:/lang/4.0/SCCS_DIRECTORIES/lang/compile/SCCS\fP

Note that:
.IP \(bu
Reviewing this does \fInot\fP increase the likelihood that you
will become fortunate enough to maintain \*(Lc!
.IP \(bu
The document is incomplete (as marked).
.IP \(bu
It represents a ``core dump'' of knowledge about \*(Lc.
I only claim a first-draft semblence of organization.
.IP \(bu
I am especially looking for comments which will improve:
.RS
.in +2n
.IP \(bu
content
(what's missing?,
what should there be more or less coverage of?,
can it be better explained from another angle?)
.IP \(bu
organization (can the information be presented better in a different order?)
.IP \(bu
level of detail (is more or less detail needed?  where?)
.in -2n
.RE
.IP \(bu
Comments concerning grammar, punctuation, spelling,
and anything else are also welcome.
.IP \(bu
It has been suggested that the ``chapter'' on Cross-Compilation
be elaborated and split out into a separate document.
Comments?
.LP
.sp 0.6v
Remember, the more picky and vicious you are with your comments \fInow\fP,
the easier life will be for the person(s) who end up supporting \*(Lc
\fIlater\fP.
.sp 0.6v
Thanks very much for your help!
.ti +4n
\(em
.nr PS 12
.nr VS 16
.LP
.\"
.\"" .EH '\fB\s+2DRAFT\s-2\fP''\fB\s+2DRAFT\s-2\fP'
.\"" .OH '\fB\s+2DRAFT\s-2\fP''\fB\s+2DRAFT\s-2\fP'
.\"" .LP
.\"" .P1
.pn 1
.bp
.\"----------------------------------------------------------------------------
.\"			Header page
.\"----------------------------------------------------------------------------
.LP
 \s2.\s0
.sp 2.5i

.nr PS 28
.nr VS 36
.LP
.ce 2
\f(LB/lib/compile\fP
Internal Architecture
.sp 1.0i
.nr PS 14
.nr VS 20
.LP
.ce 2
David L. Weaver
\*(DY
.nr PS 12
.nr VS 16
.LP
.\"
.EH '\fB\s+2DRAFT\s-2\fP'\s12\- % \-\s0'\fB\s+2DRAFT\s-2\fP'
.OH '\fB\s+2DRAFT\s-2\fP'\s12\- % \-\s0'\fB\s+2DRAFT\s-2\fP'
.EF '\fL/lib/compile\fP'Sun Microsystems CONFIDENTIAL'\*(DY'
.OF '\fL/lib/compile\fP'Sun Microsystems CONFIDENTIAL'\*(DY'
.pn 1
.\"----------------------------------------------------------------------------
.\"			The rest...
.\"----------------------------------------------------------------------------
.Sc Overview

\*(Lc is the driver program for the following Sun programming language
commands:
.TS
center;
l l l
l l l
l l l
lfL lfL l.
Command	Residence in
(Driver)	Filesystem	Function
\_	\_	\_
cc	/bin/cc	C compiler
f77	/usr/bin/f77	FORTRAN-77 compiler
lint	/usr/bin/lint	C type-checker
m2c	/usr/bin/m2c	Modula-2 compiler
pc	/usr/ucb/pc	Pascal compiler
.TE
Since a great deal of common code existed among these compilers' drivers,
they were combined into one program, \*(Lc, in SunOS Release 3.2.
From that release up through the present (Sys4-3.2 and SunOS-4.0-Beta),
these commands have existed as symbolic links to \*(Lc.

Additionally, \*(Lc serves as the \*Qmastermind\*U behind the facilities
offered in the Sun Cross-Compiler products.


This document serves as a guide to the internal operation of \*(Lc.
It is written for technical readers,
especially those who will modify or maintain \*(Lc.
It should be a \*Qliving\*U document;
those who modify \*(Lc should modify this document accordingly.


.Nb Note:
The definitions of italicized terms with global meanings in this document
can be found in Appendix A.
.Ne

.Sc "A Chronological Tour of \f(LB/lib/compile\fP" "A Chronological Tour of \*(Lc"

If the reader is using this document as a companion to the \*(Lc source code,
the ``tour'' begins in function \fLmain()\fP
.Fi compile.c .

.Sl "Processing Arguments"
\*(Lc determines which driver it should impersonate by looking at the
name by which it was called (in \fLargv[0]\fP).

Command-line arguments fall into two general categories:
options and filenames.
An option begins with `\-', and has an entry in the driver's options table.
This options table is an array of \fLOption\fP\|s named \fLoptions[]\fP
.Fi ro_data.c  .
Filenames are \*Qanything else\*U, including
source-code files,
inline-expansion template (\*Q\fL.il\fP\*U) files,
relocatable object (\*Q\fL.o\fP\*U) files,
and libraries (\*Q\fL.a\fP\*U files).

As the argument list is scanned, each argument is looked up in \fLoptions[]\fP.
\fLlookup_option()\fP
.Fi compile.c
scans this array linearly,
stopping at the first partial match in the option table.
Order is important in the table;
a given option must come before all others with shorter names which
match an initial substring of that option.
.Nb "For example:"
.br
\fL-ansi\fP must precede \fL-a\fP in the table.
Otherwise, \fL-ansi\fP on the command line would match \fL-a\fP in the table
before \fL-ansi\fP was found.
.Ne
By convention, the table is kept in generally alphabetical order,
subject to the above constraint.

The \*Qtype\*U of an option
(\fLtype\fP and \fLsubtype\fP fields of its \fLOption\fP structure)
determine how it is processed, after it is found in the table.
Typically, an option is added to the option-list for a particular compiler pass
(perhaps after being modified, in a manner appropriate to its type),
sets or resets one of the driver's internal Boolean flags, 
or sets an internal variable
(such as \*Q\fL-O\fP\fIn\fP\*U sets the optimization level,
or \*Q\fL-o\ \fP\fIfile\fP\*U sets the output filename).

If no match is found in \fLoptions[]\fP and the argument begins with `\-',
a warning is issued claiming that the argument is an \*Qunknown option\*U.
The argument is then passed, unmodified, to the linker (\fLld\fP).

If no match is found and the argument does \fInot\fP begin with `\-',
it is assumed that the argument is an input filename.
A filename is added to the list of input files for the appropriate
compilation step
(e.g. \fLcpp\fP, \fLccom\fP, \fLinline\fP, \fLas\fP)
based on its suffix
(e.g. \fL.c\fP, \fL.i\fP, \fL.il\fP, \fL.s\fP, respectively).

.Sl "Setup Before Compilation"
After all of the arguments have been scanned,
\*(Lc checks for illegal combinations of options,
and treats them as described in the following table:
.KS
.TS
center;
c	s	l
c	s	l
c	s	l
lfL 2	lfL	l.
Option
Combination	Disposition
\_	\_
-sun2	-ffpa	fatal error
-sun3	-fsky	fatal error
-sun386	-f\fIanything\fP	fatal error
-sun4	-f\fIanything\fP	fatal error
\-g	\-a	\*Q\fL-a\fP\*U turned off
\-g	\-O	\*Q\fL-O\fP\*U turned off
\-g	\-R	\*Q\fL-R\fP\*U turned off
\-a	\-O	\*Q\fL-O\fP\*U turned off
\-a	\-R	\*Q\fL-R\fP\*U turned off
.TE
.KE

If optimization was requested without specifying an optimization level
(i.e. via vanilla \*Q\fL-O\fP\*U),
a default optimization level is assigned as follows:
.TS
center;
c	c	c
c	c	c
cfL	cfL	lfL.
Target Architecture	Driver	Default Optimization Level
\_	\_	\_
\fIall\fP	f77	-O3 \fI(peephole + full global)\fP
sun4	\fIall except f77\fP	-O2 \fI(peephole + partial global)\fP
\fIall except sun4\fP	\fIall except f77\fP	-O1 \fI(peephole only)\fP
.TE

The optimization level is verified against legal values,
as determined by a combination of the
target software release, target architecture, and language:
.KS
.nr VS -2
.LP
.TS
center;
c	c	c	c
c	c	c	c
c	c	c	c
c	c	c	c
cfL	cfL	cfL	cfL.
Target			Maximum
Software	Target		Optimization
Release	Architecture	Driver	Level
\_	\_	\_	\_

3.x\**	sun4	cc,f77,pc\u\(**\d,m2c\(dg	-O4
3.x	sun2,sun3,sun386	f77	-O3
3.x	sun2,sun3,sun386	cc,pc,m2c	-O1

4.0	sun2,sun3,sun4	cc,f77,pc,m2c\(dd	-O4
4.0	sun386	cc,f77,pc,m2c	-O1
.TE
.nr PS -4
.nr VS -3
.DS B
\(**  applies to Pascal 1.05; \fL-O1\fP was the max optimization level for Pascal 1.0
\(dg  as implemented; \fL-O1\fP \fIshould have been\fP the max optimization level implemented in
\h'2n'\*(Lc for 3.x-based Modula-2
\(dd  as implemented; correct max optimization level for SunOS4.0-based Modula-2 is not yet known
.DE
.nr VS +3
.nr PS +4
.nr VS +2
.LP
.KE
.FS
Release ``\fL3.x\fP'' for Sun-4 actually refers to Sys4-3.2.
.FE

.KS
\*(Lc then performs setup for cross-compilation, as needed.
See \fBCross-Compilation\fP
and \fBSupport for NSE Variants\fP for details.
.KE


.Sl "Invocation of Compiler Passes"
After all of the arguments have been examined and sanitized,
\fIdriver\fP\fL_doit()\fP
.Fi setup_\fP\fIdriver\fP\fL.c
is called\**,
.FS
This is accomplished
'in +2m
through an incomprehensibly indirect function call, the code for
which (at the end of \fLmain()\fP) looks like:
.nr PS -4
.nr VS -4
.DS B
\fLif (driver.value->name != NULL) {
\h'6m'(*((int (*)())(driver.value->name)))();
}\fP
.DE
.nr PS +4
.nr VS +4
.in -2m
.FE
where \fIdriver\fP is the name of the compiler driver
which \*(Lc is impersonating.
E.g. in the case of driver \*(Cc, \fLcc_doit()\fP is called.

The \fIdriver\fP\fL_doit()\fP function in turn calls \fLdo_infiles()\fP,
which processes each input file according to its suffix.

\fLdo_infiles()\fP
.Fi setup.c
processes each of the input files on the global list \*Q\fLinfile\fP\*U.
It processes each file with a function appropriate to the file's suffix,
either \fLcompile_\fP\fIsuffix\fP\fL()\fP
.\""" .Fi  setup_\fP\fIdriver\fP\fL.c
(for source-code files), or
\fLcollect_\fP\fIsuffix\fP\fL()\fP
(for relocatable object (\*Q\fL.o\fP\*U) files,
libraries (\*Q\fL.a\fP\*U files),
and lint libraries (\*Q\fL.ln\fP\*U files)).
Source files are compiled using passes appropriate to their suffix,
e.g. \*Q\fL.c\fP\*U files are compiled with C compiler passes.
Relocatable object files and library files are added
to a list of files for later processing by the link-editor.
Lint library files are added
to a list of files for later processing by \fLlint2\fP.

Each \fLcompile_\fP\fIsuffix\fP\fL()\fP
or \fLcollect_\fP\fIsuffix\fP\fL()\fP
compiler-driver function is bound to a suffix
using the \fLcompile\fP or \fLcollect\fP field (respectively)
of the suffix's entry in the \fLSuffixes\fP array of \fLSuffix\fP structures
.Fi ro_data.c  .

The appropriate \fLcompile_\fP\fIsuffix\fP\fL()\fP function selects a template
of candidate program steps to run for that source language,
based on factors such as the requested optimization level.
A template of steps is an array of \fLStep\fP structures,
terminated by the \fLEND_OF_STEPS()\fP macro
.Fi ro_data.c  .
\fLcompile_\fP\fIsuffix\fP\fL()\fP then calls \fLrun_steps()\fP,
passing the name of the source file
and the selected template of compilation steps.

\fLrun_steps()\fP determines which of the given program steps
should actually be executed
(using the function specified in the step's \fLexpression\fP field),
and for each step it:
.IP \(bu 3
Performs setup for the step, e.g sets the option flags to be passed to it.
This is done by executing the function specified in the \fLsetup\fP field
of the \fLStep\fP structure.
If none is specified, the default setup function for that step's
program, found in \fLprogram->setup\fP, is executed instead.
.IP \(bu
Redirects I/O to \fL/tmp\fP files or UNIX pipes as necessary
.IP \(bu
Executes the step.
.LP


If the \fL-v\fP (\fL-verbose\fP for \fLlint\fP) command-line flag is used,
\fLrun_steps()\fP reports the steps as they are executed to the user's terminal.
The \fL-dryrun\fP flag similarly reports the steps,
but also prevents them from actually being executed.

.Sc "A Physical Tour of \f(LB/lib/compile\fP" "A Physical Tour of \*(Lc"

The following table lists the source files of \*(Lc
and the general contents of each.
It is understood that most of the \fL.c\fP files contain functions
auxilliary to their main content.
.TS
center;
c	cw(3.6i)
c	l
lfL	l.
Source File	Contents
\_	\_
driver.h	T{
Definitions of constants and structures,
used by all other source files
T}
compile.c	T{
Main program,
cross-compilation setup functions,
routine to print ``help'' messages,
option-lookup routine
T}
ro_data.c	T{
Global data tables which may be read-only,
and therefore shared by multiple instances of \*(Lc at runtime.
.\""" A few functions auxiliary to the data tables are also in this file.
T}
rw_data.c	T{
Global data which must remain writable at runtime.
T}
run_pass.c	T{
Functions which handle the actual execution of program steps
and production/destruction of associated temporary files.
T}
setup.c	T{
Setup functions for language-independent compiler passes
(e.g. \fLcpp\fP, \fLiropt\fP, \fLld\fP, \fLas\fP).
T}
setup_cc.c	T{
C-language specific compiler pass setup functions.
T}
setup_f77.c	T{
FORTRAN-specific compiler pass setup functions.
T}
setup_lint.c	T{
Lint-specific compiler pass setup functions.
T}
setup_m2c.c	T{
Modula2-specific compiler pass setup functions.
T}
setup_pc.c	T{
Pascal-specific compiler pass setup functions.
T}
driver_test.c	T{
Driver program for \*(Lc regression testing.
T}
.TE

Each following section describes one of the above source files
in greater detail.

.Sl "\f(LBdriver.h\fP" "\fLdriver.h\fP"
This file contains all of the \fL#define\fP's, structure definitions, and
\fLextern\fP declarations for the whole program.

The \fLConst_int\fP and \fLNamed_int\fP abstractions are important to
understand, as they are used throughout \*(Lc.
They are defined as:
.KS
.de Sm	\" make smaller, for code
.nr PS -4
.nr VS -6
.LP
..
.de Lg	\" make un-smaller, after code
.nr PS +4
.nr VS +6
.LP
..
.Sm
.TS
center;
lfL	s	s    7	lfL	s	s
lfL 5	lfL	lfL  7	lfL 5	lfL	lfL.
typedef struct {	typedef struct {
\|	int	value;		Const_intP	value;
\|	char	*name;		char	*help;
\|	char	*extra;		Bool	touched    :1;
\|	\|	\|	\|	Bool	redefine_ok:1;
.T&
lfL	s	s	lfL	s	s.
} Const_int, *Const_intP;	} Named_int, *Named_intP;
.TE
.Lg
.\""" .T&
.\""" lfL	s	s	lfL	lfL	lfL.
.\""" } Const_int;		Bool	redefine_ok:1;
.\""" .T&
.\""" lfL	s	s	lfL	s	s.
.\""" } Const_int;	} Named_int;
.KE
Note that both structures have a field named ``\fLvalue\fP'',
which can be confusing.
It can be doubly confusing that \fLvalue\fP's type is different
in the two structures.
These structure-member names could be changed to something like
\fLc_value\fP and \fLn_value\fP, to relieve confusion;
doing so would require a careful editing job throughout the source files.
.\""" 
.\""" The code used to say things like
.\""" .ti +4m
.\""" \fLif (target_arch.value == &arch_sun2)\fP
.\""" .br
.\""" in many places.
.\""" Such code now reads:
.\""" .ti +4m
.\""" \fLif (target_arch.value->value == ARCH_SUN2)\fP
.\""" .br

Additional structures are duplicated here,
followed by explanations of their function.

.Sm
.TS
center;
lfL	s	s
lfL  6	lfL	lfL.
typedef struct {
\|	char	*suffix;
\|	short int	in_drivers;
\|	short int	out_drivers;
\|	int	(*compile)();
\|	void	(*collect)();
\|	char	*help;
.T&
lfL	s	s.
} Suffix;

.T&
lfL	s	s
lfL  6	lfL	lfL.
typedef struct {
\|	Suffix	a;
\|	Suffix	c;
\|	Suffix	def;
\|	Suffix	f;
\|	Suffix	F;
\|	Suffix	il;
\|	Suffix	i;
\|	Suffix	ln;
\|	Suffix	mod;
\|	Suffix	o;
\|	Suffix	pi;
\|	Suffix	p;
\|	Suffix	r;
\|	Suffix	sym;
\|	Suffix	s;
\|	Suffix	S;
\|	Suffix	ir;
\|	Suffix	none;
\|	Suffix	sentinel_suffix_field;
.T&
lfL	s	s.
} Suffixes;
.TE
.Lg
There is one instance of a \fLSuffix\fP structure for each type of Suffix,
embedded in the \fLSuffixes\fP structure.
The single instance of \fLSuffixes\fP is \fLsuffixes\fP,
which is delcared in \fLro_data.c\fP.

The \fLsuffix\fP field points to the suffix in character string form,
without a leading `.'.
\fLin_drivers\fP and \fLout_drivers\fP contain bitmasks
(in \fLDRIVER_*\fP form)
indicating which drivers consume and produce (respectively)
files of the given suffix.

The \fPcompile\fP (if non-\fLNULL\fP) field points to a function named
\fLcompile_\fP\fIsuffix\fP\fL()\fP, used to compile files of this suffix.
\fPcollect\fP (if non-\fLNULL\fP) points to a function named
\fLcollect_\fP\fIsuffix\fP\fL()\fP, used to collect files of this suffix
for processing by a later compiler pass.
See the \fBInvocation of Compiler Passes\fP section for a more detailed
explanation of \fLcompile\fP and \fLcollect\fP.

.Sm
.TS
center;
lfL	s	s
lfL	s	s
lfL  6	lfL	lfL.
#define PGM_TEMPLATE_LENGTH 5
typedef struct {
\|	char	*name;
\|	char	*path;
\|	short int	drivers;
\|	char	*(*setup)();
\|	ListP	permanent_options;
\|	ListP	trailing_options;
\|	ListP	options;
\|	ListP	infile;
\|	char	*outfile;
\|	Bool	has_been_initialized :1;
\|	Template	template[PGM_TEMPLATE_LENGTH];
.T&
lfL	s	s.
} Program;

.T&
lfL	s	s
lfL	6	lfL	lfL.
typedef struct {
\|	Program	cpp;
\|	Program	m4;
\|	Program	ratfor;
\|	Program	lint1;
\|	Program	cat;
\|	Program	lint2;
\|	Program	m2cfe;
\|	Program	ccom;
\|	Program	pc0;
\|	Program	f1;
\|	Program	mf1;
\|	Program	vpaf77;
\|	Program	f77pass1;
\|	Program	iropt;
\|	Program	iropt_f77;
\|	Program	iropt_pc;
\|	Program	cg;
\|	Program	cg_f77;
\|	Program	cg_pc;
\|	Program	inline;
\|	Program	c2;
\|	Program	as;
\|	Program	asS;
\|	Program	pc3;
\|	Program	ld;
\|	Program	vpald;
\|	Program	m2l;
\|	Program	sentinel_program_field;
.T&
lfL	s	s.
} Programs;
.TE
.Lg
There is one instance of a \fLProgram\fP structure for each type of Program,
embedded in the \fLPrograms\fP structure.
The single instance of \fLPrograms\fP is \fLprograms\fP,
which is delcared in \fLrw_data.c\fP.

The \fLname\fP field points to the name of the program,
in character string form.
\fLpath\fP gives the full pathname of the program in the filesystem,
used when \fLexec()\fP'ing it.

\fLdrivers\fP contains a bitmask (in \fLDRIVER_*\fP form)
indicating which drivers make use of the program.
\fINote:\fP this field of the \fLProgram\fP structure doesn't seem to be
referenced anywhere in the code.

\fLsetup\fP is a pointer to the function to call
to set up for a run of the program.

\fLpermanent_options\fP
points to a list of options to pass on to the program,
which can be set once per driver command line.
These options are invariant between compilations of various files
on the command line.
For example:
.ti +5n
\fL%\fP\f(LB  m2c a.mod b.def  -v\fP
.br
produces these two ``\fLm2cfe\fP'' command lines:
.ti +5n
\s-4\f(LB/usr/lib/modula2/m2cfe -mod a "-M. /usr/lib/modula2" a.mod >\fP\fItemp_file\fP\s+4
.ti +5n
\s-4\f(LB/usr/lib/modula2/m2cfe -def b "-M. /usr/lib/modula2" b.def >b.sym\fP\s+4
.br
The ``\fL-M\fP'' option of the \fLm2cfe\fP program is one which is invariant
per invocation of the compiler driver (\fLm2c\fP here).
So, in this example, the \fL"-M. /usr/lib/modula2"\fP option is set once,
as a ``permanent'' option for \fLm2cfe\fP.
.\""" 
.\""" Here's another example, on a Sun-4 running SunOS 3.x:
.\""" .nf
.\""" .in +5n
.\""" \fL%\fP\f(LB  arch ; strings /vmunix | egrep 'SunOS|UNIX'\fP
.\""" \fLsun4\fP
.\""" \fL\s-4SunOS Release Sys4-3.2 (GENERIC) #1: Wed Nov 11 12:33:42 PST 1987\s+4\fP
.\""" \fL%\fP\f(LB  f77 -O4 f.f p.p\fP
.\""" \fL\s-4f.f:
.\""" /usr/lib/f77pass1 -O f.f /tmp/f77pass1.26982.s.0.s /tmp/f77pass1.26982.i.1.s /tmp/f77pass1.26982.d.2.s
.\""" /usr/lib/iropt -O4 -o /tmp/iropt.26982.3.ir /tmp/f77pass1.26982.i.1.s
.\""" /usr/lib/cg /tmp/iropt.26982.3.ir >/tmp/cg.26982.4.s
.\""" /bin/as -o f.o -O4 /tmp/f77pass1.26982.s.0.s /tmp/cg.26982.4.s /tmp/f77pass1.26982.d.2.s
.\""" p.p:
.\""" /lib/cpp -Dunix -Dsun -Dsparc -undef p.p >/tmp/cpp.26982.5.pi
.\""" /usr/lib/pc0 -o /tmp/pc0.26982.6.none -O /tmp/cpp.26982.5.pi
.\""" /usr/lib/f1 /tmp/pc0.26982.6.none >/tmp/f1.26982.7.s
.\""" /usr/lib/inline -i /usr/lib/pc2.il </tmp/f1.26982.7.s >/tmp/inline.26982.8.s
.\""" /bin/as -o p.o -O4 /tmp/inline.26982.8.s
.\""" Linking:
.\""" /bin/ld -e start -u _MAIN_ -X -o a.out /lib/crt0.o f.o p.o -lF77 -lI77 -lU77 -lm -lc\s+4\fP
.\""" .in -5n
.\""" .fi

\fLtrailing_options\fP are ``permanent'' options which need to be added to
the \fIend\fP of the command line.
This is used by the \fL-U\fP and \fL-D\P options,
so that user-provided preprocessor (\fL/lib/cpp\fP) options will apprear
after the \*(Lc-provided arguments to \fL/lib/cpp\fP.
This is currently for aesthetics.
If \fLcpp\fP's \fL-U\fP changes to allow it to be the full semantic
equivalent of \fL#undef\fP, then this ordering will matter.
``\fL-Qoption \fP\fIpass option\fP'' also uses \fLtrailing_options\fP,
so that it can be used to override default options to any pass.

\fLoptions\fP points to a list of the options to pass the program,
on a per-execution basis.
These options may change between executions of the compiler pass.
E.g., in the \fLm2c\fP example under the explanation
of ``\fLpermanent_options\fP'' above,
the ``\fL-mod a\fP'' and ``\fL-def b\fP'' options to the \fLm2cfe\fP
program changed between its two invocations.
The \fLoptions\fP field is cleared and the list re-populated prior to
each invocation of the program with which it is associated.

\fLinfile\fP points to a list of input file names for the program.

\fLoutfile\fP points to a single output file name for the program.

\fLhas_been_initialized\fP is \fLFALSE\fP until \fLpermanent_options\fP
has been set up, then it is set \fLTRUE\fP.
Its sole purpose in life seems to be preventing \fLpermanent_options\fP
from being set more than once (thus is probably just a time optimization);
it might have well have been named something like
\fLpermanent_options_have_been_set\fP.

\fLtemplate\fP is an array giving a template for the order in which
input filenames, output filenames, and options should be passed to the
program.
It also specifies whether the standard input and/or standard output
should be set up for the program.
If the standard input is used,
the value \fLSTDIN_TEMPLATE\fP \fImust\fP be in the first element of
\fLtemplate\fP, i.e. \fLtemplate[0]\fP must equal \fLSTDIN_TEMPLATE\fP\**.
.FS
This is relied upon in
'in +2m
file \fLrun_pass.c\fP,
function \fLbuild_argv()\fP,
in the \fLSTDOUT_TEMPLATE\fP case of a \fLswitch\fP.
This dependency could be removed by scanning all of the next \fLstep\fP's
template instead of just looking at \fL(step+1)->program->template[0]\fP.
.in -2m
.FE

.Sm
.TS
center;
lfL	s	s
lfL  6	lfL	lfL.
typedef struct {
\|	ProgramP	program;
\|	SuffixP	out_suffix;
\|	int	(*expression)();
\|	char	*(*setup)();
\|	struct timeval	start;
\|	short int	process;
\|	Bool	killed :1;
.T&
lfL	s	s.
} Step;
.TE
.Lg
\fLprogram\fP
.Incomplete

\fLout_suffix\fP
.Incomplete

\fLexpression\fP
.Incomplete

\fLsetup\fP
.Incomplete

\fLstart\fP
.Incomplete

\fLprocess\fP
.Incomplete

\fLkilled\fP
.Incomplete

.Sm
.TS
center;
lfL	s	s	s
lfL  6	lfL	s	lp-2.
typedef enum {
\|	end_of_list = 1,	/* The last option in the list of options */
\|	help_option,	/* Show help message */
\|	infile_option,	/* This option is an infile */
\|	lint1_option,	/* pass to lint1 */
\|	lint_i_option,	/* Special check for lint -n & -i to make sure*/
\|	lint_n_option,	/*    they do not have more options trailing*/
\|	make_lint_lib_option,	/* Make lint library option */
\|	module_option,	/* Force load modula module */
\|	module_list_option,	/* m2c search path option */
\|	optimize_option,	/* -O/-P options */
\|	outfile_option,	/* next arg is the outfile */
\|	pass_on_lint_option,	/* Pass to lint1 & lint2 */
\|	pass_on_select_option,	/* -Qoption handler */
\|	pass_on_1_option,	/*        -x      => prog -x      */
\|	pass_on_1t12_1t_option,	/*        -xREST  => prog -xREST  */
\|		/* [or] -x REST => prog -xREST */
\|	pass_on_1t_option,	/*        -xREST  => prog -xREST  */
\|	pass_on_12_option,	/*        -x REST => prog -x REST */
\|	pass_on_1to_option,	/*        -xREST  => prog REST    */
\|	produce_option,	/* -Qproduce handler */
\|	path_option,	/* -Qpath handler */
\|	run_m2l_option,	/* m2c -e handler */
\|	load_m2l_option,	/* m2c -E handler */
\|	set_int_arg_option,	/* Handle simple boolean options */
\|	set_named_int_arg_option,	/* Handle multiple choice options */
\|	set_target_arch_option1,	/* -{TARGET}        (set target_arch) */
\|	set_target_arch_option2,	/* -target {TARGET} (set target_arch) */
\|	set_target_proc_option1,	/* -{PROCESSOR}     (set target processor
\|		 *                   type & target_arch)
\|		 */
\|	set_sw_release_option,	/* next arg is the target S/W release */
\|	temp_dir_option	/* Handle -temp option */
.T&
lfL	s	s	s
lfL	s	s	s.
} Options;

.T&
lfL	s	s	s
lfL  6	lfL	lfL	lfL.
typedef struct {
\|	char	*name;
\|	int	drivers :16;
\|	Options	type    :8;
\|	int	subtype :8;
\|	Named_intP	variable;
\|	ProgramP	program;
\|	Const_intP	constant;
\|	char	*help;
.T&
lfL	s	s.
} Option;
.TE
.Lg
.Incomplete

.Sm
.TS
center;
lfL	s	s	s
lfL  6	lfL	lfL	lp-2.
#define set_flag(flag)	global_flag.flag= TRUE
#define reset_flag(flag)	global_flag.flag= FALSE
#define is_on(flag)	(global_flag.flag == TRUE)
#define is_off(flag)	(global_flag.flag == FALSE)
.TE
.Lg
.Incomplete

.Sl "\f(LBcompile.c\fP" "\fLcompile.c\fP"
.Incomplete

.Sl "\f(LBro_data.c\fP" "\fLro_data.c\fP"
.Incomplete

.Sl "\f(LBrw_data.c\fP" "\fLrw_data.c\fP"
.Incomplete

.Sl "\f(LBrun_pass.c\fP" "\fLrun_pass.c\fP"
.Incomplete

.Sl "\f(LBsetup.c\fP" "\fLsetup.c\fP"
.Incomplete

.Sl "\f(LBsetup_cc.c\fP" "\fLsetup_cc.c\fP"
.Incomplete

.Sl "\f(LBsetup_f77.c\fP" "\fLsetup_f77.c\fP"
.Incomplete

.Sl "\f(LBsetup_lint.c\fP" "\fLsetup_lint.c\fP"
.Incomplete

.Sl "\f(LBsetup_m2c.c\fP" "\fLsetup_m2c.c\fP"
.Incomplete

.Sl "\f(LBsetup_pc.c\fP" "\fLsetup_pc.c\fP"
.Incomplete

.Sl "\f(LBtest_dir/driver_test.c\fP" "\fLtest_dir/driver_test.c\fP"
.Incomplete

.Sc Cross-Compilation

\*QCross-compilation\*U means translating source code (on a host system) into
executable code which is designed to run on a target system
which is meaningfully different from the host system.
\*QDifferent\*U applies to:
.IP \(bu 3
System architecture types;
compiling code on a Sun-4 for a Sun-3 target machine is an example of 
\fIcross-architecture\fP compilation.
.IP \(bu
Operating system types;
compiling code on a PC/AT under XENIX
to produce binaries which will run on a PC/AT under MS-DOS,
and compiling code on a Sun-4 for a SPARC chip embedded in a controller card
are examples of \fIcross-OS\fP compilation.
.IP \(bu
Versions of a single operating system;
compiling code on a Sun-3 under SunOS-4.0 to run on a Sun-3 under SunOS-3.2
is an example of \fIcross(-OS)-release\fP compilation.
.LP

There has been discussion within the Programming Languages group as to which
types of cross-compilation we should support.
Support for all three types is present in \*(Lc,
but we only publicly claim to support cross-architecture compilation
(via our Cross-Compiler products).
Cross-OS compilation will be needed to support non-UNIX SPARC software
development.
The need for cross-release compilation is not likely to be strongly
established.

Cross-compilation typically refers to the first type,
cross-architecture compilation.
This is the type to which this this document refers,
except when stated otherwise.

The \*Q\fL-target \fP\fItarget_arch\fP\*U option specifies the target
architecture\**
.FS
For discussion of how to specify a target software release,
'in +2m
see the \fBTarget Software Releases\fP section.
.in -2m
.FE
for which one wishes to compile.
It may be abbreviated to \fL-\fP\fItarget_arch\fP for Sun target architectures.

If the host and requested target architectures are the same
(and the host and target software releases are the same),
native compilation is performed\**.
.FS
The section \fBSupport for NSE Variants\fP describes an exception to this.
.FE
Otherwise, cross-compilation is triggered.

Cross-compilation requires that during compilation
the native versions of compiler passes, \fL#include\fP files,
and libraries be supplanted with replacements appropriate to the target
architecture.
\*(Lc accomplishes this (transparently to the user)
by setting an environment variable named \*(Vr\**.
.FS
For brevity,
\*(Vr and \*(Vv
'in +2m
are both occasionally referred to as VROOT
in documentation and source-code comments.
.in -2m
.FE
\*(Vr functions similarly to the \fLPATH\fP environment variable,
except that it applies not only to executable files but to virtually
\fIall\fP files opened by the various compiler passes.
\*(Vr's value consists of a colon-separated list of directories.
When a program using \*(Vr tries to open a file with an absolute
pathname (i.e., the filename starts with `\fL/\fP'),
each component of \*(Vr is treated as the root of a filesystem, in order,
until the file can be opened under one of them.

All compilation programs (such as \*(Lc, \fLcpp\fP, \fLas\fP, \fLld\fP)
which open files or \fLexec()\fP other programs (e.g. compiler passes)
use functions from the \fLvroot\fP library.
Each function in the \fLvroot\fP library
replaces a system-call function which may be passed a filename.
An example function is \fLopen_vroot()\fP, which replaces \fLopen()\fP.
The behavior of any function in the \fLvroot\fP library
is modified by the \*(Vr environment variable, as decribed above,
when it is passed a filename with an absolute path.

.KS
During cross-compilation, \fLsetup_for_cross_compile()\fP usually\** 
.FS
In the case when a specific target software release is requested,
'in +2m
\*(Vr is actually set to:
.ce
\*(Ht\fLR\fP\fIsw_release\fP\fL:\fP\fIinherited_\s-2VIRTUAL_ROOT\s+2\fP
.br
See the section on \fBTarget Software Releases\fP for more information.
.in -2m
.FE
sets the value of \*(Vr to:

.ce
\*(Ht\fL:\fP\fIinherited_\s-2VIRTUAL_ROOT\s+2\fP
.br
.KE

The first component, \*(Ht, is referred to as \*(Vv in this document.
It is sometimes called \*Qthe\*U virtual root.
This nomenclature causes it to be confused with \*(Vr,
of which it is actually only one component.

\*(Vr comprises a list of potentially many filesystem paths.
However, given that \*(Vr is an undocumented internal mechanism,
users should not be assigning it a value themselves.
Since \*(Lc is the only software tool which sets \*(Vr,
and presently doesn't \fLexec()\fP itself,
\fIinherited_\s-2VIRTUAL_ROOT\s+2\fP will typically be a null string.
So, during cross-compilation \*(Vr should look like:

.ce
\*(Ht\fL:\fP

.KS
Or, for a more concrete example:

.ce
\fL/usr/arch/sun3/sun4:\fP
.KE

Thereafter during that compilation,
when a compiler pass (e.g. \fLcpp\fP) tries to open file
\fLfoo.c\fP, it opens \fLfoo.c\fP in the current directory.
But if it tries to open a file with an absolute pathname,
say \fL/usr/include/foo.h\fP, using \fLopen_vroot()\fP,
the library function would first try opening
\fL/usr/arch/sun3/sun4/usr/include/foo.h\fP.
If that failed
(since there is a zero-length string as the second component of \*(Vr),
it would try to open \fL/usr/include/foo.h\fP.

The null second component of \*(Vr (created by the final colon)
is seldom of use to Sun's packaged cross-compiler products themselves,
since they include a complete replacement set of compiler passes\**,
.FS
Except for \fL/usr/bin/lint\fP and \fL/usr/lib/lint\fP,
'in +2m
which are not distributed with Cross-Compilers 2.0.
The null second component of \*(Vr causes the native \fLlint\fP tools
to be used in that case.
.in -2m
.FE
\fL#include\fP files, and libraries under the appropriate VROOT
directory (see \*QFilesystem Structure\*U below).
However, the null second component is needed when a user specifies an
absolute pathname to a file, so that the file is searched for under the
root of the \fIuser's\fP filesystem,
as well as under \*(Vv (the first component of \*(Vr).

For example, take the following cross-compilation command:
.ce
\fLmy_sun3%\fP\f(LB  cc  /usr/bob/pgm.c  -target sun4\fP
.br
In this case, without the final colon in \*(Vr,
\fLcpp\fP would only try to open \fL/usr/arch/sun3/sun4/usr/bob/pgm.c\fP,
and fail.
With the colon in \*(Vr,
it then attempts to open \fL/usr/bob/pgm.c\fP and succeeds.

.Sl "Filesystem Structure"
\*(Lc expects a directory to be present on the host machine which contains
all the \*(Vv\|s needed for cross-compilation.
In Cross-Compilers release 2.0, that directory is hardwired to be \*(Ua\**.
.FS
In subsequent releases, the cross-compilation home directory will be
'in +2m
specifiable in the \*(Ch environment variable.
The default directory will remain \*(Ua.
.in -2m
.FE

The various \*(Vr directories are named:
.ce
\*(Ht
The \fIhost_arch\fP level is present so that heterogeneous hosts
can conveniently share these files across the network from a single fileserver.

In Cross-Compilers 2.0, there also exist directories named \*(Ct and \*(Hc,
containing files which are common across host architectures for a given target,
and common across target architectures for a given host, respectively.
.\""" \*(Lc is blissfully unaware of their presence;
.\""" the \*(Ht directories contain symbolic links into the common directories
.\""" wherever appropriate, so \*(Lc need only look under \*(Ht to find files.
This saves considerable amounts of disk space;
Server installaion of a full set of Cross-Compilers 2.0
(6 directions of cross-compilation)
would require approximately 120Mb if there were no sharing via
common directories, but given them, consumes only 37Mb of disk space.

\*(Lc is blissfully unaware of the presence of the common directories.
The \*(Ht directories contain symbolic links into the common directories
wherever appropriate, so \*(Lc only needs to look under \*(Ht to find files.

.Sl "Target Software Releases"
.LP
As stated earlier,
cross-compilation can just as easily refer to
cross-OS compilation or cross-OS-release compilation,
as to cross-architecture compilation.
\*(Lc can handle all three types of cross-compilation
by ensuring that each combination of
system architecture/OS/OS-version causes a different \*(Vv to be used.

As long as the compiler passes under each of the different \*(Vvs
are invoked identically, this works fine.
The \*Qgotcha\*U is that if there are any differences among the \*(Vvs
in how the compiler passes are invoked, 
the driver must adjust its behavior accordingly.

For instance,
if \*(Lc was to simultaneously support compilers from both SunOS 3.x
and SunOS 4.0, it would have to know about differences such as:
.IP \(bu 3
\fLiropt\fP may be run for FORTRAN compilations in either release,
but for C and Pascal compilations it can only be run during 4.0 compilations.
.IP \(bu
The \fL-p\fP flag is passed to \fLccom\fP as \fL-XP\fP in 3.x,
but as \fL-p\fP in 4.0; similarly for \fL-J\fP.
.IP \(bu
Sun-2/3 floating-point libraries are found in
\fL/usr/lib/\fP\fIfloat_option\fP in 4.0.
.IP \(bu
\*Q\fLld\fP\*U gets \fL-dc\fP and \fL-dp\fP options by default in 4.0,
but doesn't even recognize them in 3.x.
.LP

\*(Lc indeed \fIdoes\fP know about these differences and can run
both native and cross-architecture/cross-release compilations \(en
although the capability to
perform cross-release compilation has (deliberately) been kept an
undocumented feature.
This has been done deliberately because cross-release compilation
would be difficult (at best) to support ad infinitum to the external world.
However, it works fine within the internal confines of Sun,
at least in the 4.0-to-3.x direction (aided greatly by the fact that
3.x binaries can run under SunOS 4.0).

\*(Lc supports execution of compiler passes from both SunOS 4.0 and SunOS 3.x,
if the preprocessor variable \fLHANDLE_R32\fP is defined when compiling \*(Lc.
This is currently the default in its Makefile.
If \fLHANDLE_R32\fP is not defined,
only SunOS 4.0-style compiler passes are supported.

This feature is made possible with the \*Q\fL-release \fP\fIsw_release\fP\*U
option, which specifies (independently of the target architecture)
the desired target software release.
For example,
one could install Cross-Compilers 2.0 (which are SunOS 3.x-based)
on a Sun-3 running SunOS 4.0, and compile 3.x Sun-2 binaries with the
command:

.ce
\fLmy_sun3%\fP\f(LB  cc  -sun2  -release 3.2  pgm.c -o pgm\fP

When the \fL-release\fP option is not specified,
\*(Lc must somehow determine whether the default target
software release is SunOS 3.x or SunOS 4.0,
so it knows what type of options to pass to the various compiler passes.
It does this by looking for \fL/usr/lib/crt0.o\fP in the filesystem
(under \*(Vr, in the case of a cross-compilation).
If it is present, it assumes that the target software release is SunOS 4.0,
since \fL/usr/lib/crt0.o\fP is not present under \fL/usr/lib\fP in SunOS 3.x.
Granted, this is a \fIgrungy\fP thing to do,
but when we handle multiple target software releases,
we have to determine the default case \fIsomehow\fP.


As suggested earlier,
a system is actually identified by not just its system architecture,
but the \fIcombination\fP of its system architecture and the
operating system which is running on it.
Therefore, the \*Qreal\*U target directories under \*(Ua have names of
\fItarget_arch\fP\fLR\fP\fIsw_release\fP.
Target directories named just \fItarget_arch\fP
(without a software release) are defaults,
referenced when no specific target software release is requested.

Although \*(Lc never references \*(Ha\fLR\fP\fIsw_release\fP,
it could be useful to install cross-compiler tools for multiple host
software releases on one server under those directories.
Each machine can reference the correct host directory
.\""" for its own architecture
as \*(Ha by
mounting \fIhost_arch\fP\fLR\fP\fIsw_release\fP
from the server on its \*(Ha\fLR\fP\fIsw_release\fP,
then making a symbolic link from
\*(Ha\fLR\fP\fIsw_release\fP to \fIhost_arch\fP.
This is the way Cross-Compilers 2.0 is installed
(see the \fBCross-Compilers 2.0\fP appendix).

Cross-compilation without a \*Q\fL-release \fP\fIsw_release\fP\*U option
compiles for the default software release for the given target.


In more detail, \*Q\fL-release \fP\fIsw_release\fP\*U has three effects:
.IP \(bu 3
It changes the \*(Vv for cross-compilation to indicate a specific 
target software release instead of the default one
(e.g., \*(Ua\fL/sun3/sun4R3.2\fP instead of \*(Ua\fL/sun3/sun4\fP).
This value is kept in \fLtarget_sw_release[R_VROOT]\fP.
.IP \(bu
It sets the directory paths under \*(Vr from which to find compiler passes
and libraries (since they differ between SunOS-3.x and SunOS-4.0).
This value is kept in \fLtarget_sw_release[R_PATHS]\fP.
.IP \(bu
It affects which compiler passes are used (e.g. \fLiropt\fP for C)
and which flags are passed to the compiler passes
(again, since these differ between SunOS-3.x and SunOS-4.0).
This value is kept in \fLtarget_sw_release[R_PASSES]\fP.
.LP

.Sl "Foreign Target Architectures"
\*(Lc provides minimal support for cross-compilation for foreign target
architectures, via the \*Q\fL-target \fP\fIother_arch\fP\*U option.
Given this option, \*(Lc:
.IP \(bu 3
Sets the environment variable \*(Vr to the value \*Q\*(Ho\fL:\fP\*U
.IP \(bu
Expects \*(Ho to be populated with compiler passes,
include files, and libraries in the same locations as for the Sun
native compilers\**
.FS
If \*(Ho\fL/usr/lib/crt0.o\fP does not exist
'in +2m
and \*(Lc was built with \fL-DHANDLE_R32\fP,
foreign compiler passes, libraries, and include files
will be expected in their SunOS release 3.x locations.
Otherwise, they will be expected in their SunOS 4.0 locations.
It is highly recommended that foreign compiler passes be set up
like those for 4.0 and that the \fLcrt0.o\fP mentioned above be present
(even if unused),
since at some point \*(Lc may no longer support 3.x-style compilers.
.in -2m
.FE
.IP \(bu
Expects that the foreign compiler passes will accept the same options as
the Sun native compiler passes\**
.FS
Ditto the previous footnote,
'in +2m
for the options which are passed to the compiler passes.
Note that if a foreign pass does not accept the options passed it by
\*(Lc, a Shell script or program can be installed which filters or
converts the options as desired, and \fLexec()\fP's the actual compiler pass.
.in -2m
.FE
.IP \(bu
Does not pass the usual \fL-Dunix\fP, \fL-Dsun\fP,
and \fL-D\fP\fItarget_mach\fP options to \fLcpp\fP
(foreign cross-compilation users must set their own)
.IP \(bu
Can still pass additional options to the foreign compiler passes
via the \*Q\fL-Qoption \fP\fIpass_name\fP\fL \fP\fIoption\fP\*U mechanism
.LP

For example,
if on a Sun-4 host,
\*(Ua\fL/sun4/abc\fP contains appropriate compiler passes for target \fLabc\fP,
then
.ce
\fLmy_sun4%  \fP\f(LBcc  -target abc  pgm.c\fP
will cross-compile \fLpgm.c\fP for target \fLabc\fP.

The \*Q\fL-release \fP\fIsw_release\fP\*U option is not meaningful
for foreign architecture cross-compilation,
since \*(Lc is ignorant of \*Qforeign software release numbers\*U.

As of the end of 1987,
the foreign-target cross-compilation functionality is being used by at
least one internal group (the CDI project; see Dan Steinberg).

.Sl "Support for NSE Variants\s-4\**\s+4"  "Support for NSE Variants"
.FS
Marty Honda and Evan Adams
'in +2m
(in the NSE group) are contacts for additional NSE information.
.in -2m
.FE
Under Sun's Network Software Environment (NSE)
one can easily generate code for multiple target architectures,
through \*Qvariants\*U of one's NSE environment.
There is one variant per target architecture.
When one changes to a new variant everything \fIlooks\fP the same,
except that all target-dependent files in the filesystem have suddenly
been replaced \(en including compiler passes, include files, and libraries.
Effectively, a \*(Vv overlays the native root filesystem
(using an internal NSE mechanism known as \*Qtranslucent mounting\*U).
Apparently-native compilations magically create object code for the new
variant (target).

Although NSE's translucent mounting mechanism supplants \*(Vr in this
situation,
\*(Lc still needs to know the target architecture for which it's compiling.
It can discover this by checking some environment variables set by NSE.
If \*(Nv is set, then an NSE environment is activated.
In that case, \*(Lc obtains the name of the desired target architecture from
the \*(Ta environment variable.
\*(Ta contains the target architecture name, prefixed by a \*Q\fL-\fP\*U.

Any time a target architecture is explicitly given on the compiler command line,
it overrides the implicit target architecture specified by the NSE environment.

A subtle case to remember is when one is in a non-native variant of
an activated NSE environment and \fIexplicitly\fP specifies the
native architecture as the target architecture on the command line\**.
.FS
For example, one is working on a Sun-3,
'in +2m
with an activated NSE variant of \fLsun4\fP,
and one gives the command ``\fLcc -sun3 foo.c\fP''.
.in -2m
.FE
Can this be treated as a normal native compilation?
The answer is \fIno\fP.
Although \*(Lc normally obtains native compiler passes, include files,
and libraries from under the root filesystem (\*Q\fL/\fP\*U), 
NSE has arranged for the \fIcross\fP versions of those files to appear
in their place.
In this case, \*(Hh must be populated with appropriate native compilation
pieces, and \*(Lc must set \*(Vr to \*(Hh\fL:\fP.
It then proceeds with the compilation as if it were a cross-compilation.

Support for NSE is contained in the routines
\fLset_target()\fP and \fLmain()\fP
.Fi compile.c  .

.Sc "Modifying \f(LB/lib/compile\fP" "Modifying \*(Lc"
.nf\"===========================
.Sl "Coding Style"
To be brief, some clever abstractions are used in \*(Lc.
These abstractions create (sometime many) layers of indirection,
which can be a bit difficult to follow at times.
Hopefully, the material in this document helps decode some of
\*(Lc's deeper secrets.

.Sl "Lexical Conventions"
Since lexical consistency aids readability,
an attempt has been made to follow the lexical conventions present in
the original code while modifying \*(Lc.
Although this author may find other lexical conventions easier to follow
(e.g. vertically lined-up braces),
he knew that either \fIall\fP of the code or \fInone\fP
of it should be changed to follow any new conventions.
Despite automated tools to do much of such conversion work
(namely, \fLindent\fP),
expediency has so far dictated that the latter path be followed.
It is suggested that consistency be maintained,
regardless of the path chosen by subsequent maintainers.

.Sl "Recipe For Adding A Command-Line Option"
Here's a cookbook procedure for adding a new command-line option
(also see the \fBProcessing Arguments\fP section):
.IP (1) 4
Locate where to place the option in the \fLoptions[]\fP array
.Fi ro_data.c .
It should be in (case-insensitive) alphabetical order with the other
options,
although ahead of any shorter option which is an initial substring
of the new option name.
.IP (2)
Choose the option type which best fits how the new option should be handled.
See the enumeration of type \fLOptions\fP in file \fLdriver.h\fP.
For details on how each type is processed,
see \fLlookup_option()\fP in file \fLcompile.c\fP.
If there is an existing option which is handled similarly,
use it as a pattern.
If a new option type must be created:
.RS
.RS
.IP \(bu 3
add the entry to ``\fLtypedef enum {\fP...\fL} Options\fP'' in \fLdriver.h\fP
.IP \(bu 3
add \fLcase\fPs for it in the \fLswitch\fP tables in \fIboth\fP
\fLprint_help()\fP and
\fLlookup_option()\fP
.IP \(bu 3
create a new \fLOPTION_*()\fP macro for it in \fLro_data.c\fP
.RE
.RE
.IP (3)
Decide to which driver(s) the option will apply.
Select the appropriate \fLDRIVER_*\fP macro from the list in \fLdriver.h\fP
(create a new one if necessary).
.IP (4)
If the option is for internal use only,
i.e. should not be listed when the \fL-help\fP option is used,
embed the \fLDRIVER_*\fP macro reference
from step (3) in a \fLHIDE()\fP macro call,
i.e. \fLHIDE(DRIVER_*)\fP.
.IP (5)
Insert the \fLOPTION_*()\fP line at the spot chosen in step (1),
using the macro appropriate to the type chosen in step (2).
The second argument will be the value chosen in steps (3) and (4).
Fill in the other arguments to the macro as appropriate.
.IP (6)
Insert additional code where needed, if any, to handle the option.
For example, if the option sets or clears a global flag
(i.e. \fLOPTION_SET*()\fP was used),
code must be inserted somewhere to check that flag and act accordingly.
If a new option type was added,
there will be code added in the \fLcase\fP for that type
in function \fLlookup_option()\fP,
and elsewhere as necessary.
.LP

.Sl "Recipe for Adding a New Architecture"
.Incomplete

.Sl "Recipe for Adding a New Driver Type [?]"
.Incomplete

.Sl "Recipe for Adding a New Compiler Pass [?]"
.Incomplete

.Sl "General Hints"
Take care when modifying \*(Lc
to maintain support for invocation of 3.x compiler passes
(i.e. as long as support for cross-release compilation continues).
In this regard, sensitive modifications include:
.IP \(bu 3
Adding an argument to be passed to a compiler pass.
If it is an option only supported by 4.0 compiler passes,
make sure it is \fInot\fP passed to 3.x passes.
For an example, see handling of the 4.0 linker's \fL-dc\fP and \fL-dp\fP flags
in \fLsetup_ld()\fP
.Fi setup.c  .
.IP \(bu
Changing how an argument is passed to a compiler pass.
Ditto the above comment.
For an example, see handling of \fL-J\fP flag for \fLccom\fP
in \fLsetup_ccom()\fP
.Fi setup_cc.c  .
.IP \(bu
Changing which compiler passes are invoked
(e.g. when Modula-2 starts using IROPT).
For an example of how both versions are supported
(via two sets of compiler steps),
see \fLsetup_pc0_for_3x()\fP and
\fLsetup_pc0_for_non_3x()\fP
.Fi setup_pc.c  .
.LP

Expect to use
.\""" .sp 0.2v
.in +4m
\fL#ifdef HANDLE_R32\fP
.\""" .vs -4p
.br
.\""" \&\s+4...\s-4
\&\s-8\(bu   \(bu   \(bu\s+8
.br
\fL#endif\fP
.in -4m
.\""" .vs +4p
and
.ti +4m
\fLswitch (target_sw_release[R_PATHS].value->value)\fP
.br
when dealing with differences between releases 3.x and 4.0.

.Sc "Regression Testing"

A regression test suite is available for \*(Lc
(currently on the Language group's integration machine, \*Qrogue\*U,
but not yet on 4.0 release machines).
It can be run by changing directories to \*(Lc's main source directory,
\fLlang/compile\fP, and issuing a \*Q\fLmake test\fP\*U command.

The test suite runs (all flavors of) the compiler driver
on a large number of test cases, using \fL-dryrun\fP.
From approximately 150 basic command lines,
over 4000 command lines are generated by the regression test.
One line is echoed to the TTY for each command tested.
The many lines of \fL-dryrun\fP output from each executed command
are compared with previously-stored (and supposedly correct) output.
If there are any differences (as reported by \*Q\fLdiff\fP\*U),
those differences are placed in files with \*Q\fL.diff\fP\*U suffixes.

At the end of the regression test, success or failure is reported.
If the test has failed,
the list of files containing the differences (the \*Q\fL.diff\fP\*U files)
is reported.

Current deficiencies of this testing include:
.IP \(bu 3
There is no way to automatically test \fIonly\fP native compilation.
Cross-compilation testing is always done along with native compilation testing;
this is hardwired in the \fLdriver_test\fP program.
Therefore, for all tests to pass,
cross-compilers have to be installed on the machine on which the test is run.
.IP \(bu
The program which does the regression testing, \fLdriver_test\fP,
does not produce a return code.
So, there is no direct way to tell if the regression test passed or not.
The \fLMakefile\fP simply checks for the presence/absence of the
\*Q\fL.diff\fP\*U files.
.LP

.Sc "Debugging Features"

.Sl "\f(LBx\fP\f(BIdriver\fP, \f(LBX\fP\f(BIdriver\fP" "\fLx\fP\fIdriver\fP, \fLX\fP\fIdriver\fP"
If the name by which \*(Lc is invoked begins with \*Q\fLx\fP\*U,
the \*Q\fLx\fP\*U is stripped off and the driver acts as if the \fL-dryrun\fP 
option had been given.
Thus, \*Q\fLxcc c.c\fP\*U acts identically to \*Q\fLcc -dryrun c.c\fP\*U.
In \*(Lc's source directory,
\*Q\fLmake xlinks\fP\*U will create symbolic links to \fLcompile\fP
from \fLxcc\fP, \fLxf77\fP, \fLxlint\fP, \fLxm2c\fP, and \fLxpc\fP.
\*Q\fLmake Xlinks\fP\*U will create symbolic links to \fLcompile\fP
from \fLXcc\fP, \fLXf77\fP, \fLXlint\fP, \fLXm2c\fP, and \fLXpc\fP.

If the first character of the driver name is \*Q\fLX\fP\*U,
the \*Q\fLX\fP\*U is stripped off and the driver acts as if both the
\fL-dryrun\fP and \fL-normcmds\fP options had been given.

.Sl "\f(LB-normcmds\fP" "\fL-normcmds\fP"
When testing the driver,
\*Q\fLrm\fP\*U commands (which remove temporary files)
tend to clutter up the output from \fL-v\fP or \fL-dryrun\fP.
The \fL-normcmds\fP option suppresses the echoing (but not execution)
of \*Q\fLrm\fP\*U commands.  (also see \fL-keeptmp\fP, below)

.Sl "\f(LB-debug\fP" "\fL-debug\fP"
The option \fL-debug\fP sets the internal Boolean variable
\fLglobal_flag.debug\fP to the value \fLTRUE\fP.
Use of \fL-debug\fP currently causes a small amount of debugging output
when the compiler driver is executed.
Developers should feel free to \fL#ifdef\fP-out, comment-out,
or delete code whose execution depends on the value of \fLglobal_flag.debug\fP.
All such code looks like:
.nr VS -4
.vs    -4p
.DS B
\fLif ( is_on(debug) ) {
	\s+8...\s-8
}\fP
.DE
.nr VS +4
.vs    +4p

.Sl "\f(LB-verbose\fP, \f(LB-v\fP, \f(LB-dryrun\fP" "\fL-verbose\fP, \fL-v\fP, \fL-dryrun\fP"
\fL-verbose\fP causes compilation commands to be echoed to \fLstderr\fP
as they are executed by \*(Lc.
\fL-v\fP is shorthand for \fL-verbose\fP for all drivers except \fLlint\fP
(which already had a \fL-v\fP option).

\fL-dryrun\fP causes the same output as \fL-verbose\fP,
but the compilation commands are not executed (hence, a \*Qdry run\*U).

.KS
.Sl "\f(LB-rel{vroot,paths,passes}\fP" "\fL-rel{vroot,paths,passes}\fP"
The three effects of \*Q\fL-release \fP\fIsw_release\fP\*U
can also be \fIindividually\fP controlled using the unadvertised options:
.TS
center;
c	cw(3.2i)
c	c
lfL	l.
Option	Effect
\_	\_
-relvroot \fIsw_release\fP	T{
Sets the \fIsw_release\fP used in \*(Vv (therefore, affects \*(Vr).
T}
-relpaths \fIsw_release\fP	T{
Determines paths where compiler passes and
libraries are expected to be found, under \*(Vv.
T}
-relpasses \fIsw_release\fP	T{
Determines which compiler passes are invoked,
and how options are passed to them.
T}
.TE
.KE
For example,
\fL-relpasses\fP \fIsw_release\fP has been used to run 4.0 compiler passes
from a directory referenced by \fL-Qpath\fP,
on a system otherwise populated with 3.x software.

.Sl "\f(LB-keeptmp\fP" "\fL-keeptmp\fP"
The compiler driver normally removes temporary files as soon as they are
no longer needed.
Deletion of temporary files is suppressed by use of the \fL-keeptmp\fP option.

.Sc "Grocery List for the Future"

.IP (1) 5
\*(Lc for unbundled products should now be built and installed
as separate driver programs,
instead of multiple symbolic links to a common (bundled) binary.
The bundled drivers (\fLcc\fP and \fLlint\fP) could still share
a common binary, if desired.

.IP (2)
Knowledge of other drivers should be weeded out each driver's own binary.
Otherwise, for example, the C compiler driver could be used to compile
a Pascal (``\fL.p\fP'') source file,
but using the wrong sequence of Pascal passes,
or passing out-of-date options to the unbundled Pascal passes.

.IP (3)
Source code to \*(Lc needs to be reorganized, to support (1) above.
For ease of maintenance,
it continues to be desirable to keep as much driver code as possible
in \fIone\fP place.
The following is a suggestion for a revised source hierarchy, where
.B src
is a directory containing common source code for all of the compiler drivers.
.ds Ci "\v'+0.2v'\s+8\(ci\s-8\v'-0.2v'\"----draw circle----
.\"" .ds Cs "\*(Ci\h'-\w'\*(Ci'/2u-0.5n'/\h'+\w'\*(Ci'/2u+0.5n'\"---circle+slash
.ds Cs "\*(Ci\h'-\w'\*(Ci'/2u-0.6n'/\"---circle+slash
.\"===========================================================================
.ds Bv "\h'0.9n'\(bv
.TS
center;
l	l	l	lfL	s	s	s
l	l	l	lfL	s	s	s
l	l	l	lfL	s	s	s
l	l	l	lfL	s	s	s
l	l	l	lfL	s	s	s
cfL	s	s	s	s	s	s.
			\h'3m'\*(Bv lang
			\h'3m'\*(Cs
			\h'3m'\*(Bv compile
			\h'3m'\*(Cs
			\h'3m'\*(Bv
_
.T&
lfL	lfL	lfL	lfL	lfL	lfL	lfL.
\*(Bv \f(LBsrc\fP	\*(Bv cc	\*(Bv f77	\*(Bv pc	\*(Bv pc1.05	\*(Bv m2c	\*(Bv lint
\*(Cs	\*(Cs	\*(Cs	\*(Cs	\*(Cs	\*(Cs	\*(Cs
/\\\\	/\\\\	/\\\\	/\\\\	/\\\\	/\\\\	/\\\\
.TE
.\"===========================================================================
.IP (4)
Due to item (3),
\fLtest_dir\fP and its component files may need to be redistributed under the
component drivers' directories, as appropriate.

.IP (5)
The \fLdriver_test\fP program should be made to produce a useful
return code, so that the success or failure of regression testing would
be easier to report from \fLtest_dir/Makefile\fP.
The \fLMakefile\fPs should be updated accordingly.

.IP (6)
Compile-time support for the RoadRunner system (\fLsun386\fP architecture)
has been merged (from ECD source code) into a SunOS-4.1 version of \*(Lc
(see David Weaver for this code's location).
However, this code only supports \fLsun386\fP
.I native
compilation.
Cross-compilation to and from the \fLsun386\fP will require changing
the numerous compile-time \fL#ifdef i386\fP's in the code into appropriate
runtime decisions.

.IP (7)
There is an outstanding bug for the Modula-2 compiler driver which should
be fixed.  It is immortalized in BugTraq Bug-ID #1007768.

.IP (8)
\*(Lc should also be able to function as a driver for
the assembler (``\fLas\fP'') and linker (``\fLld\fP'') programs.
This would enable cross-developers to access \fIall\fP of the
user-visible programs in a consistent manner.
Specifically, they would be able to use the
``\fL-target \fP\fItarget_arch\fP'' (or ``\fL-\fP\fItarget_arch\fP'')
flags when invoking \fLas\fP and \fLld\fP,
and get the proper cross-compilation behavior.

.IP (9)
The commentary (entitled ``The Big Picture'') at the beginning of file
\fLcompile.c\fP should either be updated,
or replaced with a reference to this document.

.IP (10)
We may want to remove knowledge of \fLratfor\fP from \*(Lc,
now that \fLratfor\fP has been de-supported [per Mark Scott Johnson].
Before hacking it out of \*(Lc,
we should double-check that we won't end up supporting \fLratfor\fP
all over again when the big SunOS/System-V merge is done
(or users form a lynch mob demanding its retention).

.IP (11)
There are places in \*(Lc which could be changed to make
it adhere to stricter type-matching.
Examples:
.nr VS -4
.IP
.TS
center;
c   c
c   c
lfL lfL.
Current Code	Better Code
\_	\_
.sp 0.8v
char  *foo();	char  *foo();
if (foo())	if ( foo() != (char *)NULL )
.sp 0.8v
char  c;	char  c;
c = 0;	c = '\\\\0';
.sp 0.8v
char  *cp;	char  *cp;
if (*cp == 0)	if (*cp == '\\\\0')
.TE
.nr VS +4
.IP
Granted, these are nits,
but one might as well fix them when one runs across them.

.IP (12)
\*(Lc has not been \fLlint\fP'd in a long time.
It should be.

.IP (13)
It wouldn't hurt to split \fLcompile.c\fP into more than one source file.

.IP (14)
It would be helpful to change \*(Lc's regression testing so that
one could test just the native portion of the driver during development.
It currently always tests both native and cross-compilation;
therefore the tester must have the most recent Cross-Compiler product
installed\**.
.FS
And \fIthat\fP won't even work,
'in +2m
if one is testing an updated version of \*(Lc which acts differently during
cross-compilation than the most recent Cross-Compiler product does.
.in -2m
.FE
.LP
.\"----------------------------------------------------------------------------
.bp
.nr PS 22
.nr VS 32
.LP
.ce 99
\s+6\fBAppendix A\fP\s-6
\fBDefinition of Terms\fP
.ce 0

.nr PS 12
.nr VS 16
.LP
.XS
 \" this line intentionally blank, to leave blank line in the T.O.C.
Appendix A:  Definition of Terms
.XE
Italicized terms used in this document may take on one of many values,
as enumerated below.

.TS
l	l	l
l	l	l
lfI	lfL	l.
Term	Value	Meaning
\_	\_	\_

driver	cc	C compiler driver
	f77	FORTRAN-77 compiler driver
	lint	FORTRAN-77 compiler driver
	m2c	Modula-2 compiler driver
	pc	Pascal compiler driver


suffix	c	file suffix for C source code, before \fLcpp\fP
	i	file suffix for C source code
	p	file suffix for Pascal source code, before \fLcpp\fP
	pi	file suffix for Pascal source code
	F	file suffix for FORTRAN source code, before \fLcpp\fP
	f	file suffix for FORTRAN source code
	def	file suffix for Modula-2 definition file
	mod	file suffix for Modula-2 module file
	sym	file suffix for Modula-2 symbol file
	a	file suffix for library files (created by \fLar\fP)
	o	file suffix for relocatable object file
	il	file suffix for inline expansion template file
	S	file suffix for assembler source code, before \fLcpp\fP
	s	file suffix for assembler source code
	ln	file suffix for \fLlint\fP library file


arch,	sun2	Sun-2 system architecture
target_arch,	sun3	Sun-3 system architecture [Carrera, Prism, Sirius]
host_arch	sun4	Sun-4 system architecture [Sunrise, Cobra, ...]
	sun386\(dd	Sun-386 system architecture [RoadRunner]
	\fIother_arch\fP	Other system architecture (see Cross-Compilation)

mach,	mc68010	Motorola MC68010 machine type
target_mach,	mc68020	Motorola MC68020 machine type
host_mach	sparc	SPARC machine type
	i386\(dd	Intel 80386 machine type

sw_release	3.2	SunOS release 3.2, 3.4, 3.5, or Sys4-3.2
	4.0	SunOS release 4.0

virtual_root		T{
A directory under \*(Ua, typically \*(Ht;
used as the first component of
the \*(Vr environment variable.
T}
.TE
.FS \(dd
\*(Lc does not yet support the \fLsun386\fP architecture,
'in +2m
but should do so in SunOS release 4.1.
.in -2m
.FE
.\"----------------------------------------------------------------------------
.bp
.nr PS 22
.nr VS 32
.LP
.ce 99
\s+6\fBAppendix B\fP\s-6
\fBCross-Compilers 2.0\fP
.ce 0

.nr PS 12
.nr VS 16
.LP
.XS
Appendix B:  Cross-Compilers 2.0
.XE

This appendix describes in more detail the specifics of how our
Cross-Compilers 2.0 product is contructed, distributed, installed,
and used.

.Sl "Releases"
The Cross-Compiler products currently released or planned are:
.TS
center;
c	c	s	s	s
c	c	s	s	s
c 2	c 2	c 2	c 2	c
c	c	c	c	c
c	c	c	c	c
c	c	c	c	c.
	Supports
Cross-	\_
Compilers	Host	Host SunOS	Target	Target SunOS
Release	Architecture(s)	Version(s)	Architecture(s)	Version(s)
\_	\_	\_	\_	\_
1.0	Sun2,3	3.x	Sun2,3	3.2
2.0	Sun2,3,4	3.x, Sys4-3.2	Sun2,3,4	3.x, Sys4-3.2
2.1	Sun2,3,4	4.0	Sun2,3,4	4.0
3.0	Sun2?,3,4,386	4.1	Sun2?,3,4,386	4.1
.TE
.ds Cc "Cross-Compiler
.ds Xs "\*(Cc Server
.ds Xc "\*(Cc Client
.ds Pr "\fL/private\fP
.ds Pu "\*(Pr\fL/usr/arch\fP
.Sl "Installation"
For \*(Ccs 2.0 (and later), there are two types of installation:
\*(Xs installation and \*(Xc installation.
A \*(Xs is a system with a local disk,
onto which the software is loaded.
A \*(Xc is a system on which the \*(Cc software will be executed.
Note that:
.IP \(bu 3
\*(Xss and Clients are independent and not mutually exclusive.
A \*(Xs typically supports one or more \*(Xcs,
but a \*(Xc may be its own \*(Xs,
and two systems could be \*(Xcs of each other
(although there would be no practical reason to do so).
.IP \(bu
\*(Xss and Clients should not be confused with
ND servers and clients.
An ND client's ND server is not necessarily its \*(Xs.
\*(Xss and Clients may be (and are to some degree encouraged to be)
heterogeneous machines on the network,
which is seldom the case with ND clients and their servers.
.LP

\*(Cc installation is performed in two steps:
Server installation and Client installation.
There is a Shell script for each, placed under directory
\*(Us\fL/install\fP during Server installation.

Server installation (script \*(Us\fL/install/server\fP)
involves reading the distribution tape(s) onto a machine with a local disk,
under the directory \*(Us.
That machine then becomes the \*(Xs for subsequent Client installations.

Client installation (script \*(Us\fL/install/client\fP):
.IP \(bu 3
Sets up a private copy of \*(Ua on the Client machine.
\*(Ccs 2.0 does this by making \*(Ua a symbolic link to \*(Pu,
then populating \*(Pu.
.IP \(bu
Creates directories under \*(Pu for all appropriate
\fIhost_arch\fP\fLR\fIsw_release\fP\fL/\c
\fP\fItarget_arch\fP\fLR\fIsw_release\fP's.
.IP \(bu
Creates a symbolic link from
\*(Pr\*(Ha to
\fIhost_arch\fP\fLR\fIsw_release\fP.
.IP \(bu
For each supported target architecture,
creates a symbolic link from
\*(Pr\*(Ha\fLR\fIsw_release\fP\fL/\fP\fItarget_arch\fP to
\fItarget_arch\fP\fLR\fIsw_release\fP.
.IP \(bu
Creates a symbolic link from \*(Pu\fL/bin\fP to
\fIhost_arch\fP\fLR\fIsw_release\fP\fL/common/crossbin\fP.
.IP \(bu
Mounts the appropriate files from the \*(Xs onto the
\*(Pr\*(Ha\fLR\fP\fIsw_release\fP\fL/\fP\fItarget_arch\fP\fLR\fP\fIsw_release\fP's
(or, if the Server is the same machine as the Client,
symbolic links are used instead of NFS mounts).
.IP \(bu
Adds entries corresponding to the above mounts (if any were done)
to the Client's \fL/etc/fstab\fP.
.IP \(bu
(Optionally, but by default) installs a new \*(Lc on the Client machine.
.LP

.Sl "Filesystem Organization (\f(LB/usr/arch\fP, \f(LB/usr/arch.SERVER\fP)" "Filesystem Organization (\*(Ua, \*(Us)"

Default host and target directories for a Cross-Compiler Client
are created by making a symbolic link
from each \fIarch\fP to the appropriate \fIarch\fP\fLR\fP\fIsw_release\fP.
Initially, this is done by the Cross-Compiler Client Installation script.
For example, on a Sun-3 host running SunOS-4.0, after Client Installation
of Cross-Compilers 2.1 (which supports SunOS-4.0) for
both Sun-2 and Sun-4 targets, the following symbolic links would be present:
.TS
center;
lfL	c	lfL.
\*(Ua/sun3	\(->	sun3R4.0
\*(Ua/sun3/sun2	\(->	sun2R4.0
\*(Ua/sun3/sun4	\(->	sun4R4.0
.TE

The default target Sun-4 system above could be changed to a Sun-4 running
Sys4-3.2 merely by making \*(Ua\fL/sun3/sun4\fP a symbolic link to
\fLsun4R3.2\fP instead of to \fLsun4R4.0\fP\**.
.FS
Assuming that (a) the Cross-Compilers 2.0 product is installed,
'in +2m
(b) Cross-Compilers 2.0 binaries can run unchanged under SunOS-4.0
(which should be the case),
and (c) \*(Lc was built with \fL-DHANDLE_R32\fP.
.in -2m
.FE

Each Cross-Compiler Client's
\*(Ua\fL/\fP\fIhost_arch\fP directory \fImust\fP be a symbolic link
to \fIhost_arch\fP\fLR\fP\fIsw_release\fP,
where \fIhost_arch\fP and \fIsw_release\fP correspond to
the Client's architecture type and software release number, respectively.

.Sl "\f(LB/private/usr/arch\fP versus \f(LB/usr/arch\fP" "\*(Pu versus \*(Ua"
Each \*(Xc keeps a private version (under \*(Pu) of the directories
from \*(Ua down to the mount (\*(Vv) level of \*(Ht.
This private version is kept so that each Client has symbolic links
(the ones which set the default target software releases)
in those directories appropriate to its architecture and SunOS release.

.Sl "\f(LB/usr/arch.SERVER\fP versus \f(LB/usr/arch\fP" "\*(Us versus \*(Ua"
On a \*(Xs,
\*(Cc tapes are loaded into \*(Us instead of directly under \*(Ua.
This is because a homogeneous ND client of the server
(which may inherit \fL/usr\fP and therefore \*(Ua from its ND server)
may also be its \*(Xc.
In that case, 
the Client could inherit a read-only \*(Ua in which it could not set up
its own defaults (via the symbolic links discussed above).
.\"---------------------------------------------------------------------------
.EH '\fB\s+2DRAFT\s-2\fP''\fB\s+2DRAFT\s-2\fP'
.OH '\fB\s+2DRAFT\s-2\fP''\fB\s+2DRAFT\s-2\fP'
.LP
.bp
.PX \" print table of contents.
