#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <strings.h>
#include <iostream>
#include <ctime>

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
			headerValue = request.substr(headerValueIndex, 								carriageReturnIndex - headerValueIndex);
		}
	}
	
	return headerValue;
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
 	listen(socketListener,1);
 	clientAddrLength = sizeof(clientAddr);
    socketfd = accept(socketListener, (struct sockaddr *) &clientAddr, 						&clientAddrLength);
    if (socketfd < 0) 
    	error("ERROR on accept");
             
    int result;
    char buffer[1024];
    bzero(buffer, 1024);
    
    // read client request
    result = read(socketfd, buffer, 1024);
    if (result < 0) 
    	error("ERROR reading from socket");
    
    // print client request
    cout << buffer << endl;
    
    // set up response message
    time_t t;
    time(&t);
    struct tm *tmPtr = gmtime(&t);
    
    // send response to client
    //res = write(socketfd, ptm->tm_min, 20);
    //if (res < 0) 
    	//error("ERROR writing to socket");
    
    // close sockets
    close(socketfd);         
    close(socketListener);
	return 0;
}
