#
# $Id$
#
# Makefile for building subdirectories
#
# $Log$
# Revision 1.2  1994/10/05 18:56:29  jba
# Initial versions
#
#

EPICS=../../..
include $(EPICS)/config/CONFIG_EXTENSIONS

DIRS = graphX graphX/printUtils xc medm

include $(EPICS)/config/RULES_DIRS

