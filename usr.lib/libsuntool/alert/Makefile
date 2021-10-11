 # 
# @(#)Makefile 1.1 94/10/31
#
WHOAMI=		alert
#HDRSPUBLIC=	alert.h
#HDRSPRIVATE=	alert_impl.h
CFILES=		alert.c
CFILESLIB= 	alert.c
SPCFILESLIB=	alert.c
OBJS=	$(CFILESLIB:%.c=$(VARIANT)/%.o)

TAGFILESALL=	$(FULLPATH)/alert.c \
		$(FULLPATH)/alert.h $(FULLPATH)/alert_impl.h

# include the common stuff from ../Makefile.master
include ../Makefile.arch
include ../Makefile.master

.KEEP_STATE:

$(SPCFILESLIB:%.c=obj/%.o) \
		$(SPCFILESLIB:%.c=obj_p/%.o) \
		$(SPCFILESLIB:%.c=shared/%.o)  :=  ROI_DATA =

xall: $$(LIBS)
$(LIBS) : $$(VARIANT) $(HDRSPUBLIC) $(HDRSPRIVATE) $$(OBJS)

clean: master.clean
