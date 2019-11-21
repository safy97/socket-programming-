#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for socket(), connect(), send(), and recv() */
#include <arpa/inet.h> /* for sockaddr_in and inet_addr() */
#include <stdlib.h> /* for atoi() */
#include <string.h> /* for memset() */
#include <unistd.h>
#include <string>
#include <iostream>

using namespace std;
#define RCVBUFSIZE 300
void DieWithError(char *errorMessage);
void ProcessRequest(int socket , char * servIp);
int main(int argc , char ** argv) {
    int sock;
    struct sockaddr_in serverAddr;
    printf("server ip: %s\nport number: %s\n",argv[1],argv[2]);
    unsigned short ServerPort=  atoi(argv[2]) ;
    char *servIP = (char *)argv[1];
    char sendBuffer[RCVBUFSIZE];
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(servIP);
    serverAddr.sin_port = htons(ServerPort);
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError((char *)" socket () failed1") ;

    if (connect(sock, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
        DieWithError((char *)" connect () failed2") ;
    ProcessRequest(sock, servIP);
    close(sock);
    return 0;
}