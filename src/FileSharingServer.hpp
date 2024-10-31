#ifndef SERVER_HPP
#define SERVER_HPP

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <sstream>
#include <fstream>
#include <ncurses.h>

class Base64Encoder {
public:
    static const std::string base64_chars;

    static std::string toBase64(int num) {
        if (num == 0) return "A";
        std::string result;
        while (num > 0) {
            result = base64_chars[num % 64] + result;
            num /= 64;
        }
        return result;
    }

    static std::string encodeIP(const std::string& ip) {
        std::string output;
        std::stringstream numberStream;
        std::stringstream octetStream;

        for (char ch : ip) {
            if (isdigit(ch)) {
                numberStream << ch;
            } else if (ch == '.') {
                if (!numberStream.str().empty()) {
                    int number = std::stoi(numberStream.str());
                    octetStream << toBase64(number) << "-";
                    numberStream.str("");
                }
            }
        }
        if (!numberStream.str().empty()) {
            int number = std::stoi(numberStream.str());
            octetStream << toBase64(number);
        }
        output = octetStream.str();
        if (!output.empty() && output.back() == '-') {
            output.pop_back();
        }
        return output;
    }
};

const std::string Base64Encoder::base64_chars =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
   "abcdefghijklmnopqrstuvwxyz"
   "0123456789+/";

class NetworkUtils {
public:
    static std::string getLocalIP() {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) return "";
        const char* kDummyAddress = "8.8.8.8";
        int kDummyPort = 80;
        struct sockaddr_in dummy_addr;
        std::memset(&dummy_addr, 0, sizeof(dummy_addr));
        dummy_addr.sin_family = AF_INET;
        dummy_addr.sin_port = htons(kDummyPort);
        inet_pton(AF_INET, kDummyAddress, &dummy_addr.sin_addr);
        connect(sock, (struct sockaddr*)&dummy_addr, sizeof(dummy_addr));
        struct sockaddr_in local_addr;
        socklen_t addr_len = sizeof(local_addr);
        getsockname(sock, (struct sockaddr*)&local_addr, &addr_len);
        close(sock);
        char local_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &local_addr.sin_addr, local_ip, INET_ADDRSTRLEN);
        return std::string(local_ip);
    }
};

class Server {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

public:
    Server(int port) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
        }

        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            perror("Set socket options failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("Bind failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, 3) < 0) {
            perror("Listen failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }
    }

    void acceptConnection() {
        new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("Accept failed");
            close(server_fd);
            exit(EXIT_FAILURE);
        }
    }

    void receiveFile(const std::string &filePath) {
        std::ofstream outfile(filePath, std::ios::binary);
        if (!outfile) {
            perror("File opening failed");
            return;
        }

        char buffer[1024];
        int bytesRead;
        while ((bytesRead = read(new_socket, buffer, sizeof(buffer))) > 0) {
            outfile.write(buffer, bytesRead);
        }

        if (bytesRead < 0) {
            perror("Read error");
        }

        outfile.close();
    }

    ~Server() {
        close(new_socket);
        close(server_fd);
    }
};

class ServerUI {
public:
    ServerUI() {
        initscr();
        noecho();
        cbreak();
        curs_set(0);
    }

    ~ServerUI() {
        endwin();
        curs_set(1);
    }

    std::string getFilename() {
        mvprintw(3, (COLS - 34) / 2, "Enter filename to receive: ");
        refresh();
        echo();
        char filename[256];
        getnstr(filename, sizeof(filename) - 1);
        noecho();
        return std::string(filename);
    }

    void displaySessionID(const std::string& encodedIP) {
        mvprintw(5, (COLS - 28) / 2, "Session ID: %s", encodedIP.c_str());
        refresh();
    }

    void showCompletionMessage() {
        mvprintw(7, (COLS - 18) / 2, "Transfer Complete!");
        refresh();
    }

    void waitForExit() {
        getch();
    }
};
#endif