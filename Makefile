# Makefile for medm top level

ifeq ($(HOST_ARCH),WIN32)

TOP=../..

include $(TOP)/config/CONFIG_EXTENSIONS

#DIRS = graphX graphX/printUtils xc medm
DIRS = graphX xc medm

include $(TOP)/config/RULES_DIRS

else

EPICS=../../..

include $(EPICS)/config/CONFIG_EXTENSIONS

DIRS = graphX graphX/printUtils xc medm

include $(EPICS)/config/RULES_DIRS

endif
