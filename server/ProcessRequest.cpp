//
// Created by mostafa on 18/11/2019.
//
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fstream>
#include <vector>
#include <mutex>
#include <cmath>

using namespace std;
void DieWithError(char *errorMessage);
#define  BUFFERSISE 10240
#define  MAXMETHODSIZE 10
#define  MAXPATHSIZE 100
#define  MAXTOKENSIZE 1024
#define  MAXTOKENS 100
#define  CHUNKSIZE 1
#define  MAXPENDING 5
int maxTimeOut = 64; // default is max value
int numberConnection =0;
mutex connectionsMutex;
void updateTimeOut(){ // calculate the time out
   int temp = MAXPENDING - numberConnection;
   maxTimeOut = pow(2.0 , (double) (temp + 2));
   printf("Max Time Out : %d\n",maxTimeOut);
}
void addConnection(){ // handle add connection and make time out calculations
    connectionsMutex.lock();
    numberConnection++;
    updateTimeOut();
    connectionsMutex.unlock();
}
void subConnection(){ // handle remove connection and  make time out calculation
    connectionsMutex.lock();
    numberConnection--;
    updateTimeOut();
    connectionsMutex.unlock();
}
void printBuffer(char  buffer[BUFFERSISE] , int totalSize){ // utility to print buffer
    for(int i=0;i<totalSize;i++){
        cout<<buffer[i];
    }
    cout<<"\n";
}
void storeRequest(char arr [BUFFERSISE] , int arrSize,string delimiter , char tokens[MAXTOKENS][MAXTOKENSIZE],int &tokensSize, int &dataSize){// parse the request into header lines and data
    string s(arr,arr+arrSize);
    int pos_start =0;
    int  pos =0;
    while((pos = s.find(delimiter,pos_start))!= -1){
        string token = s.substr(pos_start, pos-pos_start);
        pos_start = pos+delimiter.length();
        copy(token.begin(),token.end(),tokens[tokensSize]);
        tokens[tokensSize][token.size()] = '\0';
        //printf("Line %d : %s \n", tokensSize,tokens[tokensSize]); // debug
        tokensSize++;
    }
    dataSize = s.size()-pos_start;
    string token = s.substr(pos_start , s.size() - pos_start);
    copy(token.begin(),token.end(),tokens[tokensSize]);
    tokens[tokensSize][token.size()] = '\0';
    //printf("Line %d : %s \n", tokensSize,tokens[tokensSize]); //debug
    tokensSize++;
}
void getMethod(char method[] , char path[] , char requestLine[]){ // get the method and path from request line
    sscanf(requestLine,"%s %s",method ,path);
    //printf("method : %s\npath: %s\n", method ,path);
}
void sendAll(int socket , char * message , int totalSize){ // wrap the send socket method to send all provided data
    int totalSent = 0;
    while(totalSize > 0){
       int sendBytes =  send(socket, message +totalSent ,totalSize, 0);
        if(sendBytes < 0)
            DieWithError((char *) "send() failed");
        totalSent += sendBytes;
        totalSize -= sendBytes;
    }
    cout<<"Sent: \n";
    printBuffer(message,totalSent);
}
int checkStopRecieve(char rcvBuffer[BUFFERSISE] , int totalSize){ // check if we have reached a complete request
    string sBuffer(rcvBuffer , rcvBuffer + totalSize);
    int pos= sBuffer.find("\r\n\r\n");
    if(pos == -1) return 0; // not all header got yet
    char tokens[MAXTOKENS][MAXTOKENSIZE];
    int tokensSize =0;
    int dataSize =0;
    storeRequest(rcvBuffer,totalSize,"\r\n",tokens,tokensSize,dataSize);
    string method(tokens[0],tokens[0]+3);
    if(method == "GET"){
        //printf("Get Stop >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
        return  1;
    }
    for(int i=0;i<tokensSize;i++){
        string line(tokens[i],tokens[i]+strlen(tokens[i]));
        if(line.find("Content-Length:")!= -1){
            //printf(">>>>>>>>>>data size: %d\n",dataSize);
            int contentLength =0;
            char  consume[MAXTOKENSIZE];
            sscanf(tokens[i],"%s %d",consume,&contentLength);
            //printf(">>>>>>>>>>>consume: %s\n",consume);
            //printf(">>>>>>>>>>>contentLength: %d\n",contentLength);
            if(dataSize == contentLength){
                //printf("Stop >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\n");
                return 1;
            }else{
                return  0;
            }
        }
    }
    return 0; //not content length

}
int recvAll(int socket , char *rcvBuffer , int &checkClose){ // wrap the recieve method to receive a complete request
    struct timeval tv;
    connectionsMutex.lock();
    tv.tv_sec = maxTimeOut;
    connectionsMutex.unlock();
    tv.tv_usec = 0;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    int rcvMsgSize = 0;
    int totalSend =0;
   //while((rcvMsgSize = recv(socket, rcvBuffer + totalSend, BUFFERSISE-totalSend, 0)) > 0) {
    while((rcvMsgSize = recv(socket, rcvBuffer + totalSend, CHUNKSIZE, 0)) > 0) {
        totalSend += rcvMsgSize;
       // printf("recieved: %d\n", totalSend);
        if(checkStopRecieve(rcvBuffer,totalSend)==1){
            //printf("reach end>>>>>>>>>>\n");
            break;
        }
    }
   if(rcvMsgSize == 0) {
       checkClose =1;
   }
   if(rcvMsgSize < 0){
       checkClose =-1;
   }
    return totalSend;
}


void handleGet(char path[MAXPATHSIZE], int socket){// operate on a get request -> start fetch data , form the response send data back
    path = path+1;
    ifstream file(path, std::ios::binary | std::ios::ate);
    if(!file.fail()) {
        streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);
        vector<char> buffer(size);
        file.read(buffer.data(), size);
        string out = "HTTP/1.1 200 OK\r\nContent-Length: " + to_string(buffer.size()) +"\r\n\r\n";
        char message[out.size()+buffer.size()];
        copy(out.begin() , out.end() , message);
        for(int i=0;i<buffer.size();i++){
            message[out.size()+i] = buffer[i];
        }
        int totalSize = buffer.size()+out.size();
        sendAll(socket, message, totalSize);
    }else{
        char *notFound = (char *) "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        sendAll(socket, notFound, strlen(notFound));
    }
}

void handlePost(char path[MAXPATHSIZE], int socket , char tokens[MAXTOKENS][MAXTOKENSIZE] , int tokensSize, int dataSize) {// operate on a post request
    path = path +1;
    ofstream out(path);
    char * data = tokens[tokensSize-1];
    out.write(data,dataSize);
    out.close();
    char *updated = (char *) "HTTP/1.1 200 OK\r\nContent-Length: 20\r\n\r\nUpdated successfully";
    sendAll(socket, updated, strlen(updated));
}


void ProcessRequest(int socket,char * client){ // process the incoming requests
    char rcvBuffer[BUFFERSISE];
    int rcvMsgSize = 0;
    int checkClose = 0;
    while (true) {
        rcvMsgSize = recvAll(socket, rcvBuffer,checkClose);
        if(rcvMsgSize > 0) {
            //printf("%s", rcvBuffer);
            char method[MAXMETHODSIZE];
            char path[MAXPATHSIZE];
            char tokens[MAXTOKENS][MAXTOKENSIZE];
            int tokensSize = 0;
            int dataSize = 0;
            storeRequest(rcvBuffer,rcvMsgSize, "\r\n", tokens, tokensSize, dataSize);
            getMethod(method, path, tokens[0]);
            cout<<"Recieved: \n";
            printBuffer(rcvBuffer,rcvMsgSize);
            if (strcmp(method, (char *) "GET") == 0) {
                handleGet(path, socket);
            } else if(strcmp(method, (char *) "POST") == 0) {
                handlePost(path, socket, tokens, tokensSize, dataSize);
            }
        }
        if(checkClose == 1){
            printf("Closed>>>>>>>>>>>>>>> clientIp: %s\n",client);
            close(socket);
            subConnection();
            break;
        }
        if(checkClose == -1){
            printf("TimeOut CloseConnection>>>>>>>>>>> ClientIp: %s\n",client);
            close(socket);
            subConnection();
            break;
        }
    }
}

