#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "dev/button-sensor.h"

#include "protocol.h"

#define LOG_MODULE "Detector"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(udp_node_process, "UDP Movement detector");
AUTOSTART_PROCESSES(&udp_node_process);


PROCESS_THREAD(udp_node_process, ev, data)
{
  PROCESS_BEGIN();

  SENSORS_ACTIVATE(button_sensor);

  static uip_ipaddr_t root_ipaddr;
  static struct etimer periodic_timer;

  do
  {
    etimer_set(&periodic_timer, 2 * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
  } while (!reach_root(&root_ipaddr));

  /* Initialize UDP connection with root*/
  connect_root(&root_ipaddr, MOVEMENT_DETECTOR, NULL);

  while (1)
  {
    PROCESS_WAIT_EVENT(); // Waiting for a event, don't care which
    if (ev == sensors_event)
    { // If the event it's provoked by the user button, then...
      if (data == &button_sensor)
      {
        LOG_INFO("Movement detected\n");
        send_request_to_root(MOVEMENT_DETECTED, TRUE, NULL);
      }
    }
  }

  PROCESS_END();
}
