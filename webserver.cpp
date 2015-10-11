#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <iostream>
#include <ctime>
#include <cstdio>

using namespace std;

void error(string msgString) {
    const char *msg = msgString.c_str();
    perror(msg);
    exit(0);
}

string getTime(struct tm* tmPtr) {
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%a, %d %b %G %T GMT", tmPtr);
    string dateString(buffer);
    return dateString;
}

string getHeaderValue(const string& request, const string& headerName) {
    string headerValue = "";
    
    int headerNameIndex = request.find(headerName);
    if (headerNameIndex != string::npos) {
        int headerValueIndex = headerNameIndex + headerName.size() + 2;
        int carriageReturnIndex = request.find("\r", headerValueIndex);
        if (carriageReturnIndex != string::npos) {
            headerValue = request.substr(headerValueIndex,                              carriageReturnIndex - headerValueIndex);
        }
    }
    
    return headerValue;
}

string getFilename(const string& request) {
    string filename = "";
    int filenameIndex = 5;
    int spaceIndex = request.find(" ", filenameIndex);
    if (spaceIndex != string::npos) {
        filename = request.substr(filenameIndex, 
                    spaceIndex - filenameIndex);
    } 
    return filename;
}

int main(int argc, char *argv[]) {
    int socketListener, portNum, socketfd;
    socklen_t clientAddrLength;
    struct sockaddr_in serverAddr, clientAddr;
    
    // open socket
    socketListener = socket(AF_INET, SOCK_STREAM, 0);
    if (socketListener < 0) 
        error("ERROR opening socket");
        
    // zero out bytes of serverAddr and get port number
    bzero((char *) &serverAddr, sizeof(serverAddr));
    portNum = atoi(argv[1]);
    
    // set serverAddr properties
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(portNum);
 
    // bind socket to IP address and port number
    if (bind(socketListener, (struct sockaddr *) &serverAddr, 
            sizeof(serverAddr)) < 0) 
        error("ERROR on binding");
 
    // listen and accept
    listen(socketListener, 5);
    clientAddrLength = sizeof(clientAddr);
    int socketNum = 1;
    
    fd_set active_fd_set;
    FD_ZERO(&active_fd_set);
    FD_SET(socketListener, &active_fd_set);

    while(1) {
        /*
        if (select(socketNum + 1, &active_fd_set, NULL, 
                    NULL, NULL) < 0)
            error("ERROR on select");
        */
        //cout << "after select";   
        //if (FD_ISSET(socketListener, &active_fd_set)) {
            socketfd = accept(socketListener, (struct sockaddr *)                               &clientAddr, &clientAddrLength);
            if (socketfd < 0) 
                error("ERROR on accept");
            
            //socketNum++;
            //FD_SET(socketfd, &active_fd_set);
            
            int result;
            char buffer[1024];
            bzero(buffer, 1024);
        
            // read client request
            result = read(socketfd, buffer, 1024);
            if (result < 0) 
                error("ERROR reading from socket");
        
            // print client request
            cout << buffer << endl;
        
            // parse file name
            const char* filename = getFilename(buffer).c_str();
        
            // open file for reading
            FILE *fp = fopen(filename, "r");
            if (fp == NULL) {
                string errMsg = "404 Not Found";
                result = write(socketfd, errMsg.c_str(), 30);
                    if (result < 0) 
                        error("ERROR writing to socket");
                error("ERROR opening file");
            }
            else {
                // write file to buffer
                char writeBuffer[10000] = { 0 };
                int bytesRead = fread(writeBuffer, 1, 10000, fp);
                
                if (bytesRead > 0) {
                    // send response to client
                    result = write(socketfd, writeBuffer, 10000);
                    if (result < 0) 
                        error("ERROR writing to socket");
                }
                else {
                    string errMsg = "500 Internal Server Error";
                    result = write(socketfd, errMsg.c_str(), 30);
                    if (result < 0) 
                        error("ERROR writing to socket");
                }
            }
            fclose(fp);
        //}
    }
    
    // close sockets
    close(socketfd);  
    close(socketListener);
    return 0;
}
