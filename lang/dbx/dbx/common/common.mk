#@(#)common.mk 1.1 (Sun) 94/10/31
#
#
# Makefile for debugger "dbx"
#
# The file "defs.h" is included by all.
#
# The object files are kept in a subdirectory:  "./obj" for nondebugging
# object files; "./o-g" for those with debugging flags set.
#
# N.B.:
#    Dbx contains a version of cerror which automatically catches
# certain errors such as out of memory, I/O error.  This fails
# utterly on SPARC, and I don't trust it on any 4.0 system.
#
#

.KEEP_STATE:
.SUFFIXES:

COMMON=../common


#### #### #### #### #### #### #### #### #### #### #### #### #### #### ####
#     The main target name --
#

#
# C compiler (& preprocessor & linker) flags
#
YFLAGS = -d -Nm30000

CPPFLAGS2 = -I${COMMON} -I$(OBJ_DIR) $(FPU) -D$(TARGET_ARCH:-%=%) $(CPP_ARCH)
CFLAGS2 = $(CB_FLAG)

dbx makedefs mkdate := CFLAGS = -O $(CFLAGS2)
dbx makedefs mkdate := CPPFLAGS = $(CPPFLAGS2)
dbx makedefs mkdate := OBJ_DIR = obj

gdbx gmakedefs gmkdate := CFLAGS = -g $(CFLAGS2)
gdbx gmakedefs gmkdate := CPPFLAGS = -DDEBUGGING $(CPPFLAGS2)
gdbx gmakedefs gmkdate := OBJ_DIR = o-g
gdbx gmakedefs gmkdate := BEEPER = @echo ''

all: 	dbx

DBX_OBJ = ${HDR_MARKER:%=$(OBJ_DIR)/%} $(OBJ:%=$(OBJ_DIR)/%)
dbx gdbx: $$(OBJ_DIR) makedefs $$(DBX_OBJ)
	@echo Linking $@
	${LINK.c} -o $@ $(OBJ:%=$(OBJ_DIR)/%)
	$(BEEPER)

MAKEDEFSOBJ = library.o cerror.o makedefs.o
XMAKEDEFSOBJ = $(MAKEDEFSOBJ:%=$(OBJ_DIR)/%)
makedefs gmakedefs:  $$(OBJ_DIR) $$(XMAKEDEFSOBJ)
	$(LINK.c) -o $@ $(MAKEDEFSOBJ:%=$(OBJ_DIR)/%)

MKDATEOBJ = mkdate.o
XMKDATEOBJ = $(MKDATEOBJ:%=$(OBJ_DIR)/%)
mkdate gmkdate: $$(OBJ_DIR) $$(XMKDATEOBJ)
	$(LINK.c) -o $@ $(MKDATEOBJ:%=$(OBJ_DIR)/%)

obj/date.c o-g/date.c:	mkdate
	${RM} $@
	./mkdate > $@

# Make the object-file directories
obj o-g:
	if [ ! -d $@ ] ; then mkdir $@ ; fi

#### #### #### #### #### #### #### #### #### #### #### #### #### #### ####
# Pattern-matching rules
#
# These rules allow both the source and object files to live in another
# directory.  The rule here provides (for each object file) the _action_
# command(s) to compile the C source, but the real dependencies are
# created by "make" and kept in ".make.state".
#

# ### ######### ###### ### ###### #####
# Non-debugging header and object files
#
obj/%.hdr : %.c
	cd obj ; ../makedefs ../$< $*.h

obj/%.hdr : ${COMMON}/%.c
	cd obj ; ../makedefs ../$< $*.h

obj/%.o : %.s
	$(COMPILE.s) $< -o $@

obj/%.o : %.c
	$(COMPILE.c) $< -o $@

obj/%.o : ${COMMON}/%.c
	$(COMPILE.c) $< -o $@

obj/%.o : obj/%.c
	$(COMPILE.c) $< -o $@

# ######### #### ###### ### ###### #####
# Debugging (-g) header and object files
o-g/%.hdr : %.c
	cd o-g ; ../makedefs ../$< $*.h

o-g/%.hdr : ${COMMON}/%.c
	cd o-g ; ../makedefs ../$< $*.h

o-g/%.o : %.s
	$(COMPILE.s) $< -o $@

o-g/%.o : %.c
	$(COMPILE.c) $< -o $@
	
o-g/%.o : ${COMMON}/%.c
	$(COMPILE.c) $< -o $@

o-g/%.o : o-g/%.c
	$(COMPILE.c) $< -o $@

#
# These header files are not derived from the C files,
# but should be gotten from SCCS when they are out of date.
#
HDRS = \
	${COMMON}/defs.h \
	${COMMON}/library.h \
	${COMMON}/expand_name.h \
	obj/commands.h \
	o-g/commands.h
.INIT: obj o-g $(HDRS)

#
# the tags stuff WILL get you to files in ../common if you're here
#
tags:
	@ echo "making tags ..."
	@ ctags -w *.[ch] $(COMMON)/*.[chy] obj/*.[ch] o-g/*.[ch]

#
# Don't worry about the removal of header files, they're created from
# the source files.
#
clean:
	rm -fr obj o-g
	${RM} y.tab.c y.tab.h \
		dbx gdbx makedefs gmakedefs mkdate gmkdate

obj/commands.h o-g/commands.h: y.tab.h obj o-g
	-cmp -s y.tab.h $@ || cp y.tab.h $@

obj/commands.c + o-g/commands.c + y.tab.h: ${COMMON}/commands.y
	@echo Expect 14 shift/reduce conflicts
	${YACC.y} ${COMMON}/commands.y
	mv y.tab.c obj/commands.c
	cp obj/commands.c o-g/commands.c

obj/commands.o o-g/commands.o: $$(@:.o=.c)
	$(COMPILE.c) -o $@ $(@:.o=.c)



#
# Install it for fortran
#
#386 specific directories
IDEST386=cluster/devel/base_devel/usr.lang
DEST386=usr/$(IDEST386)
LINKDIR=usr/lang

install_f77:	dbx
	if test `mach` = i386 ; then \
	        install -m 555 -s dbx $(DESTDIR)/$(DEST386)/f77/dbx ; \
	        /bin/rm -f $(DESTDIR)/$(LINKDIR)/f77/dbx ; \
	        ln -s ../../$(IDEST386)/f77/dbx $(DESTDIR)/$(LINKDIR)/f77/dbx ; \
	else \
		install -s dbx $(DESTDIR)/usr/lang/f77/dbx ; \
	fi
