//
// Created by mostafa on 19/11/2019.
//
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <iostream>

using namespace std;
#define  MAXREQUESTS 1000
#define MAXREQUESTSIZE 250
#define  MAXMETHODSIZE 100
#define  MAXPATHSIZE 100
#define  BUFFERSISE 10240
#define  MAXTOKENSIZE 1024
#define  MAXTOKENS 100
#define  CHUNKSIZE 1
void DieWithError(char * msg);
void printBuffer(char  buffer[BUFFERSISE] , int totalSize){// print the buffer
    for(int i=0;i<totalSize;i++){
        cout<<buffer[i];
    }
    cout<<"\n";
}
void importRequests(char requests[MAXREQUESTS][MAXREQUESTSIZE] , int &totalSize){ // import requests from the file
    ifstream in("requests.txt");
    if (!in){
        DieWithError((char *) "can't open file");
    }
    string temp;
    while (getline(in, temp)) {
        copy(temp.begin(),temp.end(),requests[totalSize]);
        requests[totalSize][temp.size()] = '\0';
        totalSize++;
        printf("request %d: %s\n",totalSize,requests[totalSize-1]);
    }

}
void parseRequest(char method[] , char path[] , char requestLine[]){ // parse a request to get the method and path
    char local[MAXPATHSIZE];
    char host[MAXPATHSIZE];
    sscanf(requestLine,"%s %s %s %s",method ,path,local,host);
    //printf("method: %s\npath: %s\n", method,path);
}
void storeRequest(char arr [BUFFERSISE] , int arrSize ,string delimiter , char tokens[MAXTOKENS][MAXTOKENSIZE],int &tokensSize, int &dataSize){ // parse the request into header lines and data
    string s(arr, arr + arrSize);
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
void sendAll(int socket , char * message , int totalSize){// wrap the send socket method to send all provided data
    int totalSent =0;
    while(totalSize > 0){
        int sendBytes =  send(socket, message + totalSent,totalSize, 0);
        if(sendBytes < 0)
            DieWithError((char *) "send() failed");
        totalSize -=sendBytes;
        totalSent += sendBytes;
    }
    printBuffer(message,totalSent);
}
int checkStopRecieve(char rcvBuffer[BUFFERSISE] , int totalSize){// check if we have reached a complete response
    string sBuffer(rcvBuffer , rcvBuffer + totalSize);
    int pos= sBuffer.find("\r\n\r\n");
    if(pos == -1) return 0; // not all header got yet
    char tokens[MAXTOKENS][MAXTOKENSIZE];
    int tokensSize =0;
    int dataSize =0;
    storeRequest(rcvBuffer,totalSize,"\r\n",tokens,tokensSize,dataSize);
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
                return  0; //content length != data size
            }
        }
    }
    return 0; //no content length

}
int recvAll(int socket , char *rcvBuffer){// wrap the recieve method to receive a complete request
    struct timeval tv;
    tv.tv_sec = 30;
    tv.tv_usec = 0;
    setsockopt(socket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    int rcvMsgSize = 0;
    int totalRecv =0;
    //while((rcvMsgSize = recv(socket, rcvBuffer + totalRecv, BUFFERSISE-totalRecv, 0)) > 0) {
    while((rcvMsgSize = recv(socket, rcvBuffer + totalRecv, CHUNKSIZE, 0)) > 0) {
        totalRecv += rcvMsgSize;
        //printf("-recieved-: %d\n", totalRecv);
        if(checkStopRecieve(rcvBuffer,totalRecv) == 1){
            //printBuffer(rcvBuffer,totalRecv);
            printBuffer(rcvBuffer,totalRecv);
            break;
        }
    }
    return totalRecv;
}
void handleGet(char * path , int socket , char * servIp){ // operate on get request and form the message
    string smessage = "GET "+(string)path + " HTTP/1.1\r\nHost: "+(string)servIp+"\r\n\r\n";
    char message[smessage.size()+1];
    copy(smessage.begin(),smessage.end(),message);
    message[smessage.size()]='\0';
    //cout<<message<<endl;
    sendAll(socket,message,strlen(message));
}
void handlePost(char * path, int socket, char * servIp){// operate on post request and form the message
    std::ifstream t("send.txt");
    std::string str((std::istreambuf_iterator<char>(t)),
                    std::istreambuf_iterator<char>());
    string smessage = "POST "+(string)path + " HTTP/1.1\r\nHost: "+(string)servIp+"\r\nContent-Length: "+to_string(str.size())+"\r\n\r\n";
    smessage = smessage + str;
    char message[smessage.size()+1];
    copy(smessage.begin(),smessage.end(),message);
    message[smessage.size()]='\0';
    //cout<<message<<endl;
    sendAll(socket,message,strlen(message));
}
void recieveGet(char * path, int socket, char * servIp){ // start receiving the get response
    char rcvBuffer[BUFFERSISE];
    int totalRecv = recvAll(socket,rcvBuffer);
    char tokens[MAXTOKENS][MAXTOKENSIZE];
    int tokensSize =0;
    int dataSize =0;
    storeRequest(rcvBuffer,totalRecv,"\r\n",tokens,tokensSize,dataSize);
    if(strcmp(tokens[0],(char *)"HTTP/1.1 200 OK")== 0) {
        ofstream out("fetched.txt");
        char *data = tokens[tokensSize - 1];
        out.write(data, dataSize);
        out.close();
    }
}
void recievePost(char * path, int socket, char * servIp){ // start receiving the post response
    char rcvBuffer[BUFFERSISE];
    int totalRecv = recvAll(socket,rcvBuffer);
    char tokens[MAXTOKENS][MAXTOKENSIZE];
    int tokensSize =0;
    int dataSize =0;
    storeRequest(rcvBuffer,totalRecv,"\r\n",tokens,tokensSize,dataSize);
    if(strcmp(tokens[0],(char *)"HTTP/1.1 200 OK")!=0){
        DieWithError((char *)"Data didn't posted");
    }

}
void ProcessRequest(int socket , char * servIp){ // parse the file -> get all request -> send all request -> receive all request -> close connection
    char requests[MAXREQUESTS][MAXREQUESTSIZE];
    int totalRequests =0;
    importRequests(requests,totalRequests);
    for(int i =0; i<totalRequests;i++){
        char  method[MAXMETHODSIZE];
        char path[MAXPATHSIZE];
        parseRequest(method , path , requests[i]);
        cout<< "send: "<<i<<"\n";
        if(strcmp(method, (char *)"Client_get")==0){
            handleGet(path,socket,servIp);
        }else{
            handlePost(path,socket,servIp);
        }
    }
    for(int i=0;i<totalRequests;i++){
        char  method[MAXMETHODSIZE];
        char path[MAXPATHSIZE];
        parseRequest(method , path , requests[i]);
        cout<<"recieve: "<<i<<"\n";
        if(strcmp(method, (char *)"Client_get")==0){
           recieveGet(path,socket,servIp);
        }else{
            recievePost(path,socket,servIp);
        }
    }

}

