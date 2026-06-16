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

struct acceptedConnection acceptedConnections[MAX_CONNECTIONS];
int acceptedConnectionCount = 0;

void startAcceptingConnections(int serverSockFD);
struct acceptedConnection* acceptIncomingConnections(int serverSockFD);
void recvAndPrintCreateSeparateThread(struct acceptedConnection *clientConnection);
void *recvAndPrint(void *clientSockFD);
void sendRecvToAllClients(char *buf, int clientSockFD);
void sendInfoToClient(int clientSockFD);

void sendInfoToClient(int clientSockFD) {
    char *info = "[Server has refused connection]";
    ssize_t sendInfoCount = send(clientSockFD, info, strlen(info), 0);
    if (sendInfoCount < 0) {
        fprintf(stderr, "[E] Could not sent error data to client\n");
    }
}

void sendRecvToAllClients(char *buf, int clientSockFD) {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (acceptedConnections[i].accepted == true && acceptedConnections[i].clientSockFD != clientSockFD) {
            ssize_t sentAmtCount = send(acceptedConnections[i].clientSockFD, buf, strlen(buf), 0);
            if (sentAmtCount < 0) {
                fprintf(stderr, "[E] Could not send to cliend with sockfd: %d\n", acceptedConnections[i].clientSockFD);
            }
        }
    }
}

void *recvAndPrint(void *clientConnection) {
    char buf[4096];
    struct acceptedConnection *clientConnectionCasted = (struct acceptedConnection *)clientConnection;

    while (true) {
        ssize_t recvCount = recv(clientConnectionCasted->clientSockFD, buf, sizeof(buf), 0);

        if (recvCount > 0) {
            buf[recvCount] = 0; // appending null byte to print
            printf("%s\n", buf);

            sendRecvToAllClients(buf, clientConnectionCasted->clientSockFD);
        } else if (recvCount == 0) {
            break; // client has shutdown the connection
        } else {
            fprintf(stderr, "[E] Failure on receiving from client connection\n");
            break;
        }
    }
    
    // make the current client connection attributes reusable
    clientConnectionCasted->accepted = false;
    acceptedConnectionCount--;

    puts("[Client Has Disconnected]\n");
    return NULL;
}

void recvAndPrintCreateSeparateThread(struct acceptedConnection *clientConnection) {
    pthread_t thread;
    pthread_create(&thread, NULL, &recvAndPrint, (void *)clientConnection);
    pthread_detach(thread);
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


void startAcceptingConnections(int serverSockFD) {
    while (true) {
        struct acceptedConnection *clientConnection = acceptIncomingConnections(serverSockFD);
        if (acceptedConnectionCount <= MAX_CONNECTIONS) {
            // TODO: fix the acceptedConnectionCount thingy
            // acceptedConnectionCount only increases(when connections add)
            // but doesn't decrease when connections leave
            // POSSIBLE FIX: make acceptedConnections a circular array
            for (int i = 0; i < MAX_CONNECTIONS; i++) {
                if (acceptedConnections[i].accepted == false) {
                    acceptedConnections[i] = *clientConnection;
                    acceptedConnectionCount++;
                }
            }

            recvAndPrintCreateSeparateThread(clientConnection);
        } else {
            puts("[Max Client Connection Reached, Refused Connection]");
            sendInfoToClient(clientConnection->clientSockFD);
        }
    }
}

int main() {
    int serverSockFD = CreateTcpIPv4Socket();
    if (serverSockFD < 0) {
        fprintf(stderr, "[E] Could not create IPv4 Socket\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in *serverAddress = CreateIPv4Address("" /* listen on all interfaces */, SERV_PORT);
    if (serverAddress == NULL) {
        fprintf(stderr, "[E] IPv4 Address creation failure\n");
        exit(EXIT_FAILURE);
    }

    int bound = bind(serverSockFD, (struct sockaddr*)serverAddress, sizeof(*serverAddress));
    if (bound == 0) {
        puts("Socket successfully bound");
    } else {
        fprintf(stderr, "[E] Socket could not be bound\n");
        free(serverAddress);
        close(serverSockFD);
        exit(EXIT_FAILURE);
    }

    int listenResult = listen(serverSockFD, LISTEN_BACKLOG);
    if (listenResult == 0) {
        startAcceptingConnections(serverSockFD);
    } else {
        fprintf(stderr, "[E] Error in listening for connections\n");
        free(serverAddress);
        close(serverSockFD);
        exit(EXIT_FAILURE);
    }


    free(serverAddress);
    shutdown(serverSockFD, SHUT_RDWR);
    close(serverSockFD);

    return 0;
}
