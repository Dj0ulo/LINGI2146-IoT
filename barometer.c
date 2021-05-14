#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "random.h"

#include "protocol.h"

#define LOG_MODULE "Barometer"
#define LOG_LEVEL LOG_LEVEL_INFO

PROCESS(udp_node_process, "UDP Barometer");
AUTOSTART_PROCESSES(&udp_node_process);

static int get(uint8_t type)
{
  if (type == TEMPERATURE)
    return random_rand() % 60 - 20;
  else if (type == PRESSURE)
    return random_rand() % 200 + 1013;
  return 0;
}

static uint32_t callback(unsigned index_node, packet p)
{
  int res = get(p.type);
  if (p.status == OK && p.value == GET)
  {
    if (p.type == TEMPERATURE)
      LOG_INFO("Temperature : %d °C\n", res);
    else if (p.type == PRESSURE)
      LOG_INFO("Pressure : %d hPa\n", res);
  }
  return res;
}

PROCESS_THREAD(udp_node_process, ev, data)
{
  PROCESS_BEGIN();

  static uip_ipaddr_t root_ipaddr;
  static struct etimer periodic_timer;

  do
  {
    etimer_set(&periodic_timer, WAITING_REACHABLE);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
  } while (!reach_root(&root_ipaddr));

  /* Initialize UDP connection with root*/
  connect_root(&root_ipaddr, BAROMETER, callback);

  static int temp = 0;
  while (TRUE)
  {
    etimer_set(&periodic_timer, 10 * 60 * CLOCK_SECOND);
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    if(temp%2)
      send_request_to_root(TEMPERATURE, get(TEMPERATURE), NULL);
    else
      send_request_to_root(PRESSURE, get(PRESSURE), NULL);
    temp += 1;
  }

  PROCESS_END();
}
