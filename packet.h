#ifndef PACKET_H
#define PACKET_H

#define SIZE_PACKET 15
#define START_PACKET 0xFF
#define END_PACKET 0xFF

#define OK 1
#define ERR_LEN 0xFF-1
#define ERR_CRC 0xFF-2
#define ERR_START 0xFF-3
#define ERR_END 0xFF-4
#define ERR_TIMEOUT 0xFF-5

#define RESEND_TM CLOCK_SECOND/5
#define TIMEOUT_COUNT 20



enum{RED, GREEN, BLUE};

#include "contiki.h"

typedef struct 
{
  uint8_t status;
  uint16_t random_id;
  uint8_t is_response;
  uint8_t type;
  uint32_t value;
  uint32_t crc;
} packet;


void log_bytes_packet(const uint8_t* buffer, unsigned len);
void log_packet(packet p);
void set_packet(uint8_t *buffer_ptr, packet p);
packet parse_packet(const uint8_t *buffer, unsigned len);

#endif