#include <stdbool.h>
#include <unistd.h>
#include "sock_lib.h"

#define SERV_ADDR "127.0.0.1"
#define SERV_PORT 22222
#define TERM_STR "exit\n"
#define SERVER_TERM_STR "[Server has refused connection]" 
#define MAX_USERNAME_LEN 50
#define DUPLICATE_USERNAME "[Username already exists]"

void readAndSendToServer(int sockfd);
void listenAndPrintCreateThread(int sockfd);
void *listenAndPrint(void *sockfd);

void listenAndPrintCreateThread(int sockfd) {
    pthread_t thread;
    pthread_create(&thread, NULL, &listenAndPrint, (void *) &sockfd);
    pthread_detach(thread);
}

void *listenAndPrint(void *sockfd) {
    char buf[4096];
    int sockfd_int = *(int *) sockfd;

    while (true) {
        ssize_t recvCount = recv(sockfd_int, buf, sizeof(buf) - 1, 0);

        if (recvCount > 0) {
            buf[recvCount] = 0; // appending null byte to print
            printf("\n%s\n", buf);

            if ((strncmp(buf, SERVER_TERM_STR, strlen(SERVER_TERM_STR))) == 0
             || (strncmp(buf, DUPLICATE_USERNAME, strlen(DUPLICATE_USERNAME)) == 0)
            ) {
                puts("[Client Shutting Down...]\nPress Enter to exit...");
                write(STDIN_FILENO, "\n", 1);
                close(sockfd_int);
                close(STDIN_FILENO);
                exit(EXIT_FAILURE);
            }
        } else if (recvCount == 0) {
            break; // server has refused/shutdown the connection?
        } else {
            fprintf(stderr, "[E] Failure on receiving from server\n");
            break;
        }
    }
    
    puts("[Disconnected From Server]\nPress Enter to exit...");
    return NULL;
}

void readAndSendToServer(int sockfd) {
    char *name = NULL;
    size_t nameSize = 0;
    int nameLen = 0;

    do { 
        printf("Please enter your name: ");
        ssize_t nameReadCount = getline(&name, &nameSize, stdin);
        name[nameReadCount - 1] = 0;
        nameLen = strlen(name);
        if (nameLen <= 1 || nameLen >= MAX_USERNAME_LEN) {
            puts("Invalid: Name length should be greater than 1 and less then 50");
            continue;
        }
    } while (nameLen <= 1 || nameLen >= MAX_USERNAME_LEN); 

    ssize_t nameSentCount = send(sockfd, name, nameLen, 0);
    if (nameLen > 0 && nameSentCount == 0) {
        fprintf(stderr, "[E] Username was not sent, connection lost\n");
    }
    if (nameSentCount < 0) {
        fprintf(stderr, "[E] Could not Username\n");
    }


    char *buf = NULL;
    size_t bufsize;
    while (true) {
        printf("You [%s]: ", name);
        ssize_t readCount = getline(&buf, &bufsize, stdin);
        
        if (readCount > 0) {
            if (strncmp(buf, TERM_STR, strlen(TERM_STR)) == 0) {
                break;
            }

            ssize_t sentAmtCount = send(sockfd, buf, strlen(buf), 0);
            if (strlen(buf) > 0 && sentAmtCount == 0) {
                fprintf(stderr, "[E] No data sent\n");
            }
            if (sentAmtCount < 0) {
                fprintf(stderr, "[E] Could not send data\n");
            }
        }
    }

    free(name);
    free(buf);
}

int main() {
    int sockfd = CreateTcpIPv4Socket();
    if (sockfd < 0) {
        fprintf(stderr, "[E] Could not create IPv4 Socket\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in *address = CreateIPv4Address(SERV_ADDR, SERV_PORT);
    if (address == NULL) {
        fprintf(stderr, "[E] IPv4 Address creation failure\n");
        exit(EXIT_FAILURE);
    }

    short connectTry = 1;
    while (connectTry++ <= 3) {
        int connectionEstablished = connect(sockfd, (struct sockaddr*)address, sizeof(*address));
        if (connectionEstablished == 0) {
            puts("Connection successfully establised");
            break;
        } else {
            fprintf(stderr, "[E] Connection Failed, trying again\n");
        }
        sleep(1);
    }
    
    if (connectTry == 3) {
        fprintf(stderr, "[E] Failed to connect to remote server, fatal error\n");
        free(address);
        close(sockfd);
        exit(EXIT_FAILURE);
        return 1;
    }

    listenAndPrintCreateThread(sockfd);
    readAndSendToServer(sockfd);

    free(address);
    close(sockfd);

    return 0;
}
