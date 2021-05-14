#include "contiki.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"
#include "dev/leds.h"

#include "protocol.h"

#define LOG_MODULE "Lamp"
#define LOG_LEVEL LOG_LEVEL_INFO

#define TIME_ON 600 * CLOCK_SECOND

PROCESS(udp_node_process, "UDP Lamp");
AUTOSTART_PROCESSES(&udp_node_process);

static struct ctimer timer;

static void auto_shut();

static void turn(int on, int color){
  leds_mask_t led = LEDS_COLOUR_NONE;
  switch (color)
  {
  case RED:
    led = LEDS_RED;
    break;
  
  case GREEN:
    led = LEDS_GREEN;
    break;
  
  case BLUE:
    led = LEDS_YELLOW;
    break;
  
  default:
    return;
  }
  if(on){
    LOG_INFO("Lamp on\n");
    leds_on(led); 
    ctimer_set(&timer, TIME_ON, auto_shut, NULL);  
  }
  else{
    LOG_INFO("Lamp off\n");
    leds_off(led);
  }    
}

static void auto_shut(){
  turn(FALSE, RED);
  turn(FALSE, GREEN);
  turn(FALSE, BLUE);
}

static uint32_t callback(unsigned index_node, packet p)
{
  if (p.status == OK)
  {
    if(p.type == LAMP_ON)
      turn(TRUE, p.value);
    else if(p.type == LAMP_OFF)
      turn(FALSE, p.value);
  }
  return 0;

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
  connect_root(&root_ipaddr, LAMP, callback);

  PROCESS_END();
}
