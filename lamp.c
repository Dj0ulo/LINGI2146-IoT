#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "dev/leds.h"

#include "protocol.h"

#define LOG_MODULE "Lamp"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(udp_lamp_process, "UDP Lamp");
AUTOSTART_PROCESSES(&udp_lamp_process);

static void callback(unsigned index_node, packet p)
{
  LOG_INFO("Action Callback of the lamp\n");
  if (p.status == OK)
  {
    leds_mask_t color = LEDS_COLOUR_NONE;
    if(p.value == GREEN)
      color = LEDS_GREEN;
    else if(p.value == BLUE)
      color = LEDS_BLUE;
    else if(p.value == RED)
      color = LEDS_RED;

    if (p.type == LEDS_ON)
    {
      LOG_INFO("Led on\n");
      leds_on(color);
    }
    else if (p.type == LEDS_OFF)
    {
      LOG_INFO("Led off\n");
      leds_off(color);
    }
  }
}

PROCESS_THREAD(udp_lamp_process, ev, data)
{
  PROCESS_BEGIN();

  static uip_ipaddr_t root_ipaddr;
  static struct etimer periodic_timer;

  do
  {
    etimer_set(&periodic_timer, 2 * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
  } while (!reach_root(&root_ipaddr));

  /* Initialize UDP connection with root*/
  connect_root(&root_ipaddr, LAMP, callback);

  etimer_set(&periodic_timer, 2 * CLOCK_SECOND);
  PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

  // send_request_to_root(67,78,NULL);

  PROCESS_END();
}
