ADD_ON = ../../..

include $(ADD_ON)/src/config/CONFIG

USR_CFLAGS = -I../graphX -I../xc -I$(XRTGRAPH)/include/Xm \
			$(MOTIF_CFLAGS) $(X11_CFLAGS)

USR_LDFLAGS = $(STATICLD) \
	$(XRTGRAPH)/lib/libxrtm.a ../xc/libANLwidgets.a \
	../graphX/printUtils/libPrintUtils.a ../graphX/libGraphX.a \
	$(MOTIF_LDFLAGS) $(X11_LDFLAGS) -lm

SRCS = actions.c caDM.c callbacks.c channelPalette.c colorPalette.c \
	 createControllers.c createExtensions.c createMonitors.c \
	createStatics.c display.c dmInit.c dummy.c \
	eventHandlers.c executeControllers.c executeExtensions.c \
	executeMonitors.c executeStatics.c help.c medm.c medmPixmap.c \
	objectPalette.c parseControllers.c parseExtensions.c parseMonitors.c \
	parseStatics.c resourcePalette.c shared.c \
	updateMonitors.c utils.c widgetDM.c writeControllers.c \
	writeExtensions.c writeMonitors.c writeStatics.c xgif.c

OBJS = actions.o caDM.o callbacks.o channelPalette.o colorPalette.o \
	createControllers.o createExtensions.o createMonitors.o \
	createStatics.o display.o dmInit.o dummy.o \
	eventHandlers.o executeControllers.o executeExtensions.o \
	executeMonitors.o executeStatics.o help.o medm.o medmPixmap.o \
	objectPalette.o parseControllers.o \
	parseExtensions.o parseMonitors.o parseStatics.o resourcePalette.o \
	shared.o updateMonitors.o utils.o widgetDM.o writeControllers.o \
	writeExtensions.o writeMonitors.o writeStatics.o xgif.o

PROD = medm

include $(ADD_ON)/src/config/RULES

CC = acc -g 
medm: $(OBJS) $(DEP_LIBS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
	# --- RUN XRT_AUTH on resulting MEDM for proper authorization
	@-export XRTHOME; XRTHOME=$(XRTGRAPH); $(XRTGRAPH)/bin/xrt_auth medm;\
	if [ $$? -ne 0 ]; then\
		echo "*********************************";\
		echo "ERROR in running XRT_AUTH on MEDM";\
		echo "*********************************";\
	fi
#

xrtAuth : medm
	# --- RUN XRT_AUTH on resulting MEDM for proper authorization
	@-export XRTHOME; XRTHOME=$(XRTGRAPH); $(XRTGRAPH)/bin/xrt_auth medm;\
	if [ $$? -ne 0 ]; then\
		echo "*********************************";\
		echo "ERROR in running XRT_AUTH on MEDM";\
		echo "*********************************";\
	fi


