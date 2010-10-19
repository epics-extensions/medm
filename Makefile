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
include $(TOP)/configure/CONFIG
DIRS = printUtils xc medm
include $(TOP)/configure/RULES_DIRS

medm_DEPEND_DIRS = printUtils xc
