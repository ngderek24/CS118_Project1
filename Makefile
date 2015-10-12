all: webserver.cpp
    g++ -o webserver webserver.cpp
    
clean: 
    $(RM) webserver
