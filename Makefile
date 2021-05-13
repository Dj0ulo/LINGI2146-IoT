# MAKE_ROUTING = MAKE_ROUTING_RPL_CLASSIC
CONTIKI_PROJECT = udp-client udp-server
all: $(CONTIKI_PROJECT)

PROJECT_SOURCEFILES += protocol.c

CONTIKI=..
include $(CONTIKI)/Makefile.include
