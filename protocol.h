#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "packet.h"

#define TRUE	1
#define FALSE	0

#define PERFECT_NETWORK TRUE

#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define RESEND_TM CLOCK_SECOND
#define TIMEOUT_COUNT 2

#define WAITING_REACHABLE 60*CLOCK_SECOND


typedef struct simple_udp_connection conn;

#define MAX_CONNECTIONS 10

#define GET 0xFFFFFFFF
#define SET 0xFFFFFFFE

enum NodeType {NOT_SET, LAMP=100, MOVEMENT_DETECTOR, BAROMETER, DOOR_LOCK};
enum PacketType{NODE_TYPE, ACK, NACK, LEDS_ON, LEDS_OFF, MOVEMENT_DETECTED, TEMPERATURE, PRESSURE, LOCK_, UNLOCK};

enum Color{RED, GREEN, BLUE};

typedef struct
{
  clock_time_t time_sent;
  struct ctimer timer;
  unsigned count_timeout;
  uint8_t data[SIZE_PACKET];
  void (*callback)(packet p);
  uint8_t running;
} request;

typedef struct 
{
  uint8_t connected;
  uint8_t type;
  conn connection;
  packet last_packet_recv;
  uint8_t last_buffer_sent[SIZE_PACKET];
  uip_ipaddr_t* ipaddr;
  uint32_t last_value;

  request req;
} node;

int send_request_to_node(unsigned index_node, uint8_t type, uint32_t value, void (*callback)(packet p));

int send_request_to_root(uint8_t type, uint32_t value, void (*callback)(packet p));


node * get_nodes();
void listen(uint32_t (*callback)(unsigned index_node, packet p));
void connect_root(uip_ipaddr_t *sender_addr, uint32_t node_type, uint32_t (*callback)(unsigned index_node, packet p));
int reach_root(uip_ipaddr_t * root_ipaddr);

#endif