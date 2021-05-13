#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"

#include "protocol.h"

#define LOG_MODULE "Lamp"
#define LOG_LEVEL LOG_LEVEL_INFO

#define SEND_INTERVAL (60 * CLOCK_SECOND)

PROCESS(udp_lamp_process, "UDP client");
AUTOSTART_PROCESSES(&udp_lamp_process);

static void callback(const uip_ipaddr_t *sender_addr, packet p){
  LOG_INFO("Action Callback of the lamp\n");
  if(p.status == OK){
    if(p.type == LEDS_ON){
      LOG_INFO("Led on\n");
    }
  }
}

PROCESS_THREAD(udp_lamp_process, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t root_ipaddr;

  PROCESS_BEGIN();

  int time_request = CLOCK_SECOND * 2;

  etimer_set(&periodic_timer, time_request);
  int reachable = 0;
  while (!reachable)
  {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    reachable = 1;
    if(!NETSTACK_ROUTING.node_is_reachable()){
      reachable = 0;
      LOG_INFO("Not reachable yet\n");
    }
    else if(!NETSTACK_ROUTING.get_root_ipaddr(&root_ipaddr)){
      reachable = 0;
      LOG_INFO("Didn't see the root yet\n");
    }

    etimer_set(&periodic_timer, time_request);
  }

  /* Initialize UDP connection with root*/
  connect_root(&root_ipaddr, callback);

  PROCESS_END();
}
