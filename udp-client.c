#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "dev/button-sensor.h"


#include "protocol.h"

#define LOG_MODULE "App"

#define SEND_INTERVAL (60 * CLOCK_SECOND)

PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);

PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();
  SENSORS_ACTIVATE(button_sensor);

  /* Initialize UDP connection */
  connect();

  int time_request = CLOCK_SECOND * 2;

  etimer_set(&periodic_timer, time_request);
  while (!(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)))
  {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    LOG_INFO("Not reachable yet\n");
    etimer_set(&periodic_timer, time_request);
  }
  while (1)
  {
    PROCESS_WAIT_EVENT(); // Waiting for a event, don't care which

    if (ev == sensors_event)
    { // If the event it's provoked by the user button, then...
      if (data == &button_sensor)
      {
        if (NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr))
        {
          /* Send to DAG root */
          LOG_INFO("Sending request...\n");

          void callback(packet p){
            LOG_INFO("cool\n");
            log_packet(p);
          }
          send_request(&dest_ipaddr, LEDS_ON, GREEN, callback);
        }
      }
    }
  }

  PROCESS_END();
}
