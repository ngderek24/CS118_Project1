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
#include <unistd.h>

using namespace std;

void error(string msgString) {
    const char *msg = msgString.c_str();
    perror(msg);
    exit(0);
}

// parses the filename from the request
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

// gets current GMT time in the proper format
string getTime() {
    time_t t;
    time(&t);
    struct tm *tmPtr = gmtime(&t);
    
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%a, %d %b %G %T GMT", tmPtr);
    string dateString(buffer);
    return dateString;
}

// setup the connection between server and client
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

// formats header line given header name and value
string formatHeader(const string& headerName, const string& headerValue) {
    return headerName + ": " + headerValue + "\n";
}

// parses file type from the file name
string getFileType(const string& filename) {
    int index = filename.find(".");
    string filetype = "";
    filetype = filename.substr(index + 1, filename.size() - index - 1);
    return filetype;
}

// builds header section of the response
string buildHeaders(const string& statusCode, const string& filename) {
    string headers = "HTTP/1.1 " + statusCode + "\n";
    headers += formatHeader("Date", getTime());
    headers += formatHeader("Connection", "keep-alive");
    if (filename != "" && filename.find(".") != string::npos) {
        headers += formatHeader("Content-Type", getFileType(filename));
    }
    return headers + "\n";
}

// handles request accordingly
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
    string filename = getFilename(buffer);

    // if no file specified, just display a message
    if (filename == "") {
        string response = buildHeaders("200 OK", filename) + "I got your message";
        result = write(socketfd, response.c_str(), response.size());
        
        if (result < 0) 
            error("ERROR writing to socket");
    }
    else {
        // open file for reading
        FILE *fp = fopen(filename.c_str(), "r");
        if (fp == NULL) {
            string response = buildHeaders("404 Not Found", "") + "404 Not Found";
            result = write(socketfd, response.c_str(), response.size());
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
                string response = buildHeaders("200 OK", filename);
                result = write(socketfd, response.c_str(), response.size());
                result = write(socketfd, writeBuffer, BUFFER_SIZE);
                if (result < 0) 
                    error("ERROR writing to socket");
            }
            else {
                string response = buildHeaders("500 Internal Server Error", filename) + "500 Internal Server Error";
                result = write(socketfd, response.c_str(), response.size());
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
