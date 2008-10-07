#  $Id$
AUTOMAKE_OPTIONS=foreign

include $(top_srcdir)/rtems-pre.am

EXEEXT=$(OBJEXEEXT)

libcoldfUtils_a_SOURCES       = eport.c drv5282DMA.c cselect.c fecmii.c drv5282QSPI.c flash.c

include_bsp_HEADERS           = coldfUtils.h

lib_LIBRARIES                 = libcoldfUtils.a

if HAVE_CEXP
bin_PROGRAMS                  = coldfUtils
coldfUtils_SOURCES            = coldfUtils.modini.c
coldfUtils_LDADD              = $(libcoldfUtils_a_OBJECTS)
coldfUtils_DEPENDENCIES       = $(libcoldfUtils_a_OBJECTS)
coldfUtils_LINK               = $(OBJLINK)
endif


AM_CFLAGS                    +=-Winline
AM_CPPFLAGS                  +=-D__IPSBAR=__IPSBAR

include $(top_srcdir)/rtems.am
