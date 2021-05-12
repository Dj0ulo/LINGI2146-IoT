#ifndef DEFINES_H
#define DEFINES_H

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define SIZE_PACKET 9
#define START_PACKET 0xFF
#define END_PACKET 0xFF

#include "contiki.h"

typedef struct 
{
  uint8_t is_valid;
  uint8_t is_response;
  uint8_t type;
  uint32_t value;
} packet;

#endif