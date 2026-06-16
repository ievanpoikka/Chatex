#ifndef LIB_SOCK_LIB_H
#define LIB_SOCK_LIB_H

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

struct sockaddr_in* CreateIPv4Address(char *ip, int port);

int CreateTcpIPv4Socket();

#endif
