#include "contiki.h"
#include "net/routing/routing.h"
#include "net/ipv6/simple-udp.h"

#include "random.h"
#include "sys/log.h"
#include "packet.h"
#include "protocol.h"

#define LOG_MODULE "Protocol"
#define LOG_LEVEL LOG_LEVEL_INFO

static node nodes[MAX_CONNECTIONS];
static node *root_node = NULL;
static uint32_t (*action)(unsigned index_node, packet p);

void callback_timeout(void* current_node);

void send_req(node *current_node)
{
  LOG_INFO("Snd ");
  LOG_INFO_6ADDR(current_node->ipaddr);
  LOG_INFO_("\n");
  log_packet(parse_packet(current_node->req.data,SIZE_PACKET));

  simple_udp_sendto(&current_node->connection, current_node->req.data, SIZE_PACKET, current_node->ipaddr);
  ctimer_set(&current_node->req.timer, RESEND_TM, callback_timeout, (void*)current_node);
}

int get_index_node_from_address(const uip_ipaddr_t *sender_addr)
{
  if (root_node)
    return 0;

  // if we are the server, we didn't set the pointer root_node
  int index_first_empty = -1;
  for (int i = 0; i < MAX_CONNECTIONS; i++)
  {
    if (index_first_empty == -1 && !nodes[i].connected)
    {
      index_first_empty = i;
    }
    if (memcmp(sender_addr, nodes[i].ipaddr, sizeof(uip_ipaddr_t)) == 0)
    {
      return i;
    }
  }
  
  LOG_INFO("New node !\n");
  return index_first_empty;
}
  


void callback_receive(conn *connection,
                      const uip_ipaddr_t *sender_addr,
                      uint16_t sender_port,
                      const uip_ipaddr_t *receiver_addr,
                      uint16_t receiver_port,
                      const uint8_t *data,
                      uint16_t datalen)
{

  if (!PERFECT_NETWORK && random_rand() % 2 == 0)
  {
    LOG_INFO("Simulate loss\n");
    return;
  }

  uint8_t cpy_data[datalen];
  memcpy(cpy_data, data, datalen);

  if (!PERFECT_NETWORK && random_rand() % 3 == 0)
  {
    LOG_INFO("Simulate corruption\n");
    cpy_data[random_rand() % datalen] = (uint8_t)random_rand() % 0xFF;
  }

  packet recv_p = parse_packet(cpy_data, datalen);

  LOG_INFO("From ");
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");

  int index_node = get_index_node_from_address(sender_addr);
  if(index_node == -1){
    LOG_INFO("Cannot accept more connections\n");
    return;
  }


  node *current_node = &nodes[index_node];
  current_node->connected = 1;
  current_node->connection = *connection;
  current_node->connection.remote_addr = *sender_addr;

  LOG_INFO("Node %d of type %u\n",index_node,(unsigned)current_node->type);
  log_packet(recv_p);
  
  if (current_node->req.running)
  {
    request *req = &current_node->req;
    if (recv_p.status != OK)
    {
      LOG_INFO("Response error %u, resending the packet\n", (unsigned)recv_p.status);
      send_req(current_node);
      return;
    }

    if(recv_p.is_response){
      ctimer_stop(&req->timer);
      if (recv_p.type == NACK)
      {
        LOG_INFO("A NACK was sent back, resending the same packet\n");
        send_req(current_node);
        return;
      }
      LOG_INFO("Request answered. %u\n",(unsigned)recv_p.type);
      req->running = 0;
      if (req->callback)
        req->callback(recv_p);
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
    set_packet(current_node->last_buffer_sent, back_p);
    simple_udp_sendto(&current_node->connection, current_node->last_buffer_sent, SIZE_PACKET, sender_addr);
    return;
  }

  if(!recv_p.is_response){

    current_node->last_packet_recv = recv_p;

    // If packet received is valid and has never been received before
    if (first_time){
      if(recv_p.type == NODE_TYPE){
        current_node->type = recv_p.value;  
      }
      current_node->last_value = action(index_node, recv_p);
      LOG_INFO("Send rep...\n");
    }else{
      LOG_INFO("Packet previously received, re-rep...\n");
    }
    back_p.type = recv_p.type;

    back_p.value = current_node->last_value;

    set_packet(current_node->last_buffer_sent, back_p);
    LOG_INFO("To ");
    LOG_INFO_6ADDR(current_node->ipaddr);
    LOG_INFO_("\n");
    // log_packet(parse_packet(current_node->last_buffer_sent, SIZE_PACKET));
    simple_udp_sendto(&current_node->connection, current_node->last_buffer_sent, SIZE_PACKET, current_node->ipaddr);
  } else{
    // if it is a lost response
    if (current_node->req.callback)
        current_node->req.callback(recv_p);
  }
}

void callback_timeout(void* current_node)
{
  request * req = &((node*)current_node)->req;
  if (req->count_timeout >= TIMEOUT_COUNT)
  {
    LOG_INFO("REQUEST TIMEOUT\n");
    packet p;
    p.status = ERR_TIMEOUT;
    req->running = 0;
    if (req->callback)
      req->callback(p);
    return;
  }
  req->count_timeout++;
  LOG_INFO("Time done %u, resending...\n", (unsigned)(1000 * (clock_time() - req->time_sent) / CLOCK_SECOND));

  send_req(current_node);
}

int send_request_to_node(unsigned index_node, uint8_t type, uint32_t value, void (*callback)(packet p))
{
  if(index_node >= MAX_CONNECTIONS)
    return 0;
  request *req = &nodes[index_node].req;
  if(req->running)
    return 0;

  packet p;
  p.is_response = 0;
  p.type = type;
  p.value = value;
  p.random_id = 0;

  req->count_timeout = 0;
  req->time_sent = clock_time();
  req->callback = callback;

  set_packet(req->data, p);
  req->running = TRUE;

  send_req(&nodes[index_node]);
  return 1;
}


int send_request_to_root(uint8_t type, uint32_t value, void (*callback)(packet p))
{
  return send_request_to_node(0, type, value, callback);
}

//-------------------------------------------------------------------


node *get_nodes()
{
  return nodes;
}

void listen(uint32_t (*callback)(unsigned index_node, packet p))
{
  action = callback;
  for (int i = 0; i < MAX_CONNECTIONS; i++)
  {
    nodes[i].type = NOT_SET;
    nodes[i].connected = FALSE;
    nodes[i].req.running = FALSE;
    nodes[i].ipaddr = &nodes[i].connection.remote_addr;

    simple_udp_register(&nodes[i].connection, UDP_SERVER_PORT, NULL,
                        UDP_CLIENT_PORT, callback_receive);
  }
}

void connect_root(uip_ipaddr_t *sender_addr, uint32_t node_type, uint32_t (*callback)(unsigned index_node, packet p))
{
  action = callback;
  root_node = &nodes[0];
  root_node->req.running = FALSE;
  root_node->ipaddr = &root_node->connection.remote_addr;
  memcpy(root_node->ipaddr, sender_addr, sizeof(uip_ipaddr_t));
  simple_udp_register(&root_node->connection, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, callback_receive);

  send_request_to_root(NODE_TYPE, node_type, NULL);
}

int reach_root(uip_ipaddr_t * root_ipaddr){
    if(!NETSTACK_ROUTING.node_is_reachable()){
      LOG_INFO("Network not reachable yet\n");
      return 0;
    }
    else if(!NETSTACK_ROUTING.get_root_ipaddr(root_ipaddr)){
      LOG_INFO("Root not reachable yet\n");
      return 0;
    }
    return 1;
}