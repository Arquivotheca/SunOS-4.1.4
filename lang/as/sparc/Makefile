#     @(#)Makefile	1.1     10/31/94
VERSION=2.20

# Set COPTS to "-O" to compile with optimization
COPTS	= -O

# Set DEBUG to "-DDEBUG" and DBX to "-g" to get debugging version
# (DEBUG=-DDEBUG should be used during development & testing; it enables
#  more internal error-checking)
DEBUG	=
DBX	=

# Use of CHIPFAB makes the assembler kludge around known bugs in the early-fab
# chips.  Set CHIPFAB to <nothing> for the final versions of the chips.
#
# Set CHIPFAB to "-DCHIPFABS -DIU_FABNO=<m> -DFPU_FABNO=<n>" for fab#<m> of the
# SPARC IU chip and fab#<n> of the FPU chip.  An <m> or <n> value of 0 turns
# off all assembler kludges for that chip.
#
# If "-DCHIPFABS" is not used, IU_FABNO and FPU_FABNO will be unreferenced in
# the code, therefore don't have to be defined here.
#
# CHIPFAB =
CHIPFAB = -DCHIPFABS -DIU_FABNO=2 -DFPU_FABNO=3

# use:	OBJFMT="-DSUN3OBJ" for sun3-like object file format,
#	OBJFMT="-DSUN3OBJ -DINTERIM_OBJ_FMT" for interim sun4 target object
#						file format,
#   and OBJFMT=<nothing> for some later "real" SPARC object file format
#						(when defined)
OBJFMT	= -DSUN3OBJ

# set PROFILE to "-p" or "-pg" to get profiling
PROFILE	=

YACC	= /usr/bin/yacc
YFLAGS	=

CC	= /usr/bin/cc
CPP	= /lib/cpp
EXTRA_CFLAGS= 
CPPFLAGS = ${DEBUG} ${CHIPFAB} ${OBJFMT}
CFLAGS	= ${COPTS} -pipe -c ${DBX} ${PROFILE} ${CPPFLAGS} ${EXTRA_CFLAGS}

LD	= /bin/ld
LDFLAGS	= ${PROFILE} ${DBX}

LINT	= lint

DESTDIR	=

# Note: for guaranteed safety, header files should be included in the order
# listed here:
HDRS	= \
	a.out.h \
	exec.h \
	structs.h \
	sym.h \
	sym_fcns.h \
	y.tab.h \
	bitmacros.h \
	errors.h \
	actions.h \
	instructions.h \
	globals.h \
	lex.h \
	functional_groups.h \
	build_instr.h \
	segments.h \
	alloc.h \
	node.h \
	emit.h \
	scratchvals.h \
	operands.h \
	check_nodes.h \
	disassemble.h \
	obj.h \
	label.h \
	lex_tables.h \
	localsyms.h \
	stab.h \
	relocs.h \
	scan_symbols.h \
	version_file.h \
	opcode_ptrs.h \
	registers.h \
	ident.h \
	optimize.h \
	opt_unique_label.h \
	opt_branches.h \
	opt_relabel.h \
	opt_dead_code.h \
	opt_utilities.h \
	opt_fcn_epilogue.h \
	opt_fcn_prologue.h \
	opt_patterns.h \
	opt_statistics.h \
	opt_loop_invert.h \
	opt_regsets.h \
	opt_progstruct.h \
	opt_context.h \
	opt_once_only.h \
	opt_deldead.h \
	opt_leaf_routine.h \
	opt_simplify_annul.h \
	opt_scheduling.h \
	set_instruction.h \
	pcc_types.h \
	symbol_expression.h

YACCSRC	=  parse.y
CSRCS	=       main.c  lex.c  lex_tables.c	 alloc.c	\
		actions_instr.c emit.c actions_expr.c		\
		actions_misc.c actions_util.c			\
		scratchvals.c  sym.c  errors.c  obj.c		\
		localsyms.c  globals.c  scan_symbols.c		\
		segments.c  relocs.c  stab.c			\
		virtual_root.c node.c operands.c check_nodes.c	\
		disassemble.c label.c opcode_ptrs.c		\
		set_instruction.c symbol_expression.c ident.c	\
								\
		optimize.c opt_branches.c opt_relabel.c		\
		opt_unique_label.c opt_dead_code.c		\
		opt_utilities.c	opt_fcn_prologue.c		\
		opt_statistics.c opt_loop_invert.c		\
		opt_regsets.c opt_deldead.c opt_patterns.c	\
		opt_scheduling.c opt_depend.c opt_sched_util.c	\
		opt_resequence.c opt_context.c opt_contents.c	\
		opt_progstruct.c opt_names.c			\
		opt_fcn_epilogue.c opt_once_only.c		\
		opt_leaf_routine.c opt_simplify_annul.c

COBJS	=	$(CSRCS:.c=.o)

SRCS = y.tab.c ${CSRCS}
OBJS = y.tab.o ${COBJS}

# ------------------  the main "make" target  -----------------------

as:	${OBJS} version_file.o
	${CC} ${LDFLAGS} -o as ${OBJS} version_file.o

objs:	${OBJS}

# "version_file.o" depends on ${OBJS} because we want the version file re-made
# any time any other object file was updated.
version_file.o: ${OBJS}
	-rm -f version_file.c ;	\
	echo "#ifdef DEBUG"				 >version_file.c ; \
	echo  "char *asm_make_date = \" [built `date +'%a %y/%m/%d %H:%M'`]\";" >>version_file.c ; \
	echo  "#endif /*DEBUG*/"			>>version_file.c ; \
	echo  "char *asm_version   = \"${VERSION}\";"	>>version_file.c ; \
	${CC}  ${CFLAGS} version_file.c

echo_objs:
	@echo ${OBJS}

# -----------  individual object files (Alphabetical order) -------------
${COBJS}:
	${CC} ${CFLAGS} $*.c

# The following rule is here to avoid re-making everything which depends on
# y.tab.h every time y.tab.c is re-made, if y.tab.h didn't actually change.
y.tab.c:	parse.y
	-mv -f y.tab.h y.tab.hOLD
	$(YACC) -d ${YFLAGS} parse.y
	-set -x;						\
		if diff y.tab.h y.tab.hOLD >/dev/null 2>&1 ;	\
		then	mv y.tab.hOLD y.tab.h ;			\
		else	rm -f y.tab.hOLD ;			\
		fi ;						\
		set -

y.tab.h:	y.tab.c

y.tab.o:	y.tab.c ${STRUCTS} ${ERRORS} ${ACTIONS} ${GLOBALS}	\
		${NODE} ${REGISTERS}
	${CC}  ${CFLAGS} y.tab.c

lint:	${SRCS}
	-${LINT} -abhx ${CPPFLAGS} ${SRCS}

tags:	${YACCSRC} ${CSRCS}
	-ctags ${YACCSRC} ${CSRCS}

clean:
	-rm -f ${OBJS} y.tab.c y.tab.h  yacc.*  y.output version_file.c

# Assembling-locore.s test of AS.  Assembling locore.s has often caused bugs
# to show up, especially the problem with the default ass'y mode being
# ASSY_MODE_QUICK vs. ASSY_MODE_BUILD_LIST.
test_locore:  TEST_LOCORE  as
	/bin/sh ./TEST_LOCORE  -Q   # -Q is temporary!

# Preprocessor-arg-passing test of AS.  Makes sure that lots of args can be
# passed through AS to preprocessor correctly.
preprocessor_arg_test:	PREPROCESSOR_ARG_TEST as
	/bin/sh ./PREPROCESSOR_ARG_TEST

# Recursive-building test of AS/C2, including disassembly.  It also tests
# for interaction with IROPT, since -O2 is used.
# Have patience -- this will take quite a while to run!
recursive_test:	RECURSIVE_TEST as
	/bin/sh ./RECURSIVE_TEST -O2 -save

# Test that the assembler/optimizer's warning/error messages are functional.
test_messages:	TEST_MESSAGES as
	/bin/sh ./TEST_MESSAGES

test:	test_locore  preprocessor_arg_test  test_messages  recursive_test
	@echo "All tests pass."

install:	as
	install -s -m 555 as ${DESTDIR}/usr/bin/as

# -------------------------------------------------------------------
# following rules for debugging use
# -------------------------------------------------------------------

# program to read & display token-count files left by assembler when the
# assembler was compiled with "-DDEBUG" and run with the "-T" cmd-line flag.
aux_lex_tc:	aux_lex_tc.o
	${CC} -o aux_lex_tc ${LDFLAGS} aux_lex_tc.o

aux_lex_tc.o:	aux_lex_tc.c ${YTAB} ${GLOBALS}
	${CC}  ${CFLAGS} aux_lex_tc.c

# program to print the names, functional groups, and functional subgroups
# of all opcode mnemonics
list_func_grps:	list_func_grps.o lex_tables.o
	${CC} -o list_func_grps ${LDFLAGS} list_func_grps.o lex_tables.o

list_func_grps.o: list_func_grps.c ${STRUCTS} ${LEX_TABLES}
	${CC}  ${CFLAGS} list_func_grps.c

# program to print the sizes of all of the structures defined in structs.h
aux_sizes:	aux_sizes.c ${STRUCTS}
	${CC}  -o aux_sizes aux_sizes.c

sizes:	aux_sizes
	@aux_sizes `mach`

# echo all of the object file names
echo:
	@echo ${OBJS}

.DEFAULT:
	sccs get -G$@ $@

depend: ${SRCS} ${HDRS}
	rm -f makedep ;
	for i in ${SRCS}; do \
	    ( ${CPP} -M ${CPPFLAGS} $$i >> makedep ); \
	done
	echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep ; \
	echo '$$r makedep' >>eddep ; \
	echo 'w' >>eddep ;
	rm -f Makefile.bak ; \
	cp Makefile Makefile.bak
	#
	# The following must be kept together as one Shell command line,
	# so that the READONLY Shell variable will not be lost.
	#
	if [ -w Makefile ]	; \
	then	READONLY=false	; \
	else	READONLY=true	; \
		chmod u+w Makefile;\
	fi			; \
	ed - Makefile < eddep	; \
	echo '# DEPENDENCIES MUST END AT END OF FILE' >> Makefile ; \
	echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >> Makefile ; \
	echo '# see make depend above' >> Makefile ; \
	if $$READONLY ; then chmod a-w Makefile ; fi
	#
	rm -f eddep makedep

# DO NOT DELETE THIS LINE -- make depend uses it

y.tab.o: y.tab.c
y.tab.o: /usr/include/stdio.h
y.tab.o: ./structs.h
y.tab.o: ./errors.h
y.tab.o: ./actions.h
y.tab.o: ./globals.h
y.tab.o: ./registers.h
main.o: main.c
main.o: /usr/include/stdio.h
main.o: /usr/include/signal.h
main.o: /usr/include/vm/faultcode.h
main.o: /usr/include/sys/file.h
main.o: /usr/include/sys/fcntlcom.h
main.o: /usr/include/sys/exec.h
main.o: ./structs.h
main.o: ./bitmacros.h
main.o: ./errors.h
main.o: ./globals.h
main.o: ./lex.h
main.o: ./segments.h
main.o: ./alloc.h
main.o: ./node.h
main.o: ./emit.h
main.o: ./obj.h
main.o: ./check_nodes.h
main.o: ./disassemble.h
main.o: ./relocs.h
main.o: ./scan_symbols.h
main.o: ./version_file.h
main.o: ./optimize.h
lex.o: lex.c
lex.o: /usr/include/stdio.h
lex.o: /usr/include/sys/wait.h
lex.o: /usr/include/sys/file.h
lex.o: /usr/include/sys/fcntlcom.h
lex.o: ./structs.h
lex.o: ./sym.h
lex.o: ./y.tab.h
lex.o: ./bitmacros.h
lex.o: ./errors.h
lex.o: ./instructions.h
lex.o: ./globals.h
lex.o: ./lex.h
lex.o: ./alloc.h
lex.o: ./node.h
lex.o: ./scratchvals.h
lex.o: ./disassemble.h
lex.o: ./lex_tables.h
lex.o: ./localsyms.h
lex.o: ./opcode_ptrs.h
lex.o: ./registers.h
lex_tables.o: lex_tables.c
lex_tables.o: /usr/include/stdio.h
lex_tables.o: ./structs.h
lex_tables.o: ./y.tab.h
lex_tables.o: ./instructions.h
lex_tables.o: ./lex.h
lex_tables.o: ./functional_groups.h
alloc.o: alloc.c
alloc.o: /usr/include/stdio.h
alloc.o: ./structs.h
alloc.o: ./alloc.h
actions_instr.o: actions_instr.c
actions_instr.o: /usr/include/stdio.h
actions_instr.o: ./structs.h
actions_instr.o: ./sym.h
actions_instr.o: ./y.tab.h
actions_instr.o: ./bitmacros.h
actions_instr.o: ./errors.h
actions_instr.o: ./actions.h
actions_instr.o: ./instructions.h
actions_instr.o: ./globals.h
actions_instr.o: ./lex.h
actions_instr.o: ./build_instr.h
actions_instr.o: ./segments.h
actions_instr.o: ./alloc.h
actions_instr.o: ./node.h
actions_instr.o: ./scratchvals.h
actions_instr.o: ./obj.h
actions_instr.o: ./stab.h
actions_instr.o: ./opcode_ptrs.h
actions_instr.o: ./registers.h
actions_instr.o: ./set_instruction.h
emit.o: emit.c
emit.o: /usr/include/stdio.h
emit.o: ./structs.h
emit.o: ./sym.h
emit.o: ./y.tab.h
emit.o: ./bitmacros.h
emit.o: ./errors.h
emit.o: ./actions.h
emit.o: ./globals.h
emit.o: ./lex.h
emit.o: ./build_instr.h
emit.o: ./segments.h
emit.o: ./alloc.h
emit.o: ./node.h
emit.o: ./emit.h
emit.o: ./disassemble.h
emit.o: ./stab.h
emit.o: ./opcode_ptrs.h
emit.o: ./registers.h
emit.o: ./set_instruction.h
emit.o: ./ident.h
emit.o: ./optimize.h
emit.o: ./opt_utilities.h
actions_expr.o: actions_expr.c
actions_expr.o: /usr/include/stdio.h
actions_expr.o: ./structs.h
actions_expr.o: ./sym.h
actions_expr.o: ./y.tab.h
actions_expr.o: ./bitmacros.h
actions_expr.o: ./errors.h
actions_expr.o: ./actions.h
actions_expr.o: ./globals.h
actions_expr.o: ./build_instr.h
actions_expr.o: ./segments.h
actions_expr.o: ./alloc.h
actions_expr.o: ./node.h
actions_expr.o: ./scratchvals.h
actions_expr.o: ./symbol_expression.h
actions_misc.o: actions_misc.c
actions_misc.o: /usr/include/stdio.h
actions_misc.o: ./structs.h
actions_misc.o: ./sym.h
actions_misc.o: ./y.tab.h
actions_misc.o: ./bitmacros.h
actions_misc.o: ./errors.h
actions_misc.o: ./actions.h
actions_misc.o: ./instructions.h
actions_misc.o: ./globals.h
actions_misc.o: ./lex.h
actions_misc.o: ./build_instr.h
actions_misc.o: ./segments.h
actions_misc.o: ./alloc.h
actions_misc.o: ./node.h
actions_misc.o: ./scratchvals.h
actions_misc.o: ./operands.h
actions_misc.o: ./label.h
actions_misc.o: ./opcode_ptrs.h
actions_misc.o: ./registers.h
actions_misc.o: ./symbol_expression.h
actions_util.o: actions_util.c
actions_util.o: /usr/include/stdio.h
actions_util.o: ./structs.h
actions_util.o: ./y.tab.h
actions_util.o: ./bitmacros.h
actions_util.o: ./errors.h
actions_util.o: ./actions.h
actions_util.o: ./instructions.h
actions_util.o: ./globals.h
actions_util.o: ./lex.h
actions_util.o: ./build_instr.h
actions_util.o: ./segments.h
actions_util.o: ./obj.h
actions_util.o: ./registers.h
actions_util.o: ./opt_unique_label.h
scratchvals.o: scratchvals.c
scratchvals.o: /usr/include/stdio.h
scratchvals.o: ./structs.h
scratchvals.o: ./errors.h
scratchvals.o: ./alloc.h
sym.o: sym.c
sym.o: /usr/include/stdio.h
sym.o: ./structs.h
sym.o: ./sym.h
sym.o: ./alloc.h
sym.o: ./sym_fcns.h
sym.o: ./sym_fcns.h
errors.o: errors.c
errors.o: /usr/include/stdio.h
errors.o: ./structs.h
errors.o: ./globals.h
obj.o: obj.c
obj.o: /usr/include/stdio.h
obj.o: /usr/include/sun4/a.out.h
obj.o: /usr/include/sys/exec.h
obj.o: ./structs.h
obj.o: ./errors.h
obj.o: ./globals.h
obj.o: ./segments.h
obj.o: ./bitmacros.h
obj.o: ./sym.h
obj.o: ./stab.h
obj.o: ./relocs.h
localsyms.o: localsyms.c
globals.o: globals.c
globals.o: /usr/include/stdio.h
globals.o: ./structs.h
globals.o: ./globals.h
scan_symbols.o: scan_symbols.c
scan_symbols.o: /usr/include/stdio.h
scan_symbols.o: ./structs.h
scan_symbols.o: ./sym.h
scan_symbols.o: ./bitmacros.h
scan_symbols.o: ./errors.h
scan_symbols.o: ./globals.h
segments.o: segments.c
segments.o: /usr/include/stdio.h
segments.o: ./structs.h
segments.o: ./sym.h
segments.o: ./bitmacros.h
segments.o: ./errors.h
segments.o: ./actions.h
segments.o: ./globals.h
segments.o: ./segments.h
segments.o: ./alloc.h
segments.o: ./node.h
relocs.o: relocs.c
relocs.o: /usr/include/stdio.h
relocs.o: /usr/include/sun4/a.out.h
relocs.o: /usr/include/sys/exec.h
relocs.o: ./structs.h
relocs.o: ./sym.h
relocs.o: ./bitmacros.h
relocs.o: ./errors.h
relocs.o: ./build_instr.h
relocs.o: ./globals.h
relocs.o: ./segments.h
relocs.o: ./alloc.h
relocs.o: ./scan_symbols.h
relocs.o: ./registers.h
relocs.o: ./symbol_expression.h
stab.o: stab.c
stab.o: /usr/include/stdio.h
stab.o: /usr/include/stab.h
stab.o: /usr/include/sun4/a.out.h
stab.o: /usr/include/sys/exec.h
stab.o: ./structs.h
stab.o: ./errors.h
stab.o: ./globals.h
stab.o: ./segments.h
stab.o: ./node.h
stab.o: ./emit.h
stab.o: ./obj.h
virtual_root.o: virtual_root.c
virtual_root.o: /usr/include/errno.h
virtual_root.o: /usr/include/sys/errno.h
virtual_root.o: /usr/include/stdio.h
virtual_root.o: /usr/include/ctype.h
virtual_root.o: /usr/include/sys/param.h
virtual_root.o: /usr/include/machine/param.h
virtual_root.o: /usr/include/sys/signal.h
virtual_root.o: /usr/include/vm/faultcode.h
virtual_root.o: /usr/include/sys/types.h
virtual_root.o: /usr/include/sys/sysmacros.h
virtual_root.o: /usr/include/sys/file.h
virtual_root.o: /usr/include/sys/fcntlcom.h
virtual_root.o: /usr/include/sys/stat.h
node.o: node.c
node.o: /usr/include/stdio.h
node.o: ./structs.h
node.o: ./actions.h
node.o: ./globals.h
node.o: ./lex.h
node.o: ./alloc.h
node.o: ./node.h
node.o: ./disassemble.h
node.o: ./label.h
node.o: ./opcode_ptrs.h
operands.o: operands.c
operands.o: /usr/include/stdio.h
operands.o: ./structs.h
operands.o: ./errors.h
operands.o: ./globals.h
operands.o: ./alloc.h
check_nodes.o: check_nodes.c
check_nodes.o: /usr/include/stdio.h
check_nodes.o: /usr/include/sun4/asm_linkage.h
check_nodes.o: ./structs.h
check_nodes.o: ./errors.h
check_nodes.o: ./globals.h
check_nodes.o: ./node.h
check_nodes.o: ./opcode_ptrs.h
check_nodes.o: ./opt_branches.h
check_nodes.o: ./opt_utilities.h
check_nodes.o: ./registers.h
check_nodes.o: ./sym.h
check_nodes.o: ./optimize.h
check_nodes.o: ./instructions.h
disassemble.o: disassemble.c
disassemble.o: /usr/include/stdio.h
disassemble.o: ./structs.h
disassemble.o: ./sym.h
disassemble.o: ./y.tab.h
disassemble.o: ./bitmacros.h
disassemble.o: ./errors.h
disassemble.o: ./actions.h
disassemble.o: ./globals.h
disassemble.o: ./lex.h
disassemble.o: ./build_instr.h
disassemble.o: ./segments.h
disassemble.o: ./alloc.h
disassemble.o: ./node.h
disassemble.o: ./disassemble.h
disassemble.o: ./registers.h
disassemble.o: ./symbol_expression.h
label.o: label.c
label.o: /usr/include/stdio.h
label.o: ./structs.h
label.o: ./bitmacros.h
label.o: ./errors.h
label.o: ./globals.h
label.o: ./node.h
opcode_ptrs.o: opcode_ptrs.c
opcode_ptrs.o: /usr/include/stdio.h
opcode_ptrs.o: ./structs.h
opcode_ptrs.o: ./sym.h
opcode_ptrs.o: ./opcode_ptrs.h
set_instruction.o: set_instruction.c
set_instruction.o: /usr/include/stdio.h
set_instruction.o: ./structs.h
set_instruction.o: ./set_instruction.h
symbol_expression.o: symbol_expression.c
symbol_expression.o: /usr/include/stdio.h
symbol_expression.o: ./structs.h
symbol_expression.o: ./sym.h
symbol_expression.o: ./bitmacros.h
symbol_expression.o: ./errors.h
symbol_expression.o: ./globals.h
symbol_expression.o: ./alloc.h
symbol_expression.o: ./node.h
symbol_expression.o: ./opcode_ptrs.h
symbol_expression.o: ./symbol_expression.h
ident.o: ident.c
ident.o: /usr/include/stdio.h
ident.o: /usr/include/sun4/a.out.h
ident.o: /usr/include/sys/exec.h
ident.o: ./structs.h
ident.o: ./errors.h
ident.o: ./globals.h
optimize.o: optimize.c
optimize.o: /usr/include/stdio.h
optimize.o: ./structs.h
optimize.o: ./globals.h
optimize.o: ./node.h
optimize.o: ./check_nodes.h
optimize.o: ./opcode_ptrs.h
optimize.o: ./sym.h
optimize.o: ./registers.h
optimize.o: ./optimize.h
optimize.o: ./opt_branches.h
optimize.o: ./opt_relabel.h
optimize.o: ./opt_dead_code.h
optimize.o: ./opt_utilities.h
optimize.o: ./opt_statistics.h
optimize.o: ./opt_loop_invert.h
optimize.o: ./opt_once_only.h
optimize.o: ./opt_leaf_routine.h
opt_branches.o: opt_branches.c
opt_branches.o: /usr/include/stdio.h
opt_branches.o: ./structs.h
opt_branches.o: ./actions.h
opt_branches.o: ./globals.h
opt_branches.o: ./node.h
opt_branches.o: ./label.h
opt_branches.o: ./opcode_ptrs.h
opt_branches.o: ./optimize.h
opt_branches.o: ./opt_unique_label.h
opt_branches.o: ./opt_utilities.h
opt_branches.o: ./opt_statistics.h
opt_branches.o: ./opt_loop_invert.h
opt_branches.o: ./opt_simplify_annul.h
opt_relabel.o: opt_relabel.c
opt_relabel.o: /usr/include/stdio.h
opt_relabel.o: ./structs.h
opt_relabel.o: ./bitmacros.h
opt_relabel.o: ./node.h
opt_relabel.o: ./label.h
opt_relabel.o: ./optimize.h
opt_relabel.o: ./opt_statistics.h
opt_unique_label.o: opt_unique_label.c
opt_unique_label.o: /usr/include/stdio.h
opt_unique_label.o: ./structs.h
opt_unique_label.o: ./actions.h
opt_unique_label.o: ./opcode_ptrs.h
opt_unique_label.o: ./opt_utilities.h
opt_dead_code.o: opt_dead_code.c
opt_dead_code.o: /usr/include/stdio.h
opt_dead_code.o: ./structs.h
opt_dead_code.o: ./node.h
opt_dead_code.o: ./optimize.h
opt_dead_code.o: ./opt_statistics.h
opt_utilities.o: opt_utilities.c
opt_utilities.o: /usr/include/stdio.h
opt_utilities.o: ./structs.h
opt_utilities.o: ./sym.h
opt_utilities.o: ./bitmacros.h
opt_utilities.o: ./node.h
opt_utilities.o: ./opcode_ptrs.h
opt_utilities.o: ./optimize.h
opt_utilities.o: ./opt_utilities.h
opt_utilities.o: ./registers.h
opt_utilities.o: ./opt_regsets.h
opt_fcn_prologue.o: opt_fcn_prologue.c
opt_fcn_prologue.o: /usr/include/stdio.h
opt_fcn_prologue.o: ./structs.h
opt_fcn_prologue.o: ./bitmacros.h
opt_fcn_prologue.o: ./registers.h
opt_fcn_prologue.o: ./optimize.h
opt_fcn_prologue.o: ./opt_utilities.h
opt_fcn_prologue.o: ./opt_regsets.h
opt_fcn_prologue.o: ./opt_statistics.h
opt_statistics.o: opt_statistics.c
opt_statistics.o: /usr/include/stdio.h
opt_statistics.o: ./structs.h
opt_statistics.o: ./globals.h
opt_statistics.o: ./opt_utilities.h
opt_statistics.o: ./opt_statistics.h
opt_loop_invert.o: opt_loop_invert.c
opt_loop_invert.o: /usr/include/stdio.h
opt_loop_invert.o: ./structs.h
opt_loop_invert.o: ./globals.h
opt_loop_invert.o: ./node.h
opt_loop_invert.o: ./label.h
opt_loop_invert.o: ./opcode_ptrs.h
opt_loop_invert.o: ./optimize.h
opt_loop_invert.o: ./opt_unique_label.h
opt_loop_invert.o: ./opt_utilities.h
opt_loop_invert.o: ./opt_statistics.h
opt_regsets.o: opt_regsets.c
opt_regsets.o: /usr/include/stdio.h
opt_regsets.o: ./structs.h
opt_regsets.o: ./alloc.h
opt_regsets.o: ./bitmacros.h
opt_regsets.o: ./registers.h
opt_regsets.o: ./optimize.h
opt_regsets.o: ./opt_statistics.h
opt_regsets.o: ./opt_utilities.h
opt_regsets.o: ./opt_regsets.h
opt_regsets.o: ./pcc_types.h
opt_deldead.o: opt_deldead.c
opt_deldead.o: /usr/include/stdio.h
opt_deldead.o: ./registers.h
opt_deldead.o: ./structs.h
opt_deldead.o: ./bitmacros.h
opt_deldead.o: ./optimize.h
opt_deldead.o: ./sym.h
opt_deldead.o: ./opt_utilities.h
opt_deldead.o: ./opt_statistics.h
opt_deldead.o: ./opt_regsets.h
opt_deldead.o: ./opt_deldead.h
opt_deldead.o: ./opcode_ptrs.h
opt_patterns.o: opt_patterns.c
opt_patterns.o: /usr/include/stdio.h
opt_patterns.o: ./structs.h
opt_patterns.o: ./bitmacros.h
opt_patterns.o: ./registers.h
opt_patterns.o: ./optimize.h
opt_patterns.o: ./opt_utilities.h
opt_patterns.o: ./opt_regsets.h
opt_patterns.o: ./opt_regsets.h
opt_patterns.o: ./opt_statistics.h
opt_patterns.o: ./sym.h
opt_patterns.o: ./opcode_ptrs.h
opt_scheduling.o: opt_scheduling.c
opt_scheduling.o: /usr/include/stdio.h
opt_scheduling.o: ./structs.h
opt_scheduling.o: ./opt_scheduling.h
opt_scheduling.o: ./opt_utilities.h
opt_scheduling.o: ./opcode_ptrs.h
opt_scheduling.o: ./registers.h
opt_scheduling.o: ./optimize.h
opt_depend.o: opt_depend.c
opt_depend.o: /usr/include/stdio.h
opt_depend.o: ./structs.h
opt_depend.o: ./opt_scheduling.h
opt_depend.o: ./registers.h
opt_depend.o: ./globals.h
opt_sched_util.o: opt_sched_util.c
opt_sched_util.o: /usr/include/stdio.h
opt_sched_util.o: ./structs.h
opt_sched_util.o: ./globals.h
opt_sched_util.o: ./alloc.h
opt_sched_util.o: ./registers.h
opt_sched_util.o: ./opt_utilities.h
opt_sched_util.o: ./opt_scheduling.h
opt_resequence.o: opt_resequence.c
opt_resequence.o: /usr/include/stdio.h
opt_resequence.o: ./structs.h
opt_resequence.o: ./opt_scheduling.h
opt_resequence.o: ./registers.h
opt_resequence.o: ./optimize.h
opt_resequence.o: ./opt_utilities.h
opt_resequence.o: ./globals.h
opt_context.o: opt_context.c
opt_context.o: /usr/include/stdio.h
opt_context.o: ./structs.h
opt_context.o: ./registers.h
opt_context.o: ./bitmacros.h
opt_context.o: ./node.h
opt_context.o: ./optimize.h
opt_context.o: ./opcode_ptrs.h
opt_context.o: ./node.h
opt_context.o: ./opt_utilities.h
opt_context.o: ./opt_statistics.h
opt_context.o: ./opt_progstruct.h
opt_context.o: ./opt_regsets.h
opt_context.o: ./opt_context.h
opt_context.o: ./alloc.h
opt_contents.o: opt_contents.c
opt_contents.o: /usr/include/stdio.h
opt_contents.o: ./structs.h
opt_contents.o: ./registers.h
opt_contents.o: ./bitmacros.h
opt_contents.o: ./node.h
opt_contents.o: ./optimize.h
opt_contents.o: ./sym.h
opt_contents.o: ./opcode_ptrs.h
opt_contents.o: ./node.h
opt_contents.o: ./globals.h
opt_contents.o: ./opt_utilities.h
opt_contents.o: ./opt_statistics.h
opt_contents.o: ./opt_progstruct.h
opt_contents.o: ./opt_regsets.h
opt_contents.o: ./opt_context.h
opt_progstruct.o: opt_progstruct.c
opt_progstruct.o: /usr/include/stdio.h
opt_progstruct.o: ./structs.h
opt_progstruct.o: ./bitmacros.h
opt_progstruct.o: ./alloc.h
opt_progstruct.o: ./optimize.h
opt_progstruct.o: ./opt_utilities.h
opt_progstruct.o: ./opt_statistics.h
opt_progstruct.o: ./opt_progstruct.h
opt_names.o: opt_names.c
opt_names.o: /usr/include/stdio.h
opt_names.o: ./structs.h
opt_names.o: ./globals.h
opt_names.o: ./registers.h
opt_names.o: ./bitmacros.h
opt_names.o: ./node.h
opt_names.o: ./optimize.h
opt_names.o: ./opcode_ptrs.h
opt_names.o: ./node.h
opt_names.o: ./opt_utilities.h
opt_names.o: ./opt_statistics.h
opt_names.o: ./opt_progstruct.h
opt_names.o: ./opt_regsets.h
opt_names.o: ./opt_context.h
opt_names.o: ./opt_deldead.h
opt_names.o: ./set_instruction.h
opt_names.o: /usr/include/alloca.h
opt_fcn_epilogue.o: opt_fcn_epilogue.c
opt_fcn_epilogue.o: /usr/include/stdio.h
opt_fcn_epilogue.o: ./structs.h
opt_fcn_epilogue.o: ./bitmacros.h
opt_fcn_epilogue.o: ./registers.h
opt_fcn_epilogue.o: ./optimize.h
opt_fcn_epilogue.o: ./opt_utilities.h
opt_fcn_epilogue.o: ./opt_regsets.h
opt_fcn_epilogue.o: ./opt_statistics.h
opt_once_only.o: opt_once_only.c
opt_once_only.o: /usr/include/stdio.h
opt_once_only.o: ./structs.h
opt_once_only.o: ./node.h
opt_once_only.o: ./optimize.h
opt_once_only.o: ./opcode_ptrs.h
opt_once_only.o: ./sym.h
opt_once_only.o: ./opt_fcn_epilogue.h
opt_once_only.o: ./opt_fcn_prologue.h
opt_once_only.o: ./opt_patterns.h
opt_once_only.o: ./opt_utilities.h
opt_once_only.o: ./opt_regsets.h
opt_leaf_routine.o: opt_leaf_routine.c
opt_leaf_routine.o: /usr/include/stdio.h
opt_leaf_routine.o: ./structs.h
opt_leaf_routine.o: ./registers.h
opt_leaf_routine.o: ./opcode_ptrs.h
opt_leaf_routine.o: ./optimize.h
opt_leaf_routine.o: ./node.h
opt_leaf_routine.o: ./opt_utilities.h
opt_leaf_routine.o: ./opt_regsets.h
opt_leaf_routine.o: ./opt_statistics.h
opt_simplify_annul.o: opt_simplify_annul.c
opt_simplify_annul.o: /usr/include/stdio.h
opt_simplify_annul.o: ./structs.h
opt_simplify_annul.o: ./globals.h
opt_simplify_annul.o: ./node.h
opt_simplify_annul.o: ./optimize.h
opt_simplify_annul.o: ./opt_utilities.h
opt_simplify_annul.o: ./opt_statistics.h
# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
