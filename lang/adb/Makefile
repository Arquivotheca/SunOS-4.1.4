#
# @(#)Makefile 1.1 94/10/31 SMI
#

DESTDIR=
MAKECOMMON= common/MakeCommon
SCCS_MAKECOMMON= common/SCCS/s.MakeCommon

all: adb kadb

adb: ${MAKECOMMON} FRC
	cd ${CPU}; $(MAKE) ${MFLAGS} CC="${CC}" AS="${AS}"

kadb: ${MAKECOMMON} FRC
	cd ${CPU}/kadb; $(MAKE) ${MFLAGS} CC="${CC}" AS="${AS}"

install: FRC
	cd ${CPU}; $(MAKE) ${MFLAGS} DESTDIR=${DESTDIR} install

clean: ${MAKECOMMON}
	cd ${CPU}; $(MAKE) ${MFLAGS} clean
	cd ${CPU}/kadb; $(MAKE) ${MFLAGS} clean

.DEFAULT:
	sccs get -G$@ $@

FRC:
