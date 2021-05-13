#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "sys/log.h"
#include "dev/leds.h"

#include "protocol.h"


PROCESS(udp_root_process, "Root UDP server");
AUTOSTART_PROCESSES(&udp_root_process);

static void callback(const uip_ipaddr_t *sender_addr, packet p){
  LOG_INFO("Action Callback of the root!\n");

}

PROCESS_THREAD(udp_root_process, ev, data)
{
  PROCESS_BEGIN();

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();


  listen(callback);


  PROCESS_END();
}