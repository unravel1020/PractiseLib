// #include <iostream>
// #include <cstring>
// #include <sys/socket.h>
// #include <arpa/inet.h>
// #include <unistd.h>

// #define PORT 8080 // 服务端监听的端口号

// int main() {
//     int sock = 0; // 文件描述符
//     struct sockaddr_in serv_addr;
//     char buffer[1024] = {0}; // 用于存储接收到的数据

//     // 创建 socket 文件描述符
//     if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
//         perror("Socket creation failed");
//         exit(EXIT_FAILURE);
//     }

//     serv_addr.sin_family = AF_INET;         // IPv4
//     serv_addr.sin_port = htons(PORT);       // 端口号

//     // 将 IP 地址转换为二进制格式
//     if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
//         perror("Invalid address/ Address not supported");
//         close(sock);
//         exit(EXIT_FAILURE);
//     }

//     // 连接到服务端
//     if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
//         perror("Connection failed");
//         close(sock);
//         exit(EXIT_FAILURE);
//     }

//     // 向服务端发送消息
//     const char *message = "Hello from client!";
//     send(sock, message, strlen(message), 0);
//     std::cout << "Message sent to server.\n";

//     // 读取服务端的响应
//     read(sock, buffer, 1024);
//     std::cout << "Response from server: " << buffer << "\n";

//     // 关闭连接
//     close(sock);

//     return 0;
// }


//以上为Linux版本的客户端代码，在Windows下运行可能会出错，因为Windows的socket编程和Linux的socket编程有些区别。

// #include <iostream>
// #include <winsock2.h>

// #pragma comment(lib, "ws2_32.lib") // 链接 Winsock 库

// #define PORT 8080

// int main() {
//     WSADATA wsaData;
//     SOCKET sock;
//     struct sockaddr_in serv_addr;
//     char buffer[1024] = {0};

//     // 初始化 Winsock
//     if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//         std::cerr << "WSAStartup failed.\n";
//         return -1;
//     }

//     // 创建 socket
//     if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
//         std::cerr << "Socket creation failed.\n";
//         WSACleanup();
//         return -1;
//     }

//     // 设置服务器地址和端口
//     serv_addr.sin_family = AF_INET;
//     serv_addr.sin_port = htons(PORT);
//     serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

//     // 连接到服务器
//     if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
//         std::cerr << "Connection failed.\n";
//         closesocket(sock);
//         WSACleanup();
//         return -1;
//     }

//     // 向服务器发送消息
//     const char *message = "Hello from client!";
//     send(sock, message, strlen(message), 0);
//     std::cout << "Message sent to server.\n";

//     // 读取服务器的响应
//     recv(sock, buffer, 1024, 0);
//     std::cout << "Response from server: " << buffer << "\n";

//     // 关闭连接
//     closesocket(sock);
//     WSACleanup();

//     return 0;
// }


//以上是windows版本的客户端代码，在Linux下运行可能会出错，因为Linux的socket编程和Windows的socket编程有些区别。

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <winsock2.h>
#include <chrono>

#include "Logger.h"
#include "Protocol.h"

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 8080

using namespace net::logging;
using namespace net::protocol;

std::atomic<bool> is_running(true);
std::atomic<bool> is_connected(true);

void send_heartbeat(SOCKET sock) {
    while (is_connected && is_running) {
        std::string ping_packet = Protocol::serialize(MessageType::PING, "");
        send(sock, ping_packet.c_str(), ping_packet.size(), 0);
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
}

SOCKET connect_to_server() {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    
    // 使用 Windows 兼容方式设置 IP 地址
    serverAddr.sin_addr.s_addr = inet_addr(SERVER_IP);

    if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
        LOG(LogLevel::ERR) << "Invalid IP address.";
        closesocket(sock);
        return INVALID_SOCKET;
    }

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        LOG(LogLevel::ERR) << "Connect failed.";
        closesocket(sock);
        return INVALID_SOCKET;
    }

    LOG(LogLevel::INFO) << "Connected to server.";
    return sock;
}

int main() {
    Logger::init("client", Logger::Level::INFO, 7);

    SOCKET sock = connect_to_server();
    if (sock == INVALID_SOCKET) {
        LOG(LogLevel::ERR) << "Failed to connect to server.";
        return -1;
    }

    std::thread heartbeat_thread(send_heartbeat, sock);
    heartbeat_thread.detach();

    while (is_running) {
        std::cout << "Enter message to send (type 'exit' to quit): ";
        std::string message;
        std::getline(std::cin, message);

        if (message == "exit") {
            is_connected = false;
            send(sock, Protocol::serialize(MessageType::EXIT, "").c_str(), 0, 0);
            break;
        }

        std::string packet = Protocol::serialize(MessageType::DATA, message);
        send(sock, packet.c_str(), packet.size(), 0);
        LOG(LogLevel::INFO) << "Message sent to server (" << packet.size() << " bytes).";

        char buffer[1024];
        int bytes = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes > 0) {
            MessageType type;
            std::string data;
            if (Protocol::deserialize(buffer, bytes, type, data)) {
                LOG(LogLevel::INFO) << "Response from server: " << data;
            }
        } else {
            int err = WSAGetLastError();
            LOG(LogLevel::ERR) << "Server disconnected or error occurred. Error code: " << err;
            break;
        }
    }

    closesocket(sock);
    WSACleanup();
    Logger::shutdown();

    return 0;
}