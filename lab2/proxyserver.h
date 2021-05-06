#ifndef PROXYSERVER_H_INCLUDE
#define PROXYSERVER_H_INCLUDE

#include "httpserver.h"
extern char * UpstreamIP;
extern int UpstreamPort;
extern int epfd;

extern void proxyStart();
#endif