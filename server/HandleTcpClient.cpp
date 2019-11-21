//
// Created by mostafa on 17/11/2019.
//
#include <stdio.h> /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>
#include <cstring>

#define RCVBUFSIZE 32
void DieWithError(char *errorMessage);
void HandleTCPClient(int clntSocket){
    /* Error handling function */
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;
    /* Buffer for echo string */
    /* Size of received message */
    /* Receive message from client */
    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError((char *)"recv() failed") ;
    printf("%s\n", echoBuffer);
    /* Send received string and receive again until end of transmission */
    while (recvMsgSize > 0) /* zero indicates end of transmission */
    {
    /* Echo message back to client */
        if (send(clntSocket, echoBuffer, recvMsgSize, 0) != recvMsgSize)
            DieWithError((char *)"send() failed");
    /* See if there is more data to receive */
        if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
            DieWithError((char *)"recv() failed");
    }
    close(clntSocket);


//    char *message = (char *) "HTTP/1.1 200 OK\r\nContent-Length: 43\r\nContent-Type: text/html\r\n\r\n<html><body><h1>It works!</h1></body></html>";
//    if (send(clntSocket, message, strlen(message), 0) != strlen(message))
//        DieWithError((char *) "send() failed");
//        while ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) != 0) {
//            if (recvMsgSize < 0)
//                DieWithError((char *) "recv() failed");
//            printf("%s\n", echoBuffer);
//        }
//    close(clntSocket);
    }
