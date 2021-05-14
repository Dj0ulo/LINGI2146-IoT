#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "sys/log.h"
#include "dev/leds.h"

#include "protocol.h"

#define LOG_MODULE "Root"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(udp_root_process, "Root UDP server");
AUTOSTART_PROCESSES(&udp_root_process);

static void handle_packet(packet p)
{
  node *nodes = get_nodes();
  if (p.type == TEMPERATURE)
  {
    LOG_INFO("The barometer said that there is a temperature of %d °C\n", (int)p.value);
      for (int i = 0; i < MAX_CONNECTIONS; i++)
        if (nodes[i].connected && nodes[i].type == DOOR_LOCK)
          send_request_to_node(i, (int)p.value < 0 ? LOCK_ : UNLOCK, SET, NULL);
  }
  else if (p.type == PRESSURE)
  {
    LOG_INFO("The barometer said that there is a pressure of %d hPa\n", (int)p.value);
      for (int i = 0; i < MAX_CONNECTIONS; i++)
        if (nodes[i].connected && nodes[i].type == DOOR_LOCK)
          send_request_to_node(i, (int)p.value < 1013 ? LOCK_ : UNLOCK, SET, NULL);
  }
  else if (p.type == LOCK_)
  {
    LOG_INFO("The lock is %s\n", p.value == LOCK_ ? "locked" : "unlocked");
  }
  else if (p.type == MOVEMENT_DETECTED)
  {
    LOG_INFO("Movement detected\n");
    for (int i = 0; i < MAX_CONNECTIONS; i++)
      if (nodes[i].connected && nodes[i].type == LAMP)
        send_request_to_node(i, LAMP_ON, GREEN, NULL);
  }
}

static uint32_t callback(unsigned index_node, packet p)
{
  node *nodes = get_nodes();
  if (p.type == NODE_TYPE)
  {
    LOG_INFO("Nodes [");
    for (int i = 0; i < MAX_CONNECTIONS; i++)
    {
      if (nodes[i].connected)
      {
        LOG_INFO_6ADDR(&nodes[i].connection.remote_addr);
        LOG_INFO_(" : %u, ", (unsigned)nodes[i].type);
      }
    }
    LOG_INFO_("]\n");

    if (p.value == BAROMETER)
    {
      send_request_to_node(index_node, TEMPERATURE, GET, handle_packet);
    }
    else if (p.value == DOOR_LOCK)
    {
      send_request_to_node(index_node, LOCK_, GET, handle_packet);
    }
  }
  else
  {
    handle_packet(p);
  }
  return 0;
}

PROCESS_THREAD(udp_root_process, ev, data)
{
  PROCESS_BEGIN();

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  listen(callback);

  PROCESS_END();
}