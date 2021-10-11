# @(#)device.mk 1.1 94/10/31 SMI
# pixrect library common makefile template
# (included in each sub-makefile)

# targets
LIB	      = $(TARGET_DIR)/libpixrect.a
LIB_g	      = $(TARGET_DIR)/libpixrect_g.a
LIB_p	      = $(TARGET_DIR)/libpixrect_p.a
LIB_pg	      = $(TARGET_DIR)/libpixrect_pg.a
LIB_so	      = $(TARGET_DIR)/libpixrect.so
LIB_sa	      = $(TARGET_DIR)/libpixrect.sa
LIB_sh =	$(LIB_sa) $(LIB_so)

CSRC =		$(CDATA) $(CNORM) $(CNOOPT) $(CNORO)

OBJ1 =		$(CSRC:.c=.o) $(MOREOBJ) $(ASRC:.S=.o)
OBJ2 =		$(CPROM:.c=.o) $(CSTAND:.c=.o)
OBJ =		$(OBJ1) $(OBJ2)
		
ODATA =		$(CDATA:.c=.o)
ONOOPT =	$(CNOOPT:.c=.o)
ONORO =		$(CNORO:.c=.o)
OSHSA1 =	$(CSHSA1:.c=.o)
OSHSA2 =	$(CSHSA2:.c=.o)
OSHSA =		$(OSHSA1) $(OSHSA2)

$(LIB_sh) := 	OBJ1 += $(OSHSA2)

SHOBJ =		$(OBJ1:%.o=$(SHOBJ_DIR)/%.o)
SHDATA =	$(ODATA:%.o=$(SHOBJ_DIR)/%.o)
SHNOOPT =	$(ONOOPT:%.o=$(SHOBJ_DIR)/%.o)
SHONORO = 	$(ONORO:%.o=$(SHOBJ_DIR)/%.o)

# note: CSHNOROs also appear in CSRC; used only to kill -R
SHNORO =	$(CSHNORO:%.c=$(SHOBJ_DIR)/%.o)

# misc commands/flags
CPP =		/lib/cpp -E
ARFLAGS	=	ruc

# compiler flags
INCLUDE = 	-I.. -I../pixrect -I../../../sys -I../../../include -I../../libsunwindow
FLAGS =
FLOAT-sun2 =	-fsoft
FLOAT-sun3 =	-fsoft
FLOAT =		$(FLOAT$(TARGET_ARCH))
OPTIM =		-O
PIC =		-pic -DPIC
READONLY =	-R
SHLIB =		$(PIC) -DSHLIB
SHLIB_SA =	-DSHLIB_SA

$(LIB_g) := 	READONLY=
$(LIB_g) := 	OPTIM=

$(ONORO) \
$(LIB)($(ONORO)) \
$(LIB_p)($(ONORO)) \
$(LIB_pg)($(ONORO)) \
$(SHNORO) \
$(SHONORO) \
	:=	READONLY=

CPPFLAGS =	$(INCLUDE) $(PIXINC)

CFBASE =	$(FLAGS) $(FLOAT)
CFDATA =	$(CFBASE)
CFNOOPT =	$(CFBASE) $(READONLY)
CFNORM =	$(CFBASE) $(OPTIM) $(READONLY)

CFLAGS =	$(CFNORM)

$(ODATA) \
$(LIB)($(ODATA)) \
$(LIB_p)($(ODATA)) \
$(LIB_pg)($(ODATA)) \
$(SHDATA) \
	:= CFLAGS = $(CFDATA)

$(ONOOPT) \
$(LIB)($(ONOOPT)) \
$(LIB_p)($(ONOOPT)) \
$(LIB_pg)($(ONOOPT)) \
$(SHNOOPT) \
	:= CFLAGS = $(CFNOOPT)

$(LIB_sh) := ASFLAGS += $(SHLIB)

$(LIB_sh) \
$(SHDATA) \
$(SHNOOPT) \
	:= CFLAGS += $(SHLIB)

$(LIB_sa) := ASFLAGS += $(SHLIB_SA)
$(LIB_sa) := CFLAGS += $(SHLIB_SA)

# rules
.SUFFIXES: .IL .IL~

$(TARGET_DIR)/%.il:	%.IL
	$(CPP) -I$(PIXINC) $< > $@

$(TARGET_DIR)/%.o $(SHOBJ_DIR)/%.o: %.c
	$(COMPILE.c) -o $@ $<

$(TARGET_DIR)/%.o $(SHOBJ_DIR)/%.o: %.S
	$(COMPILE.S) -o $@ $<

$(LIB_p)(%.o): %.c
	$(COMPILE.c) -p -o $% $<
	$(AR) $(ARFLAGS) $@ $%
	$(RM) $%
$(LIB_pg)(%.o): %.c
	$(COMPILE.c) -pg -o $% $<
	$(AR) $(ARFLAGS) $@ $%
	$(RM) $%
$(LIB_g)(%.o): %.c
	$(COMPILE.c) -g -o $% $<
	$(AR) $(ARFLAGS) $@ $%
	$(RM) $%
$(LIB_g)(%.o) $(LIB_p)(%.o) $(LIB_pg)(%.o): %.S
	$(COMPILE.c) -o $% $<
	$(AR) $(ARFLAGS) $@ $%
	$(RM) $%

.PRECIOUS: $(LIB) $(LIB_p) $(LIB_g) $(LIB_pg)

default:
	@echo "no target specified" ; exit 1

clean:
	$(RM) $(OBJ)

ALL_SRCS = $(CSRC) $(ASRC) $(MORESRC) $(HDRS)

populate: $(ALL_SRCS) $(MAKEFILE)

sid:	$(ALL_SRCS) $(MAKEFILE)
	$(SID_WHAT) $(ALL_SRCS) $(MAKEFILE) >> $(SID_FILE)

tags:	$(ALL_SRC)
	$(TAGS_CMD) -f $(TAGS_FILE) $(ALL_SRCS:%=$(DEVDIR)/%)
	$(RM) `basename $(TAGS_FILE)`; \
		ln -s $(TAGS_FILE) `basename $(TAGS_FILE)`


# indirection to avoid `foo' is up to date messages
libpixrect%: $(TARGET_DIR)/libpixrect%

$(LIB) \
$(LIB_g) \
$(LIB_p) \
$(LIB_pg): $$@($$(OBJ)) $(MORESRC)

$(LIB_sa): $$@($(OSHSA)) $(MORESRC)

$(LIB_so): $$(SHOBJ) $(MORESRC)

FRC:
