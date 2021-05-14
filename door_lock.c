#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "dev/button-sensor.h"
#include "dev/leds.h"
#include "random.h"


#include "protocol.h"

#define LOG_MODULE "Lock"
#define LOG_LEVEL LOG_LEVEL_INFO

#define DURATION_LOCK 5*CLOCK_SECOND

PROCESS(udp_node_process, "UDP Door lock");
AUTOSTART_PROCESSES(&udp_node_process);

static uint32_t is_lock = UNLOCK;
static struct ctimer timer;

static void send_state(){
  if(is_lock)
    leds_on(LEDS_GREEN);
  else
    leds_off(LEDS_GREEN);
  send_request_to_root(LOCK_, is_lock,NULL);
}

static uint32_t callback(unsigned index_node, packet p)
{
  if (p.status != OK)
    return 0;
  

  if (p.type == LOCK_){
    if(p.value == GET){
      LOG_INFO("State asked\n");
      return is_lock;
    }
    LOG_INFO("Locking...\n");
    is_lock = LOCK_;
    ctimer_set(&timer, DURATION_LOCK, send_state, NULL);
  }
  else if (p.type == UNLOCK){
    LOG_INFO("Unlocking...\n");
    is_lock = UNLOCK;

    ctimer_set(&timer, DURATION_LOCK, send_state, NULL);
  }

  return 0;
}

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
  connect_root(&root_ipaddr, DOOR_LOCK, callback);


  PROCESS_END();
}
