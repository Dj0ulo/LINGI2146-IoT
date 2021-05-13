#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "packet.h"

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

enum PacketType{NODE_TYPE, ACK, NACK, LEDS_ON, LEDS_OFF};

typedef struct simple_udp_connection conn;

#define MAX_CONNECTIONS 10

enum NodeType {NOT_SET, LAMP};
typedef struct 
{
  uint8_t connected;
  uint8_t type;
  conn connection;
  packet last_packet_recv;
  uint8_t last_buffer_sent[SIZE_PACKET];
} node;

void send_request(const uip_ipaddr_t *dest_ipaddr, uint8_t type, uint32_t value, void (*callback)(packet p));

node * get_nodes();
void listen(void (*callback)(const uip_ipaddr_t *sender_addr, packet p));
void connect_root(uip_ipaddr_t *sender_addr, void (*callback)(const uip_ipaddr_t *sender_addr, packet p));

#endif