# MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC
CONTIKI_PROJECT = movement_detector lamp root
all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += packet.c protocol.c

CFLAGS += -std=gnu11

CONTIKI=..
include $(CONTIKI)/Makefile.include
