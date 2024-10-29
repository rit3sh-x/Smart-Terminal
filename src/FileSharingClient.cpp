#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fstream>
#include <cstring>

class Client {
    int sock;
    struct sockaddr_in serv_addr;

public:
    Client(const std::string &ip, int port) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            std::cerr << "Socket creation error\n";
            exit(EXIT_FAILURE);
        }

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
            std::cerr << "Invalid address\n";
            exit(EXIT_FAILURE);
        }
    }

    void connectToServer() {
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            std::cerr << "Connection Failed\n";
            exit(EXIT_FAILURE);
        }
        std::cout << "Connected to the server\n";
    }

    void sendFile(const std::string &filePath) {
        std::ifstream infile(filePath, std::ios::binary);
        if (!infile.is_open()) {
            std::cerr << "File could not be opened\n";
            return;
        }

        char buffer[1024];
        int bytesRead;
        int totalBytes = 0;
        
        while (infile.read(buffer, sizeof(buffer)) || (bytesRead = infile.gcount()) > 0) {
            send(sock, buffer, bytesRead, 0);
            totalBytes += bytesRead;
            std::cout << "Sent: " << totalBytes << " bytes\r";
            std::cout.flush();
        }

        std::cout << "\nFile sent successfully\n";
        infile.close();
    }

    ~Client() {
        close(sock);
    }
};

int main() {
    std::string ip = "192.168.1.72";  // or the server IP if on a different network
    int port = 8080;
    Client client(ip, port);
    client.connectToServer();

    std::string filePath;
    std::cout << "Enter the file path you want to send: ";
    std::cin >> filePath;

    client.sendFile(filePath);
    return 0;
}
