#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"


#include "sys/log.h"
#include "defines.h"

void init_conn(struct simple_udp_connection* c, simple_udp_callback callback){
  simple_udp_register(c, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, callback);
}

void log_bytes_packet(const uint8_t* buffer, size_t len){
  int i=0;
  LOG_INFO("Bytes (%u) : ", (unsigned)len);
  for(i=0;i<len;i++)
    LOG_INFO_("%u ",(unsigned)buffer[i]);
  LOG_INFO_("\n");
}

void log_packet(packet p){
  if(!p.is_valid){
    LOG_INFO("Packet : [UNVALID]\n");
    return;
  }
  LOG_INFO("Packet : %s %u : %u\n", p.is_response ? "response " : "request ",(unsigned)p.type, (unsigned)p.value);
}

void set_packet(uint8_t *buffer_ptr, packet params){
  unsigned off = 0;
  memset(buffer_ptr, START_PACKET, 1);
  off += 1;
  memcpy(buffer_ptr+off, &params.is_valid, 1);
  off += 1;
  memcpy(buffer_ptr+off, &params.is_response, 1);
  off += 1;
  memcpy(buffer_ptr+off, &params.type, 1);
  off += 1;
  memcpy(buffer_ptr+off, &params.value, 4);
  off += 4;
  memset(buffer_ptr+off, END_PACKET, 1);
}

packet parse_packet(const uint8_t *buffer, size_t len){
  // uint8_t buffer[SIZE_PACKET] = buffer_ptr;
  packet params;

  if (len != SIZE_PACKET || buffer[0] != START_PACKET || buffer[SIZE_PACKET-1] != END_PACKET){
    params.is_valid = 0;
    return params;
  }

  params.is_valid = buffer[1]; 
  params.is_response = buffer[2]; 
  params.type = buffer[3]; 
  memcpy(&params.value, buffer+4, 4);

  return params;
}

void send_packet(struct simple_udp_connection* c, const uip_ipaddr_t* dest_ipaddr, packet p){
  uint8_t buffer[SIZE_PACKET];
  set_packet(buffer, p);
  simple_udp_sendto(c, buffer, SIZE_PACKET, dest_ipaddr);
}

void set_hello_sentence(char *str, unsigned len, unsigned count){
  if (count == 0){
    snprintf(str, len, "Hello !");
  }else if(count == 1){
    snprintf(str, len, "Hello (second time...)");
  }else if(count == 2){
    snprintf(str, len, "Hello (third time...)");
  }else{
    snprintf(str, len, "Hello (%dth time) :-(",count);
  }
}








