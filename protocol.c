#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "sys/log.h"
#include "defines.h"

typedef struct
{
  struct simple_udp_connection connection;
  clock_time_t time_sent;
  struct ctimer timer;
  unsigned count_timeout;
  uint8_t last_buffer_sent[SIZE_PACKET];
  packet last_packet_recv;
  uint16_t last_id;
  uip_ipaddr_t dest_ipaddr;
  void (*callback)(packet p);
} sending_infos;

static sending_infos si;

/**
 * From : https://stackoverflow.com/questions/21001659/crc32-algorithm-implementation-in-c-without-a-look-up-table-and-with-a-public-li#21001712
 * @param buffer: The buffer to check
 * @return the value of the checksum
 */
uint32_t crc32b(const uint8_t *buffer, size_t len)
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

void log_bytes_packet(const uint8_t *buffer, size_t len)
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

  si.last_id = p.random_id;

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

packet parse_packet(const uint8_t *buffer, size_t len)
{
  // uint8_t buffer[SIZE_PACKET] = buffer_ptr;
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

void callback_timeout();
void send_last(){
  LOG_INFO("Packet (%u)\n", (unsigned)parse_packet(si.last_buffer_sent, SIZE_PACKET).random_id);

  simple_udp_sendto(&si.connection, si.last_buffer_sent, SIZE_PACKET, &si.dest_ipaddr);
  ctimer_set(&si.timer, RESEND_TM, callback_timeout, NULL);
}

void callback_timeout()
{
  if (si.count_timeout >= TIMEOUT_COUNT)
  {
    LOG_INFO("REQUEST TIMEOUT\n");
    packet p;
    p.status = ERR_TIMEOUT;
    if(si.callback)
      si.callback(p);
    return;
  }
  si.count_timeout++;
  LOG_INFO("Time done %u, resending...\n", (unsigned)(1000 * (clock_time() - si.time_sent) / CLOCK_SECOND));

  send_last();
}

void callback_receive(struct simple_udp_connection *c,
                      const uip_ipaddr_t *sender_addr,
                      uint16_t sender_port,
                      const uip_ipaddr_t *receiver_addr,
                      uint16_t receiver_port,
                      const uint8_t *data,
                      uint16_t datalen)
{

  ctimer_stop(&si.timer);

  LOG_INFO("PROTOCOL packet received from ");
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");

  packet p = parse_packet(data, datalen);
  log_packet(p);

  if (p.status != OK)
  {
    LOG_INFO("There was an error %u, resending the same packet\n", (unsigned)p.status);

    send_last();
    return;
  }
  if (p.type == NACK)
  {
    LOG_INFO("A NACK was sent back, resending the same packet\n");

    send_last();
    return;
  }
  LOG_INFO("Responded\n");
  if(si.callback)
    si.callback(p);

}

void connect()
{
  simple_udp_register(&si.connection, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, callback_receive);
}


void send_request(const uip_ipaddr_t *dest_ipaddr, uint8_t type, uint32_t value, void (*callback)(packet p))
{
  packet p;
  p.is_response = 0;
  p.type = type;
  p.value = value;

  si.dest_ipaddr = *dest_ipaddr;
  si.count_timeout = 0;
  si.time_sent = clock_time();
  si.callback = callback;

  set_packet(si.last_buffer_sent, p);
  send_last();
}

void callback_respond(struct simple_udp_connection *c,
                      const uip_ipaddr_t *sender_addr,
                      uint16_t sender_port,
                      const uip_ipaddr_t *receiver_addr,
                      uint16_t receiver_port,
                      const uint8_t *data,
                      uint16_t datalen)
{

  LOG_INFO("Packet received from ");
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");

  if (random_rand() % 2 == 0)
  {
    LOG_INFO("Simulate loss\n");
    return;
  }

  uint8_t cpy_data[datalen];
  memcpy(cpy_data, data, datalen);

  if (random_rand() % 3 == 0)
  {
    LOG_INFO("Simulate corruption\n");
    cpy_data[random_rand() % datalen] = (uint8_t)random_rand() % 0xFF;
  }

  packet recv_p = parse_packet(cpy_data, datalen);

  int first_time = si.last_packet_recv.crc != recv_p.crc;
  si.last_packet_recv = recv_p;

  log_bytes_packet(cpy_data, datalen);
  log_packet(recv_p);

  packet back_p;
  back_p.is_response = 1;
  back_p.random_id = recv_p.random_id;

  if (recv_p.status != OK )
  {
    LOG_INFO("Error %u, sending NACK.\n", (unsigned)recv_p.status);
    back_p.type = NACK;
    back_p.value = recv_p.status;
  }
  else
  {
    LOG_INFO("Sending response.\n");

    //here manage what to respond

    // If packet received is valid and has never been here do:
    if(first_time){
      si.callback(recv_p);
    }

    back_p.type = ACK;
    back_p.value = 0;
  }
  set_packet(si.last_buffer_sent, back_p);
  simple_udp_sendto(&si.connection, si.last_buffer_sent, SIZE_PACKET, sender_addr);
}

void listen(void (*callback)(packet p))
{
  si.callback = callback;
  simple_udp_register(&si.connection, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, callback_respond);
}
