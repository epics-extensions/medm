ADD_ON = ../../..

include $(ADD_ON)/src/config/CONFIG


USR_CFLAGS = $(MOTIF_CFLAGS) $(X11_CFLAGS)

SUB_DIRS =  printUtils

include $(ADD_ON)/src/config/HRULES


SRCS = algorithms.c graph.c graph3d.c graphInteraction.c seql.c seqlInteraction.c \
	strip.c surface.c surfaceInternals.c utils.c

OBJS = algorithms.o graph.o graph3d.o graphInteraction.o seql.o seqlInteraction.o \
	strip.o surface.o surfaceInternals.o utils.o

LIBNAME = libGraphX.a

include $(ADD_ON)/src/config/RULES

