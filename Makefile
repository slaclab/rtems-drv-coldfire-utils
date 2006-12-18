#
#  $Id$
#
# Templates/Makefile.lib
#       Template library Makefile
#


LIBNAME=libcoldfUtils.a        # xxx- your library names goes here
LIB=${ARCH}/${LIBNAME}
PGMS=${ARCH}/coldfUtils.obj

# C and C++ source names, if any, go here -- minus the .c or .cc
C_PIECES=eport drv5282DMA cselect fecmii drv5282QSPI
C_PIECES+=coldfUtils.modini
C_FILES=$(C_PIECES:%=%.c)
C_O_FILES=$(C_PIECES:%=${ARCH}/%.o)

CC_PIECES=
CC_FILES=$(CC_PIECES:%=%.cc)
CC_O_FILES=$(CC_PIECES:%=${ARCH}/%.o)

H_FILES=coldfUtils.h

# Assembly source names, if any, go here -- minus the .S
S_PIECES=
S_FILES=$(S_PIECES:%=%.S)
S_O_FILES=$(S_FILES:%.S=${ARCH}/%.o)

SRCS=$(C_FILES) $(CC_FILES) $(H_FILES) $(S_FILES)
OBJS=$(C_O_FILES) $(CC_O_FILES) $(S_O_FILES)

include $(RTEMS_MAKEFILE_PATH)/Makefile.inc

include $(RTEMS_CUSTOM)
include $(RTEMS_ROOT)/make/lib.cfg

#
# Add local stuff here using +=
#

DEFINES  +=
CPPFLAGS += -I.
# inline declarations require -O
CFLAGS   += -Winline -D__IPSBAR=__IPSBAR

#
# Add your list of files to delete here.  The config files
#  already know how to delete some stuff, so you may want
#  to just run 'make clean' first to see what gets missed.
#  'make clobber' already includes 'make clean'
#

CLEAN_ADDITIONS +=
CLOBBER_ADDITIONS +=

all:	${ARCH} $(SRCS) $(PGMS) $(LIB)

#
# doesn't work if we define this just after OBJS= :-(
# must be after inclusion of RTEMS_CUSTOM
$(LIB): OBJS:=$(filter-out %.modini.o,$(OBJS))

#How to make a relocatable object
$(filter %.obj, $(PGMS)): ${OBJS}
	$(make-obj)

$(LIB): ${OBJS}
	$(make-library)

ifndef RTEMS_SITE_INSTALLDIR
RTEMS_SITE_INSTALLDIR = $(PROJECT_RELEASE)
endif

${RTEMS_SITE_INSTALLDIR}/lib \
${RTEMS_SITE_INSTALLDIR}/include/bsp:
	test -d $@ || mkdir -p $@

# Install the library, appending _g or _p as appropriate.
# for include files, just use $(INSTALL_CHANGE)
install:  all ${RTEMS_SITE_INSTALLDIR}/lib $(RTEMS_SITE_INSTALLDIR)/bin ${RTEMS_SITE_INSTALLDIR}/include/bsp/
	$(INSTALL_VARIANT) -m 644 ${LIB} ${RTEMS_SITE_INSTALLDIR}/lib/
	$(INSTALL_VARIANT) -m 644 ${PGMS} ${RTEMS_SITE_INSTALLDIR}/bin/
	$(INSTALL_CHANGE) -m 644 ${H_FILES} ${RTEMS_SITE_INSTALLDIR}/include/bsp/

REVISION=$(filter-out $$%,$$Name$$)
tar:
	@$(make-tar)
