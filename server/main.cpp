#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <thread>

using namespace std;
#define MAXPENDING 5
void DieWithError(char *errorMessage);
void addConnection();
void ProcessRequest(int socket,char * clientIp);
void runSocket(int socket , char * clientIp){
    ProcessRequest (socket , clientIp) ;
}
int main(int argc , char **argv) {
    int servSock;
    int clntSock;
    struct sockaddr_in servAddr;
    struct sockaddr_in clntAddr;
    unsigned short echoServPort = atoi(argv[1]);
    unsigned int clntLen;

    if ((servSock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError( (char *)"socket () failed") ;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servAddr.sin_port = htons(echoServPort);
    if (bind(servSock, (struct sockaddr *)&servAddr, sizeof(servAddr)) < 0)
        DieWithError ( (char *)"bind () failed");

    if (listen(servSock, MAXPENDING) < 0)
        DieWithError((char *)"listen() failed") ;

    while(true)
    {
        clntLen = sizeof(clntAddr);
        if ((clntSock = accept(servSock, (struct sockaddr *) &clntAddr, &clntLen)) < 0)
            DieWithError((char *)"accept() failed");
          printf("Handling client %s\n", inet_ntoa(clntAddr.sin_addr));
//        ProcessRequest (clntSock,inet_ntoa(clntAddr.sin_addr)) ;
        addConnection();
        thread t(runSocket, clntSock, inet_ntoa(clntAddr.sin_addr));
        t.detach();
    }
    return 0;
}