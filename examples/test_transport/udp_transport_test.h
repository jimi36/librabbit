#ifndef udp_server_h
#define udp_server_h

#include <stdio.h>

#include <pump/time.h>
#include <pump/service.h>
#include <pump/transport.h>

using namespace pump;

extern void start_udp_server(const std::string &ip, uint16 port);

extern void start_udp_client(const std::string &ip, uint16 port);

#endif