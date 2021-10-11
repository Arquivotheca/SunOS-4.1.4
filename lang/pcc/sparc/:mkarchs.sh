#!/bin/sh
#
# @(#):mkarchs.sh	1.1 (Sun) 10/31/94
# shell script to make unopt.{ccom,cg}.a from objects saved
# during a step of a bootstrap operation
#
set -v
rm -f ccom.a cg.a
ar cru ccom.a \
    cgram.o p1.o comm1.o xdefs.o scan.o pftn.o trees.o optim.o \
    reader.o match.o yyerror.o  symtab.o code.o local.o regvars.o \
    local2.o order.o  opmatch.o allo1.o allo2.o bound.o table.o \
    float2.o myflags2.o stab.o optim2.o util2.o su.o  gencall.o \
    struct.o  genswitch.o assign_field.o malign.o picode.o \
    messages.o \
    ir_wf.o ir_alloc.o ir_misc.o ir_debug.o ir_map.o \
    ir_map_expr.o
ranlib ccom.a
ar cru cg.a \
    cgrdr/cg_main.o cgrdr/cgflags.o cgrdr/debug.o cgrdr/do_ir_archive.o \
    cgrdr/intr_map.o cgrdr/misc.o cgrdr/pcc.o cgrdr/pccfmt.o \
    cgrdr/read_ir.o cgrdr/rewrite.o \
    freader.o fallo2.o fopmatch.o fmatch.o ftable.o \
    forder.o  fbound.o flocal2.o fcomm2.o ffloat2.o fmyflags2.o \
    foptim2.o  futil2.o fsu.o fgencall.o fstruct.o \
    fgenswitch.o fassign_field.o fmalign.o fpicode.o 
ranlib cg.a
