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

static void callback(const uip_ipaddr_t *sender_addr, packet p){
  node *nodes = get_nodes();
  LOG_INFO("Nodes [");
  for(int i=0;i<MAX_CONNECTIONS;i++){
    if(nodes[i].connected){
      LOG_INFO_6ADDR(&nodes[i].connection.remote_addr);
      LOG_INFO_(" : %u, ",(unsigned)nodes[i].type);
    }

  }
  LOG_INFO_("]\n");

}

PROCESS_THREAD(udp_root_process, ev, data)
{
  PROCESS_BEGIN();

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  listen(callback);

  PROCESS_END();
}