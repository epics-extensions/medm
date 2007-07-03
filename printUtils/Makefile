#*************************************************************************
# Copyright (c) 2002 The University of Chicago, as Operator of Argonne
# National Laboratory.
# Copyright (c) 2002 The Regents of the University of California, as
# Operator of Los Alamos National Laboratory.
# This file is distributed subject to a Software License Agreement found
# in the file LICENSE that is included with this distribution. 
#*************************************************************************
#
# $Id$
#
TOP = ../../..
include $(TOP)/configure/CONFIG

SHARED_LIBRARIES = NO

CMPLR = STRICT

# Motif
USE_MOTIF12=NO
#USE_MOTIF12=YES

# Optimization
ifdef WIN32
#HOST_OPT = NO
else
#HOST_OPT = NO
endif

# Compiler specific
#GCC_WARN_YES = -Wall -Wtraditional -Wmissing-prototypes -Wstrict-prototypes
GCC_WARN_YES = -Wall

USR_INCLUDES = -I$(X11_INC)

ifdef WIN32
USR_CFLAGS_WIN32 += /DWIN32 /D_WINDOWS
USR_CFLAGS_WIN32 += $(EXCEED_CFLAGS)

# The proper import/export declarations are not included for DLLs
SHARED_LIBRARIES = NO
endif

ifeq ($(USE_MOTIF12),YES)
USR_CFLAGS_solaris += -DMOTIF12_HEADERS
endif

LIBSRCS += getopt.c
LIBSRCS += pUtils.c
LIBSRCS += ps_utils.c
LIBSRCS += utilPrint.c
LIBSRCS += xwd.c
LIBSRCS += xwd2ps.c

LIBRARY_HOST = PrintUtils

include $(TOP)/configure/RULES
