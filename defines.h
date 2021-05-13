#ifndef DEFINES_H
#define DEFINES_H

#include "contiki.h"

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define SIZE_PACKET 15
#define START_PACKET 0xFF
#define END_PACKET 0xFF

#define NO_ERR 1
#define ERR_LEN 0xFF-1
#define ERR_CRC 0xFF-2
#define ERR_START 0xFF-3
#define ERR_END 0xFF-4

#define RESEND_TM CLOCK_SECOND/5
#define TIMEOUT_COUNT 20

#define ACK 0
#define NACK 1
#define LEDS_ON 2
#define LEDS_OFF 2

enum{RED, GREEN, BLUE};


typedef struct 
{
  uint8_t is_valid;
  uint16_t random_id;
  uint8_t is_response;
  uint8_t type;
  uint32_t value;
  uint32_t crc;
} packet;

#endif