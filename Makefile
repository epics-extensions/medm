# Makefile for medm top level

TOP = ../..
ifneq ($(wildcard $(TOP)/config)x,x)
  # New Makefile.Host config file location
  include $(TOP)/config/CONFIG_EXTENSIONS

  ifeq ($(HOST_ARCH),WIN32)
   DIRS = graphX xc medm
  else
   DIRS = graphX graphX/printUtils xc medm
  endif

  include $(TOP)/config/RULES_DIRS
else
  # Old Makefile.Unix config file location
  EPICS=../../..
  include $(EPICS)/config/CONFIG_EXTENSIONS

  ifeq ($(HOST_ARCH),WIN32)
   DIRS = graphX xc medm
  else
   DIRS = graphX graphX/printUtils xc medm
  endif

  include $(EPICS)/config/RULES_DIRS
endif

