
GRAPHX_DIR = 	..

CFLAGS  =	-O  -I$(GRAPHX_DIR)  -I/usr/include/Xm \
					-I/usr/include/X11 -I. 

GRAPHX_LIB = $(GRAPHX_DIR)/graphApp.a $(GRAPHX_DIR)/graphX.a \
					$(GRAPHX_DIR)/printUtils.a 

LIBS = $(GRAPHX_LIB) -lXm -lXt -lX11 -lm


GRAPHAPP_OBJS = graphApp.o
GRAPHAPP_INCLUDES = GraphApp.h



all: graphApp.a ../../lib/graphApp.a

../../lib/graphApp.a : graphApp.a
	cp $(GRAPHAPP_INCLUDES) ../../h
	cp graphApp.a ../../lib
	ranlib ../../lib/graphApp.a

graphApp.a: $(GRAPHAPP_OBJS) $(GRAPHAPP_INCLUDES)
	ar crv $@ $(GRAPHAPP_OBJS)
	ar tv $@
	ranlib $@


graphApp.o: 	graphApp.c GraphApp.h
	$(CC) -c $(CFLAGS)  graphApp.c 


clean:
	rm -f *.[oa]
