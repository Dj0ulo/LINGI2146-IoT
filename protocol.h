#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "defines.h"


void init_conn(struct simple_udp_connection* c, simple_udp_callback callback);

void set_hello_sentence(char *str, unsigned len, unsigned count);

void log_bytes_packet(const uint8_t* buffer, size_t len);
void log_packet(packet p);
packet parse_packet(const uint8_t *buffer, size_t len);
void send_request(const uip_ipaddr_t* dest_ipaddr, uint8_t type, uint32_t value);
void listen();
void connect();
#endif