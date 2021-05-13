#include "net/routing/routing.h"
#include "net/ipv6/simple-udp.h"

#include "random.h"
#include "sys/log.h"
#include "packet.h"
#include "protocol.h"

typedef struct
{
  conn *connection;
  clock_time_t time_sent;
  struct ctimer timer;
  unsigned count_timeout;
  uint8_t last_buffer_sent[SIZE_PACKET];
  uint16_t last_id;
  uip_ipaddr_t dest_ipaddr;
  void (*callback)(packet p);
  uint8_t running;
} request_infos;

static request_infos req;

static node nodes[MAX_CONNECTIONS];
static node *root_node = NULL;
static void (*action)(const uip_ipaddr_t *sender_addr, packet p);

void callback_timeout();

void send_req()
{
  LOG_INFO("Packet (%u)\n", (unsigned)parse_packet(req.last_buffer_sent, SIZE_PACKET).random_id);

  simple_udp_sendto(req.connection, req.last_buffer_sent, SIZE_PACKET, &req.dest_ipaddr);
  ctimer_set(&req.timer, RESEND_TM, callback_timeout, NULL);
}

node *get_node_from_address(const uip_ipaddr_t *sender_addr)
{
  node *current_node = NULL;

  if (!root_node)
  {
    // if we are the server, we didn't set the pointer root_node
    int index_first_empty = -1;
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
      if (index_first_empty == -1 && !nodes[i].connected)
      {
        index_first_empty = i;
      }
      if (memcmp(sender_addr, &nodes[i].connection.remote_addr, sizeof(uip_ipaddr_t)) == 0)
      {
        current_node = &nodes[i];
        break;
      }
    }
    if (current_node == NULL)
    {
      if (index_first_empty == -1)
      {
        LOG_INFO("Cannot accept more connections");
        return NULL;
      }
      current_node = &nodes[index_first_empty];
      memcpy(&current_node->connection.remote_addr, sender_addr, sizeof(uip_ipaddr_t));
    }
  }
  else
  {
    current_node = root_node;
  }
  return current_node;
}

void callback_receive(conn *c,
                      const uip_ipaddr_t *sender_addr,
                      uint16_t sender_port,
                      const uip_ipaddr_t *receiver_addr,
                      uint16_t receiver_port,
                      const uint8_t *data,
                      uint16_t datalen)
{

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

  LOG_INFO("Packet received from ");
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");
  
  log_packet(recv_p);


  node *current_node = get_node_from_address(sender_addr);

  if (req.running)
  {
    if (recv_p.status != OK)
    {
      LOG_INFO("There was an error %u, resending the same packet\n", (unsigned)recv_p.status);
      send_req();
      return;
    }

    if(recv_p.is_response){
      ctimer_stop(&req.timer);
      if (recv_p.type == NACK)
      {
        LOG_INFO("A NACK was sent back, resending the same packet\n");
        send_req();
        return;
      }
      LOG_INFO("Responded\n");
      req.running = 0;
      if (req.callback)
        req.callback(recv_p);
      return;
    }

  }

  // is probably a request
  int first_time = current_node->last_packet_recv.crc != recv_p.crc;

  packet back_p;
  back_p.is_response = 1;
  back_p.random_id = recv_p.random_id;

  if (recv_p.status != OK)
  {
    LOG_INFO("Error %u, sending NACK.\n", (unsigned)recv_p.status);
    back_p.type = NACK;
    back_p.value = recv_p.status;
  }
  else
  {
    LOG_INFO("Sending acknowledgement.\n");
    current_node->last_packet_recv = recv_p;

    // If packet received is valid and has never been received before
    if (first_time)
      action(sender_addr, recv_p);

    back_p.type = ACK;
    back_p.value = 0;
  }
  set_packet(current_node->last_buffer_sent, back_p);
  simple_udp_sendto(&current_node->connection, current_node->last_buffer_sent, SIZE_PACKET, sender_addr);
  
}

void callback_timeout()
{
  if (req.count_timeout >= TIMEOUT_COUNT)
  {
    LOG_INFO("REQUEST TIMEOUT\n");
    packet p;
    p.status = ERR_TIMEOUT;
    req.running = 0;
    if (req.callback)
      req.callback(p);
    return;
  }
  req.count_timeout++;
  LOG_INFO("Time done %u, resending...\n", (unsigned)(1000 * (clock_time() - req.time_sent) / CLOCK_SECOND));

  send_req();
}

void send_request(const uip_ipaddr_t *dest_ipaddr, uint8_t type, uint32_t value, void (*callback)(packet p))
{
  packet p;
  p.is_response = 0;
  p.type = type;
  p.value = value;
  p.random_id = 0;

  req.dest_ipaddr = *dest_ipaddr;
  req.count_timeout = 0;
  req.time_sent = clock_time();
  req.callback = callback;

  set_packet(req.last_buffer_sent, p);
  req.last_id = p.random_id;
  req.running = 1;
  send_req();
}

//-------------------------------------------------------------------


node *get_nodes()
{
  return nodes;
}

void listen(void (*callback)(const uip_ipaddr_t *sender_addr, packet p))
{
  req.running = 0;
  action = callback;
  for (int i = 0; i < MAX_CONNECTIONS; i++)
  {
    nodes[i].type = NOT_SET;
    nodes[i].connected = 0;
    simple_udp_register(&nodes[i].connection, UDP_SERVER_PORT, NULL,
                        UDP_CLIENT_PORT, callback_receive);
  }
}

void connect_root(uip_ipaddr_t *sender_addr, void (*callback)(const uip_ipaddr_t *sender_addr, packet p))
{
  req.running = 0;
  action = callback;
  root_node = &nodes[0];
  simple_udp_register(&root_node->connection, UDP_CLIENT_PORT, sender_addr,
                      UDP_SERVER_PORT, callback_receive);

  req.connection = &root_node->connection;
  send_request(sender_addr, 42, 69, NULL);
}