#
# Makefile for GraphX library testing
#
# Mark Anderson		(anderson@el3.el.anl.gov)
#
CC = cc -D_NO_PROTO


GRAPHX_H = 	../h
GRAPHX_LIB = 	../lib

#CFLAGS  =	-g  -I$(GRAPHX_H) -I/usr/include/Xm -I/usr/include/X11 -I. 
CFLAGS  =	-O  -I$(GRAPHX_H) -I/usr/include/Xm -I/usr/include/X11 -I. 

GRAPHX_LIBS =	$(GRAPHX_LIB)/graphApp.a $(GRAPHX_LIB)/graphX.a \
			$(GRAPHX_LIB)/printUtils.a

LIBS = $(GRAPHX_LIBS) -lXm -lXt -lX11 -lm

PROGS =	 testHist  testGraph testGraph3d testStrip testSurface testSurface_erf \
			testSeql testGraphApp1 testGraphApp5 

default: $(PROGS)

all: $(PROGS)
	@echo " "
	@echo ">> be sure to:  *** setenv XENVIRONMENT ./Xdefaults ***"
	@echo " "


testHist:	testHist.o $(GRAPHX_LIB)
		$(CC) $(CFLAGS) testHist.o -o $@  $(LIBS)

testSeql:	testSeql.o $(GRAPHX_LIB)
		$(CC) $(CFLAGS) testSeql.o -o $@  $(LIBS)

testGraph:	testGraph.o $(GRAPHX_LIB)
		$(CC) $(CFLAGS) testGraph.o -o $@  $(LIBS)

testGraph3d:	testGraph3d.o $(GRAPHX_LIB)
		$(CC) $(CFLAGS) testGraph3d.o -o $@  $(LIBS)

testStrip:	testStrip.o $(GRAPHX_LIB)
		$(CC) $(CFLAGS) testStrip.o -o $@  $(LIBS)

testSurface:	testSurface.o $(GRAPHX_LIB)
		$(CC) $(CFLAGS) testSurface.o -o $@  $(LIBS)

testSurface_erf: testSurface_erf.o $(GRAPHX_LIB)
		$(CC) $(CFLAGS) testSurface_erf.o -o $@  $(LIBS)

testGraphApp1:	testGraphApp1.o $(GRAPHX_LIB)
		$(CC) $(CFLAGS) testGraphApp1.o -o $@  $(LIBS)

testGraphApp5:	testGraphApp5.o $(GRAPHX_LIB)
		$(CC) $(CFLAGS) testGraphApp5.o -o $@  $(LIBS)


clean::
		rm -f *.o
		rm -f $(PROGS) 



testGraph.o: 	testGraph.c  GraphX.h 

testGraph3d.o: 	testGraph3d.c  GraphX.h 

testHist.o:	testHist.c  GraphX.h 

testStrip.o:	testStrip.c GraphX.h

testSurface.o:	testSurface.c GraphX.h 

testSurface_erf.o: testSurface_erf.c GraphX.h 

testGraphApp1.o: testGraphApp1.c GraphX.h

testGraphApp5.o: testGraphApp5.c GraphX.h

GraphX.h : $(GRAPHX_H)/GraphX.h $(GRAPHX_H)/Graph.h \
	$(GRAPHX_H)/Graph3d.h $(GRAPHX_H)/Seql.h $(GRAPHX_H)/Strip.h \
	$(GRAPHX_H)/Surface.h $(GRAPHX_H)/3D.h
