#include "sock_lib.h"

int CreateTcpIPv4Socket() {
    return socket(AF_INET, SOCK_STREAM, 0);
}

struct sockaddr_in* CreateIPv4Address(char *ip, int port) {
    struct sockaddr_in *address = malloc(sizeof(*address));

    if (address == NULL) {
        puts("[E] address struct memory allocation failed");
        exit(EXIT_FAILURE);
    }

    address->sin_family = AF_INET;
    address->sin_port = htons(port);

    if (strlen(ip) == 0) {
        address->sin_addr.s_addr = INADDR_ANY;
    } else {
        inet_pton(AF_INET, ip, &address->sin_addr.s_addr);
    }
    
    return address;
}
