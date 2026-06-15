#include <stdbool.h>
#include <unistd.h>
#include "sock_lib.h"

#define SERV_ADDR "127.0.0.1"
#define SERV_PORT 22222
#define LISTEN_BACKLOG 10

struct acceptedConnection {
    int clientSockFD;
    struct sockaddr_in addr;
    int error;
    bool accepted;
};

struct acceptedConnection* acceptIncomingConnections(int serverSockFD);
void recvAndPrint(int clientSockFD);

void recvAndPrint(int clientSockFD) {
    char buf[4096];

    while (true) {
        ssize_t recvCount = recv(clientSockFD, buf, sizeof(buf), 0);

        if (recvCount > 0) {
            buf[recvCount] = 0; // appending null byte to print
            printf("%s\n", buf);
        } else if (recvCount == 0) {
            break;
        } else {
            puts("[E] Failure on receiving from client connection");
            break;
        }
    }
}

struct acceptedConnection* acceptIncomingConnections(int serverSockFD) {
    struct sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    int clientSockFD = accept(serverSockFD, (struct sockaddr*) &clientAddr, (socklen_t *) &clientAddrSize);

    struct acceptedConnection *clientSocket = malloc(sizeof(*clientSocket));
    clientSocket->addr = clientAddr;
    clientSocket->clientSockFD = clientSockFD;
    clientSocket->accepted = (clientSockFD > 0);

    if (!clientSocket->accepted) {
        clientSocket->error = clientSockFD;
        // set error when clientSockFD is less than 0
    }

    return clientSocket;
}

int main() {
    int serverSockFD = CreateTcpIPv4Socket();
    if (serverSockFD < 0) {
        puts("[E] Could not create IPv4 Socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in *serverAddress = CreateIPv4Address("" /* listen on all interfaces */, SERV_PORT);
    if (serverAddress == NULL) {
        puts("[E] IPv4 Address creation failure");
        exit(EXIT_FAILURE);
    }

    int bound = bind(serverSockFD, (struct sockaddr*)serverAddress, sizeof(*serverAddress));
    if (bound == 0) {
        puts("Socket successfully bound");
    } else {
        puts("[E] Socket could not be bound");
        free(serverAddress);
        close(serverSockFD);
        exit(EXIT_FAILURE);
    }

    int listenResult = listen(serverSockFD, LISTEN_BACKLOG);
    if (listenResult == 0) {
        struct acceptedConnection *clientSocket = acceptIncomingConnections(serverSockFD);
        recvAndPrint(clientSocket->clientSockFD);
    } else {
        puts("[E] Error in listening for connections");
        free(serverAddress);
        close(serverSockFD);
        exit(EXIT_FAILURE);
    }


    free(serverAddress);
    close(serverSockFD);

    return 0;
}
