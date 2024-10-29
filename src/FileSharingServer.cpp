#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <cstring>

class Server {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

public:
    Server(int port) {
        // Create socket
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == 0) {
            std::cerr << "Socket creation error\n";
            exit(EXIT_FAILURE);
        }

        // Bind socket to port
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);
        bind(server_fd, (struct sockaddr *)&address, sizeof(address));

        // Start listening
        listen(server_fd, 3);
    }

    void acceptConnection() {
        std::cout << "Waiting for a connection...\n";
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            std::cerr << "Connection acceptance failed\n";
            exit(EXIT_FAILURE);
        }
        std::cout << "Connection established!\n";
    }

    void receiveFile(const std::string &filePath) {
        std::ofstream outfile(filePath, std::ios::binary);
        char buffer[1024];
        int bytesRead;
        int totalBytes = 0;
        
        while ((bytesRead = read(new_socket, buffer, 1024)) > 0) {
            outfile.write(buffer, bytesRead);
            totalBytes += bytesRead;
            std::cout << "Received: " << totalBytes << " bytes\r";
            std::cout.flush();
        }
        
        std::cout << "\nFile received successfully\n";
        outfile.close();
    }

    ~Server() {
        close(new_socket);
        close(server_fd);
    }
};

int main() {
    Server server(8080);
    server.acceptConnection();
    server.receiveFile("received_file.txt");
    return 0;
}
