#include <stdbool.h>
#include <unistd.h>
#include "sock_lib.h"

#define SERV_ADDR "127.0.0.1"
#define SERV_PORT 22222
#define LISTEN_BACKLOG 10
#define MAX_CONNECTIONS 10
#define MAX_USERNAME_LEN 50
#define MAX_RECV_BUF_SIZE 4096

#define SERVER_TERM_STR "[Server has refused connection]"
#define DUPLICATE_USERNAME "[Username already exists]"

struct acceptedConnection {
    int clientSockFD;
    char name[MAX_USERNAME_LEN];
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
void sendRecvToAllClients(char *buf, struct acceptedConnection *clientConnection);
void sendInfoToClient(char *info, int clientSockFD);
bool recvAndSetUsername(struct acceptedConnection *clientConnection);

bool recvAndSetUsername(struct acceptedConnection *clientConnection) {
    char usernameBuf[MAX_USERNAME_LEN];
    ssize_t recvUsernameCount = recv(clientConnection->clientSockFD, usernameBuf, (MAX_USERNAME_LEN - 1), 0); // -1 to leave space for null byte
    if (recvUsernameCount > 0) {
        usernameBuf[((recvUsernameCount < MAX_USERNAME_LEN)? recvUsernameCount: (MAX_USERNAME_LEN - 1))] = 0; // set last byte to NULL
        for (int i = 0; i < acceptedConnectionCount; i++) {
            if (strncmp(acceptedConnections[i].name, usernameBuf, strlen(acceptedConnections[i].name)) == 0) {
                sendInfoToClient(DUPLICATE_USERNAME, clientConnection->clientSockFD);
                return false;
            }
        }
        snprintf(clientConnection->name, MAX_USERNAME_LEN, "%s", usernameBuf);
    } else {
        fprintf(stderr, "[E] Could not receive username from client\n");
    }
    return true;
}

void sendInfoToClient(char *info, int clientSockFD) {
    ssize_t sendInfoCount = send(clientSockFD, info, strlen(info), 0);
    if (sendInfoCount < 0) {
        fprintf(stderr, "[E] Could not sent error data to client\n");
    }
}

void sendRecvToAllClients(char *recvBuf, struct acceptedConnection *clientConnection) {
    char bufWithUsername [MAX_RECV_BUF_SIZE + MAX_USERNAME_LEN];
    snprintf(bufWithUsername, (MAX_RECV_BUF_SIZE + MAX_USERNAME_LEN), "%s: %s", clientConnection->name, recvBuf);

    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        // send received data to everyone except who sent it
        if (acceptedConnections[i].accepted == true && acceptedConnections[i].clientSockFD != clientConnection->clientSockFD) {
            ssize_t sentAmtCount = send(acceptedConnections[i].clientSockFD, bufWithUsername, strlen(bufWithUsername), 0);
            if (sentAmtCount < 0) {
                fprintf(stderr, "[E] Could not send to cliend with sockfd: %d\n", acceptedConnections[i].clientSockFD);
            }
        }
    }
}

void *recvAndPrint(void *clientConnection) {
    char recvBuf[MAX_RECV_BUF_SIZE];
    struct acceptedConnection *clientConnectionCasted = (struct acceptedConnection *)clientConnection;

    while (true) {
       ssize_t recvCount = recv(clientConnectionCasted->clientSockFD, recvBuf, MAX_RECV_BUF_SIZE - 1, 0); // -1 for null byte

        // skip if client send a newline
        recvBuf[strcspn(recvBuf, "\n")] = 0;
        if (recvBuf[0] == 0) {
            continue;
        }

        // skip if client sends all whitespace
        bool onlyWhitespace = true;
        for (int i = 0; recvBuf[i] != 0; i++) {
            if (!isspace((unsigned char) recvBuf[i])) {
                onlyWhitespace = false;
                break;
            }
        }
        if (onlyWhitespace) {
            continue;
        }

        if (recvCount > 0) {
            recvBuf[recvCount] = 0; // appending null byte to print
            printf("%s: %s\n", clientConnectionCasted->name, recvBuf);

            sendRecvToAllClients(recvBuf, clientConnectionCasted);
        } else if (recvCount == 0) {
            break; // client has shutdown the connection
        } else {
            fprintf(stderr, "[E] Failure on receiving from client connection\n");
            break;
        }
    }
    
    // make the current client connection attributes reusable
    clientConnectionCasted->accepted = false;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (acceptedConnections[i].clientSockFD == clientConnectionCasted->clientSockFD) {
            acceptedConnections[i].accepted = false;
            acceptedConnections[i].name[0] = 0; // reset name
        }
    }
    acceptedConnectionCount--;

    printf("[%s] has disconnected\n", clientConnectionCasted->name);
    sendRecvToAllClients("[has left the conversation]", clientConnectionCasted);
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
        if (acceptedConnectionCount < MAX_CONNECTIONS) {

            // if client sends a username that already exists, terminate connection
            if (!recvAndSetUsername(clientConnection)) {
                puts("[Client send duplicate username, Refused Connection]");
                continue;
            }

            // find place to put new client in the connections array
            // TODO
            // improve the below checking functionality
            for (int i = 0; i < MAX_CONNECTIONS; i++) {
                if (acceptedConnections[i].accepted == false) {
                    acceptedConnections[i] = *clientConnection;
                    acceptedConnectionCount++;

                    printf("[%s] has connected to the server\n", clientConnection->name);
                    sendRecvToAllClients("[has joined the conversation]", clientConnection);
                    break;
                }

            }

            recvAndPrintCreateSeparateThread(clientConnection);
        } else {
            puts("[Max Client Connection Reached, Refused Connection]");
            sendInfoToClient(SERVER_TERM_STR, clientConnection->clientSockFD);
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
