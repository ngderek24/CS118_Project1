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
#include <cstring>

using namespace std;

void error(string msgString) {
    const char *msg = msgString.c_str();
    perror(msg);
    exit(0);
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

void setup(int &socketListener, int &socketfd, struct sockaddr_in &serverAddr, int portNum) {
    // open socket
    socketListener = socket(AF_INET, SOCK_STREAM, 0);
    if (socketListener < 0)
        error("ERROR opening socket");
    
    // zero out bytes of serverAddr
    bzero((char *) &serverAddr, sizeof(serverAddr));
    
    // set serverAddr properties
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(portNum);
    
    // bind socket to IP address and port number
    if (bind(socketListener, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0)
        error("ERROR on binding");
}

void handleRequest(int socketfd) {
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

    // if no file specified, just display a message
    if (strcmp(filename, "") == 0) {
        result = write(socketfd, "I got your message", 18);
        if (result < 0) 
            error("ERROR writing to socket");
    }
    else {
        // open file for reading
        FILE *fp = fopen(filename, "r");
        if (fp == NULL) {
            string errMsg = "404 Not Found";
            result = write(socketfd, errMsg.c_str(), 30);
                if (result < 0) 
                    error("ERROR writing to socket");
            cout << "ERROR opening file" << endl;
        }
        else {
            // write file to buffer
            const int BUFFER_SIZE = 2000000;
            char writeBuffer[BUFFER_SIZE] = { 0 };
            int bytesRead = fread(writeBuffer, 1, BUFFER_SIZE, fp);
            
            if (bytesRead > 0) {
                // send response to client
                result = write(socketfd, writeBuffer, BUFFER_SIZE);
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
    }
}

int main(int argc, char *argv[]) {
    int socketListener, portNum, socketfd;
    socklen_t clientAddrLength;
    struct sockaddr_in serverAddr, clientAddr;
    
    // get port number and setup
    portNum = atoi(argv[1]);
    setup(socketListener, socketfd, serverAddr, portNum);
 
    // listen and accept
    listen(socketListener, 5);
    clientAddrLength = sizeof(clientAddr);
    
    while(1) {
        socketfd = accept(socketListener, (struct sockaddr *) &clientAddr, &clientAddrLength);
        if (socketfd < 0) 
            error("ERROR on accept");
            
        int pid = fork();
        // child process handles the client request
        if (pid == 0)  { 
             close(socketListener);
             
             handleRequest(socketfd);
             exit(0);
        }
        // parent closes socket and continues to listen
        else if (pid > 0) {
            close(socketfd);
        }
        // fork failed
        else {
             error("ERROR on fork");
        }
    }
    
    
    // close sockets 
    close(socketListener);
    return 0;
}
