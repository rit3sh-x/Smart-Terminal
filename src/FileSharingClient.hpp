#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h> // For inet_pton
#include <unistd.h>    // For close()
#include <cstring>     // For memset()

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    const char *message = "Hello from client";
    char buffer[1024] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    // Set the server address and port
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    // Convert IPv4 address from text to binary
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address or Address not supported" << std::endl;
        return -1;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }

    // Send the message
    send(sock, message, strlen(message), 0);
    std::cout << "Message sent to server" << std::endl;

    // Receive server response (if any)
    int valread = read(sock, buffer, 1024);
    if (valread > 0) {
        std::cout << "Server response: " << buffer << std::endl;
    } else {
        std::cerr << "No response from server" << std::endl;
    }

    // Close the socket
    close(sock);

    return 0;
}

