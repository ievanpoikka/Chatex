#include <stdbool.h>
#include <unistd.h>
#include "sock_lib.h"

#define SERV_ADDR "127.0.0.1"
#define SERV_PORT 22222
#define TERM_STR "exit\n"

void readAndSendToServer(int sockfd);
void listenAndPrint(int sockfd);

void readAndSendToServer(int sockfd) {
    char *buf = NULL;
    size_t bufsize;
    while (true) {
        ssize_t readCount = getline(&buf, &bufsize, stdin);
        
        if (readCount > 0) {
            if (strncmp(buf, TERM_STR, strlen(TERM_STR)) == 0) {
                break;
            }

            ssize_t sentAmtCount = send(sockfd, buf, strlen(buf), 0);
            if (strlen(buf) > 0 && sentAmtCount == 0) {
                fprintf(stderr, "[E] No data sent");
            }
            if (sentAmtCount < 0) {
                fprintf(stderr, "[E] Could not send data");
            }
        }
    }
}

void listenAndPrint(int sockfd) {
    char buf[4096];
    ssize_t recvCount = recv(sockfd, buf, sizeof(buf), 0);

    if (recvCount > 0) {
        buf[recvCount] = 0; // appending null byte to print
        printf("%s\n", buf);
    } else if (recvCount < 0) {
        fprintf(stderr, "[E] Error in receiving data");
    }
    else {
        fprintf(stderr, "[I] No Data Received");
    }
}

int main() {
    int sockfd = CreateTcpIPv4Socket();
    if (sockfd < 0) {
        fprintf(stderr, "[E] Could not create IPv4 Socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in *address = CreateIPv4Address(SERV_ADDR, SERV_PORT);
    if (address == NULL) {
        fprintf(stderr, "[E] IPv4 Address creation failure");
        exit(EXIT_FAILURE);
    }

    short connectTry = 1;
    while (connectTry++ <= 3) {
        int connectionEstablished = connect(sockfd, (struct sockaddr*)address, sizeof(*address));
        if (connectionEstablished == 0) {
            puts("Connection successfully establised");
            break;
        } else {
            fprintf(stderr, "[E] Connection Failed, trying again");
        }
        sleep(1);
    }
    
    if (connectTry == 3) {
        fprintf(stderr, "[E] Failed to connect to remote server, fatal error");
        free(address);
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    readAndSendToServer(sockfd);

    free(address);
    close(sockfd);

    return 0;
}
