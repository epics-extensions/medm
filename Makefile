#*************************************************************************
# Copyright (c) 2002 The University of Chicago, as Operator of Argonne
# National Laboratory.
# Copyright (c) 2002 The Regents of the University of California, as
# Operator of Los Alamos National Laboratory.
# This file is distributed subject to a Software License Agreement found
# in the file LICENSE that is included with this distribution. 
#*************************************************************************
# Makefile for medm top level

TOP = ../..
ifneq ($(wildcard $(TOP)/config)x,x)
  # New Makefile.Host config file location
  include $(TOP)/config/CONFIG_EXTENSIONS
  DIRS = printUtils xc medm
  include $(TOP)/config/RULES_DIRS
else
  # Old Makefile.Unix config file location
  EPICS=../../..
  include $(EPICS)/config/CONFIG_EXTENSIONS
  DIRS = printUtils xc medm
  include $(EPICS)/config/RULES_DIRS
endif
