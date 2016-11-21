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

# gprof
#USR_CFLAGS += -pg
#USR_LDFLAGS += -pg

# Source browser options
ifeq ($(HOST_OPT),NO)
  ifeq ($(ANSI),ACC)
    ifeq ($(OS_CLASS),solaris)
      USR_CFLAGS += -xsb
    endif
  endif
endif

#STATIC_BUILD = YES
CMPLR = STRICT

# Purify
ifeq ($(OS_CLASS),solaris)
#PURIFY=YES
#DEBUGCMD = purify $(PURIFY_FLAGS)
PURIFY_FLAGS = -first-only -chain-length=26
#PURIFY_FLAGS = -first-only -chain-length=26 -max_threads=256
#PURIFY_FLAGS = -first-only -chain-length=26 -always-use-cache-dir -cache-dir=/tmp/purifycache
#PURIFY_FLAGS = -first-only -chain-length=26 -enable-new-cache-scheme
#PURIFY_FLAGS = -first-only -chain-length=26 -enable-new-cache-scheme -always-use-cache-dir -cache-dir=/tmp/purifycache

# Put the cache files in the appropriate bin directory
PURIFY_FLAGS += -always-use-cache-dir -cache-dir=$(shell $(PERL) $(TOP)/config/fullPathName.pl .)
#PURIFY_FLAGS += -force-rebuild=no
# Seems to solve problem with duplicate libc, libdl, libnsl, libsocket
PURIFY_FLAGS += -follow-terminal-symlinks
endif

# Web browser options
#USR_CFLAGS += -DFIREFOXPATH="\"firefox\""

# Use to test Xrt/Graph 2.4, Otherwise should come from CONFIG
#XRTGRAPH=/opt/local/xrtgraph2.4.0
#XRTGRAPH_EXTENSIONS =

# Use to test SciPlot or JPT, otherwise should come from CONFIG
# Set the one you want and reset any that come from CONFIG
#XRTGRAPH = 
#SCIPLOT = YES
#JPT = YES

# Use to test CDEV, Otherwise should come from CONFIG
#MEDM_CDEV = YES

# Set up for Xrt/Graph
ifdef XRTGRAPH
  SCIPLOT =     # Don't allow more than one plot package
  JPT =         # Don't allow more than one plot package
  #USR_CFLAGS += -I$(XRTGRAPH)/include/Xm  -D __COLOR_RULE_H__
  USR_CFLAGS += -DXRTGRAPH
  USR_CFLAGS += -I$(XRTGRAPH)/include/Xm
  USR_LIBS += xrtm
  xrtm_DIR = $(XRTGRAPH)/lib

#  The following sets up for the XRT/graph 3.0 extensions
  ifdef XRTGRAPH_EXTENSIONS
    ifeq ($(XRTGRAPH_EXTENSIONS),YES)
      USR_CFLAGS += -DXRT_EXTENSIONS
#      USR_LIBS += xrtmprop xrtfield xrttable xrtgear Xpm Xext
# XRT 4.0
      USR_LIBS += xrtmprop xrtfield xrttable xrtgear pdsutil Xpm
      xrtmprop_DIR = $(XRTGRAPH)/lib
      xrtfield_DIR = $(XRTGRAPH)/lib
      xrttable_DIR = $(XRTGRAPH)/lib
      xrtgear_DIR = $(XRTGRAPH)/lib
      pdsutil_DIR = $(XRTGRAPH)/lib
#     Xpm_DIR = $(X11_LIB)
      Xpm_DIR = $(XRTGRAPH)/lib
    endif
  endif

  SRCS += medmCartesianPlot.c
  SRCS += medmXrtGraph.c
endif     #ifdef XRTGRAPH

# Set up for SciPLot
ifdef SCIPLOT
  JPT =         # Don't allow more than one plot package
  USR_CFLAGS += -DSCIPLOT -DMOTIF
  SRCS += medmCartesianPlot.c
  SRCS += medmSciPlot.c
  SRCS += SciPlot.c
endif

# Set up for JPT
ifdef JPT
  JPT_INC = ../../../jpt
  USR_CFLAGS += -DJPT -I$(JPT_INC)
  USR_LIBS += Jpt
#  USR_LDFLAGS += -L$(JPT_LIB)
  SRCS += medmCartesianPlot.c
  SRCS += medmJpt.c
endif

USR_CFLAGS += -DEDITRES

PROD_HOST = medm

USR_INCLUDES = -I../../printUtils -I../../xc -I$(MOTIF_INC) -I$(X11_INC)

PROD_LIBS = ANLwidgets PrintUtils
ANLwidgets_DIR = $(INSTALL_LIB)
PrintUtils_DIR = $(INSTALL_LIB)

# Setup for either CA or CDEV
ifdef MEDM_CDEV
  cdev_DIR = $(CDEVLIB)
  PROD_LIBS += cdev
  SRCS += medmCdev.cc
  SRCS += medmCdevUtils.c
  USR_LDFLAGS += -R$(CDEVLIB) -L$(CDEVLIB)

  USR_INCLUDES += -I. -I.. -I$(CDEV)/include
#  USR_INCLUDES += -I/usr/user1/chen/local/Xmu/include
  USR_CFLAGS += -DMEDM_CDEV
  USR_CCFLAGS += $(USR_CFLAGS)
  SRCS += medmCdev.cc
  SRCS += medmCdevUtils.c
else
  ca_DIR = $(EPICS_BASE_LIB)
  Com_DIR = $(EPICS_BASE_LIB)
  PROD_LIBS += ca Com
  SRCS += medmCA.c
endif

USR_LIBS_DEFAULT = Xm Xt Xmu X11 Xext

ifeq ($(USE_MOTIF12),YES)
USR_CFLAGS_solaris += -DMOTIF12_HEADERS
USR_LIBS_solaris = Xm12 Xt Xmu X11 Xext
Xm12_DIR = $(MOTIF_LIB)
endif

USR_LIBS_Linux = Xm Xt Xp Xmu X11 Xext
#Xp is not available and not needed on Linux Mint 18 
#USR_LIBS_Linux = Xm Xt Xmu X11 Xext

USR_LIBS_cygwin32 = Xm Xt Xmu X11 Xext SM ICE
USR_LIBS_Darwin = -nil-
PROD_SYS_LIBS_Darwin = Xt Xmu X11 Xext Xft Xp fontconfig png

#Do not use PROD_LDFLAGS_Darwin because it goes after OP_SYS_LDFLAGS
USR_LDFLAGS_Darwin = -L/usr/X11/lib /opt/local/lib/libXm.a /opt/local/lib/libjpeg.a /opt/local/lib/libiconv.a

WIN32_RUNTIME=MD
USR_CFLAGS_WIN32 += /DWIN32 /D_WINDOWS
USR_LDFLAGS_WIN32 += /SUBSYSTEM:WINDOWS
USR_LIBS_WIN32 += $(EXCEED_XLIBS)
USR_CFLAGS_WIN32 += $(EXCEED_CFLAGS)

Xm_DIR = $(MOTIF_LIB)
Xt_DIR = $(X11_LIB)
X11_DIR = $(X11_LIB)
Xmu_DIR = $(X11_LIB)
Xp_DIR = $(X11_LIB)
Xext_DIR = $(X11_LIB)
Xft_DIR = $(X11_LIB)
fontconfig_DIR = $(X11_LIB)
png_DIR = $(X11_LIB)
jpeg_DIR = $(MOTIF_LIB)
iconv_DIR = $(MOTIF_LIB)

SRCS += actions.c
SRCS += bubbleHelp.c
SRCS += callbacks.c
SRCS += browserHelp.c
SRCS += channelPalette.c
SRCS += colorPalette.c
SRCS += dialogs.c
SRCS += display.c
SRCS += eventHandlers.c
SRCS += help.c
SRCS += medm.c
SRCS += medmArc.c
SRCS += medmBar.c
SRCS += medmByte.c
SRCS += medmChoiceButtons.c
SRCS += medmCommon.c
SRCS += medmComposite.c
SRCS += medmControl.c
SRCS += medmDisplay.c
SRCS += medmImage.c
SRCS += medmIndicator.c
SRCS += medmMenu.c
SRCS += medmMessageButton.c
SRCS += medmMeter.c
SRCS += medmMonitor.c
SRCS += medmOval.c
SRCS += medmPixmap.c
SRCS += medmPolygon.c
SRCS += medmPolyline.c
SRCS += medmRectangle.c
SRCS += medmRelatedDisplay.c
SRCS += medmShellCommand.c
SRCS += medmStripChart.c
SRCS += medmText.c
SRCS += medmTextEntry.c
SRCS += medmTextUpdate.c
SRCS += medmValuator.c
SRCS += medmWheelSwitch.c
SRCS += medmWidget.c
SRCS += objectPalette.c
SRCS += productDescriptionShell.c
SRCS += resourcePalette.c
SRCS += updateMonitors.c
SRCS += updateTask.c
SRCS += utils.c
SRCS += xgif.c

RCS_WIN32 += medm.rc

include $(TOP)/configure/RULES

medm.res:../medm.ico

ifdef XRTGRAPH
build: xrtAuth
endif

xrtAuth: medm
	# --- RUN XRT_AUTH on resulting MEDM for proper authorization
	@-export XRTHOME; XRTHOME=$(XRTGRAPH); $(XRTGRAPH)/bin/xrt_auth medm;\
	if [ $$? -ne 0 ]; then\
		echo "************************************************";\
		echo "ERROR in running XRT_AUTH on MEDM";\
		echo "";\
		echo "      ERROR!!!      ERROR!!!      ERROR!!!";\
		echo "";\
		echo "      ERROR!!!      ERROR!!!      ERROR!!!";\
		echo "";\
		echo "      ERROR!!!      ERROR!!!      ERROR!!!";\
		echo "";\
		echo "************************************************";\
	else\
		echo "**************************************************************";\
		echo "License warnings for XRT field, table, and gear may be ignored";\
		echo "Patch successful";\
		echo "**************************************************************";\
	fi

xxxx:
	@which echo
	@echo PERL: $(PERL)
	@echo PROD: $(PROD)
	@echo ca_DIR: $(ca_DIR)
	@echo RCS_WIN32: $(RCS_WIN32)
	@echo RES: $(RES)
	@echo PROD_RESS: $(PROD_RESS)
	@echo XRTGRAPH_EXTENSIONS: $(XRTGRAPH_EXTENSIONS)
	@echo XRTGRAPH: $(XRTGRAPH)
	@echo JPT: $(JPT)
	@echo PURIFY: $(PURIFY)
	@echo PURIFYCMD: $(PURIFYCMD)
	@echo OS_CLASS: $(OS_CLASS)
	@echo USR_CFLAGS: $(USR_CFLAGS)
	@echo USR_CCFLAGS: $(USR_CCFLAGS)
	@echo CC: $(CC)
	@echo MOTIF_LIB: $(MOTIF_LIB)
	@echo MOTIF_INC: $(MOTIF_INC)
	@echo X11_LIB: $(X11_LIB)
	@echo X11_INC: $(X11_INC)

# **************************** Emacs Editing Sequences *****************
# Local Variables:
# mode: makefile
# End:
