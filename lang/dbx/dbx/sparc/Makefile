#
# @(#)Makefile 1.1 (Sun) 94/10/31

DESTDIR =
DEST	= ${DESTDIR}/usr/ucb/dbx

OBJ = \
	alias_expand.o \
	asm.o \
	assign.o \
	c.o \
	cerror.o \
	check.o \
	commands.o \
	coredump.o \
	dbx_sstep.o \
	dbxenv.o \
	dbxmain.o \
	debug.o \
	disasm.o \
	display.o \
	eval.o \
	events.o \
	expand_name.o \
	fortran.o \
	getshlib.o \
	help.o \
	kernel.o \
	keywords.o \
	languages.o \
	library.o \
	lists.o \
	machine.o \
	mappings.o \
	modula.o \
	names.o \
	object.o \
	operators.o \
	pascal.o \
	pipeout.o \
	printsym.o \
	process.o \
	runtime.o \
	sas_tables.o \
	scanner.o \
	source.o \
	srcs_cmd.o \
	symbols.o \
	tree.o \
	ttyfix.o \
	types.o

HDR_MARKER = \
    asm.hdr \
    c.hdr \
    check.hdr \
    coredump.hdr \
    dbxenv.hdr \
    dbxmain.hdr \
    debug.hdr \
    display.hdr \
    eval.hdr \
    events.hdr \
    fortran.hdr \
    getshlib.hdr \
    kernel.hdr \
    keywords.hdr \
    languages.hdr \
    lists.hdr \
    machine.hdr \
    mappings.hdr \
    modula.hdr \
    names.hdr \
    object.hdr \
    operators.hdr \
    pascal.hdr \
    pipeout.hdr \
    printsym.hdr \
    process.hdr \
    runtime.hdr \
    scanner.hdr \
    source.hdr \
    srcs_cmd.hdr \
    symbols.hdr \
    tree.hdr \
    ttyfix.hdr \
    types.hdr

.INIT: sas_includes.h

ASFLAGS = -Qoption as -P

include ../common/common.mk
