# MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC
CONTIKI_PROJECT = lamp root
all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += packet.c protocol.c

CFLAGS += -std=gnu11

CONTIKI=..
include $(CONTIKI)/Makefile.include
