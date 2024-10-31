#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <sstream>
#include <ncurses.h>

class Base64Decoder {
public:
    static int fromBase64(const std::string& base64Str) {
        int value = 0;
        for (char c : base64Str) {
            value *= 64;
            if (c >= 'A' && c <= 'Z') {
                value += c - 'A';
            } else if (c >= 'a' && c <= 'z') {
                value += c - 'a' + 26;
            } else if (c >= '0' && c <= '9') {
                value += c - '0' + 52;
            } else if (c == '+') {
                value += 62;
            } else if (c == '/') {
                value += 63;
            }
        }
        return value;
    }

    static std::string decodeIP(const std::string& encodedIP) {
        std::string output;
        std::stringstream base64Stream;
        for (char ch : encodedIP) {
            if (isdigit(ch) || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || ch == '+' || ch == '/') {
                base64Stream << ch;
            } else if (ch == '-') {
                std::string base64Str = base64Stream.str();
                int value = fromBase64(base64Str);
                output.append(std::to_string(value)).append(".");
                base64Stream.str("");
            }
        }
        if (!base64Stream.str().empty()) {
            std::string base64Str = base64Stream.str();
            int value = fromBase64(base64Str);
            output.append(std::to_string(value));
        }
        if (!output.empty() && output.back() == '.') {
            output.pop_back();
        }
        return output;
    }
};

class FileTransferClient {
    int sock;
    struct sockaddr_in serv_addr;
    std::string fileName;

public:
    FileTransferClient(const std::string& ip, int port, const std::string& fileName) : fileName(fileName) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);
        inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr);
    }

    void connectToServer() {
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("Connection Failed");
            exit(EXIT_FAILURE);
        }
    }

    void sendFile() {
        std::ifstream infile(fileName, std::ios::binary);
        if (!infile) {
            perror("File opening failed");
            return;
        }
        char buffer[1024];
        while (infile.read(buffer, sizeof(buffer))) {
            send(sock, buffer, infile.gcount(), 0);
        }
        send(sock, buffer, infile.gcount(), 0);
        infile.close();
    }

    ~FileTransferClient() {
        close(sock);
    }
};

class ClientUI {
public:
    ClientUI() {
        initscr();
        noecho();
        cbreak();
    }

    ~ClientUI() {
        endwin();
    }

    std::string getSessionID() {
        mvprintw(3, (COLS - 34) / 2, "Enter Session ID: ");
        refresh();
        echo();
        char receivedID[256];
        getnstr(receivedID, sizeof(receivedID) - 1);
        noecho();
        return std::string(receivedID);
    }

    void showMessage(const std::string& message) {
        mvprintw(5, (COLS - message.size()) / 2, "%s", message.c_str());
        refresh();
    }

    void waitForExit() {
        getch();
    }
};
#endif