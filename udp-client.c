#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "sys/log.h"

#include "protocol.h"

#define LOG_MODULE "App"

#define SEND_INTERVAL		  (60 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;

PROCESS(udp_client_process, "UDP client");
AUTOSTART_PROCESSES(&udp_client_process);


/*---------------------------------------------------------------------------*/
static void udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{

  LOG_INFO("Packet received from ");
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");

  packet p = parse_packet(data, datalen);
  
  log_packet(p);
  
  if(!p.is_valid)
    LOG_INFO("Raw : \"%.*s\"\n", datalen, (char *) data);
}
/*---------------------------------------------------------------------------*/

PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();

  /* Initialize UDP connection */
  init_conn(&udp_conn, udp_rx_callback);

  int time_request = CLOCK_SECOND * 2;
  printf("Client %d s\n", (int)(time_request/CLOCK_SECOND));

  etimer_set(&periodic_timer, time_request);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));

    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      /* Send to DAG root */
      packet p;
      p.is_valid = 1;
      p.is_response = 0;
      p.type = 69;
      p.value = 420;
      send_packet(&udp_conn, &dest_ipaddr, p);
      LOG_INFO("Client sending :\n");
      log_packet(p);

    } else {
      LOG_INFO("Not reachable yet\n");
    }
    etimer_set(&periodic_timer, time_request);

    // /* Add some jitter */
    // etimer_set(&periodic_timer, SEND_INTERVAL
    //   - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
