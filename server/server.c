#include <stdbool.h>
#include <unistd.h>
#include "sock_lib.h"

#define SERV_ADDR "127.0.0.1"
#define SERV_PORT 22222
#define LISTEN_BACKLOG 10
#define MAX_CONNECTIONS 10

struct acceptedConnection {
    int clientSockFD;
    struct sockaddr_in addr;
    int error;
    bool accepted;
};

void startAcceptingConnections(int serverSockFD);
struct acceptedConnection* acceptIncomingConnections(int serverSockFD);
void recvAndPrintCreateSeparateThread(struct acceptedConnection *clientConnection);
void *recvAndPrint(void *clientSockFD);

void recvAndPrintCreateSeparateThread(struct acceptedConnection *clientConnection) {
    pthread_t thread;
    pthread_create(&thread, NULL, &recvAndPrint, (void *)(&clientConnection->clientSockFD));
}

void *recvAndPrint(void *clientSockFD) {
    char buf[4096];

    while (true) {
        ssize_t recvCount = recv(*(int *) clientSockFD, buf, sizeof(buf), 0);

        if (recvCount > 0) {
            buf[recvCount] = 0; // appending null byte to print
            printf("%s\n", buf);
        } else if (recvCount == 0) {
            break; // client has shutdown the connection
        } else {
            fprintf(stderr, "[E] Failure on receiving from client connection");
            break;
        }
    }
    
    puts("[Client Has Disconnected]");
    pthread_detach(pthread_self());
    return NULL;
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

struct acceptedConnection acceptedConnections[MAX_CONNECTIONS];
int acceptedConnectionCount = 0;

void startAcceptingConnections(int serverSockFD) {
    while (true) {
        struct acceptedConnection *clientConnection = acceptIncomingConnections(serverSockFD);
        if (acceptedConnectionCount <= MAX_CONNECTIONS) {
            acceptedConnections[acceptedConnectionCount++] = *clientConnection;

            recvAndPrintCreateSeparateThread(clientConnection);
        }
    }
}

int main() {
    int serverSockFD = CreateTcpIPv4Socket();
    if (serverSockFD < 0) {
        fprintf(stderr, "[E] Could not create IPv4 Socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in *serverAddress = CreateIPv4Address("" /* listen on all interfaces */, SERV_PORT);
    if (serverAddress == NULL) {
        fprintf(stderr, "[E] IPv4 Address creation failure");
        exit(EXIT_FAILURE);
    }

    int bound = bind(serverSockFD, (struct sockaddr*)serverAddress, sizeof(*serverAddress));
    if (bound == 0) {
        puts("Socket successfully bound");
    } else {
        fprintf(stderr, "[E] Socket could not be bound");
        free(serverAddress);
        close(serverSockFD);
        exit(EXIT_FAILURE);
    }

    int listenResult = listen(serverSockFD, LISTEN_BACKLOG);
    if (listenResult == 0) {
        startAcceptingConnections(serverSockFD);
    } else {
        fprintf(stderr, "[E] Error in listening for connections");
        free(serverAddress);
        close(serverSockFD);
        exit(EXIT_FAILURE);
    }


    free(serverAddress);
    shutdown(serverSockFD, SHUT_RDWR);
    close(serverSockFD);

    return 0;
}
