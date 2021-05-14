#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "sys/log.h"
#include "dev/leds.h"

#include "protocol.h"

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO


PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);

PROCESS_THREAD(udp_server_process, ev, data)
{
  PROCESS_BEGIN();

  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Accept UDP connection */
  void callback(packet p){
    if(p.status == OK){
      if(p.type == LAMP_ON){
        LOG_INFO("Led on\n");
      }
    }
  }
  listen(callback);
  PROCESS_END();
}