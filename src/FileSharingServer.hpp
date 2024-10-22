#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

int main() {
	int server_fd, new_socket; //server_fd: store file descriptor for the accepted client connection, new_socket: Holds the file_descriptor for the accepted client after a successful call to accept()
	struct sockaddr_in address; //IP address & Port Number
	int addrlen = sizeof(address); //size of the address structure
	char buffer[1024] = {0}; //store incoming data from the client

	//creating socket 
	
	server_fd = socket(AF_INET, SOCK_STREAM, 0); //AF_INET stands for IPv4(address family), SOCK_STREAM (type of socket), 0: protocol argument 
	//Bind the socket to a port
	
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(8080);
	bind(server_fd, (struct sockaddr *)&address, sizeof(address));

	//listen and accept connection
	
	listen(server_fd, 3);
	new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);

	//recieve file data
	int valread = read(new_socket, buffer, 1024);
	cout<< "File Data: " <<buffer<<endl;

	close(new_socket);
	close(server_fd);

	return 0;
}


