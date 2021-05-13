#include "contiki.h"
#include "sys/log.h"
#include "random.h"

#include "packet.h"
#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_MODULE "Packet"

/**
 * From : https://stackoverflow.com/questions/21001659/crc32-algorithm-implementation-in-c-without-a-look-up-table-and-with-a-public-li#21001712
 * @param buffer: The buffer to check
 * @return the value of the checksum
 */
uint32_t crc32b(const uint8_t *buffer, unsigned len)
{
  uint32_t byte, crc = 0xFFFFFFFF, mask;

  int i;
  for (i = 0; i < len; i++)
  {
    byte = buffer[i]; // Get next byte.
    crc = crc ^ byte;
    int j;
    for (j = 7; j >= 0; j--)
    { // Do eight times.
      mask = -(crc & 1);
      crc = (crc >> 1) ^ (0xEDB88320 & mask);
    }
  }
  return ~crc;
}

void log_bytes_packet(const uint8_t *buffer, unsigned len)
{
  int i = 0;
  LOG_INFO("Bytes (%u) : ", (unsigned)len);
  for (i = 0; i < len; i++)
    LOG_INFO_("%02x ", (unsigned)buffer[i]);
  LOG_INFO_("\n");
}

void log_packet(packet p)
{
  if (p.status == ERR_LEN)
  {
    LOG_INFO("Packet : [UNVALID SIZE]\n");
    return;
  }
  if (p.status == ERR_START)
  {
    LOG_INFO("Packet : [UNVALID START]\n");
    return;
  }
  if (p.status == ERR_END)
  {
    LOG_INFO("Packet : [UNVALID END]\n");
    return;
  }
  if (p.status == ERR_CRC)
  {
    LOG_INFO("Packet : [UNVALID CHECKSUM]\n");
    return;
  }
  LOG_INFO("Packet (id: %u) : %s %u : %u\n", (unsigned)p.random_id, p.is_response ? "response " : "request ", (unsigned)p.type, (unsigned)p.value);
}

void set_packet(uint8_t *buffer_ptr, packet p)
{
  unsigned off = 0;
  memset(buffer_ptr, START_PACKET, 1);
  off += 1;

  p.status = OK;
  memcpy(buffer_ptr + off, &p.status, 1);
  off += 1;

  if(!p.is_response)
    p.random_id = random_rand();

  memcpy(buffer_ptr + off, &p.random_id, 2);
  off += 2;
  memcpy(buffer_ptr + off, &p.is_response, 1);
  off += 1;
  memcpy(buffer_ptr + off, &p.type, 1);
  off += 1;
  memcpy(buffer_ptr + off, &p.value, 4);
  off += 4;

  p.crc = crc32b(buffer_ptr + 1, off - 1);
  memcpy(buffer_ptr + off, &p.crc, 4);
  off += 4;

  memset(buffer_ptr + off, END_PACKET, 1);
}

packet parse_packet(const uint8_t *buffer, unsigned len)
{
  packet params;

  if (len != SIZE_PACKET)
  {
    params.status = ERR_LEN;
    return params;
  }

  if (buffer[0] != START_PACKET)
  {
    params.status = ERR_START;
    return params;
  }
  if (buffer[SIZE_PACKET - 1] != END_PACKET)
  {
    params.status = ERR_END;
    return params;
  }

  unsigned off = 1;

  params.status = buffer[off];
  off += 1;
  memcpy(&params.random_id, buffer + off, 2);
  off += 2;
  params.is_response = buffer[off];
  off += 1;
  params.type = buffer[off];
  off += 1;
  memcpy(&params.value, buffer + off, 4);
  off += 4;

  memcpy(&params.crc, buffer + off, 4);

  if (params.crc != crc32b(buffer + 1, off - 1))
    params.status = ERR_CRC;

  return params;
}