ADD_ON = ../../..

include $(ADD_ON)/src/config/CONFIG

USR_CFLAGS = $(MOTIF_CFLAGS) $(X11_CFLAGS)

USR_LDFLAGS = $(STATICLD) $(LIBNAME) $(MOTIF_LDFLAGS) $(X11_LDFLAGS) -lm


SRCS = BarGraph.c Caption.c Clip.c Control.c Indicator.c Matrix.c \
	Meter.c Value.c bae.c bar1.c indicator1.c meter1.c utils.c version.c

OBJS = BarGraph.o Caption.o Clip.o Control.o Indicator.o Matrix.o \
	Meter.o Value.o utils.o version.o

LIBNAME = libANLwidgets.a

include $(ADD_ON)/src/config/RULES


meter1: $(OBJS) $(DEP_LIBS) meter1.c
	$(CC) -o $@ $@.c $(LDFLAGS)

indicator1 : $(OBJS) $(DEP_LIBS) indicator1.c
	$(CC) -o $@ $@.c $(LDFLAGS)

bar1: $(OBJS) $(DEP_LIBS) bar1.c
	$(CC) -o $@ $@.c $(LDFLAGS)

