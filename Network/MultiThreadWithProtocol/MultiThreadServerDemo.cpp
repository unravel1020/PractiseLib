#include <string>
#include <vector>
#include <csignal>
#include <atomic>
#include <iostream>
#include <winsock2.h>
#include <thread>
#include <chrono>

// 使用的头文件
#include "Logger.h"
#include "Protocol.h"

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define RECV_TIMEOUT_MS 5000

std::atomic<bool> is_running(true);
std::vector<std::thread> threads;
SOCKET server_fd;

// 引入命名空间
using namespace net::logging;
using namespace net::protocol;

void stop_server() {
    is_running = false;
}

void signal_handler(int signal) {
    LOG(LogLevel::INFO) << "Interrupt signal (" << signal << ") received.";
    stop_server();

    for (auto& th : threads) {
        if (th.joinable()) th.join();
    }

    closesocket(server_fd);
    WSACleanup();

    LOG(LogLevel::INFO) << "Server stopped.";
    exit(0);
}

void handle_client(SOCKET client_socket) {
    DWORD timeout = RECV_TIMEOUT_MS;
    setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    char buffer[1024];
    auto last_active_time = std::chrono::steady_clock::now();

    while (is_running) {
        int bytes = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes <= 0) {
            int err = WSAGetLastError();
            LOG(LogLevel::ERR) << "recv() failed with error: " << err;
            break;
        }

        last_active_time = std::chrono::steady_clock::now();

        MessageType type;
        std::string data;
        if (!Protocol::deserialize(buffer, bytes, type, data)) {
            LOG(LogLevel::ERR) << "Failed to parse message.";
            std::string packet = Protocol::serialize(MessageType::ERROR_MSG, "Invalid message format.");
            send(client_socket, packet.c_str(), packet.size(), 0);
            continue; // ❗不要 break
        }

        switch (type) {
            case MessageType::DATA:
                LOG(LogLevel::INFO) << "Received data: " << data.substr(0, 100);
                send(client_socket,
                     Protocol::serialize(MessageType::DATA, "Hello from server!").c_str(),
                     static_cast<int>(data.size()), 0);
                break;

            case MessageType::EXIT:
                LOG(LogLevel::INFO) << "Client requested disconnect.";
                goto disconnect;

            case MessageType::PING:
                LOG(LogLevel::INFO) << "Received PING, sending PONG.";
                send(client_socket,
                     Protocol::serialize(MessageType::PONG, "").c_str(),
                     0, 0);
                break;

            case MessageType::PONG:
                LOG(LogLevel::INFO) << "Received PONG.";
                last_active_time = std::chrono::steady_clock::now();
                break;

            default:
                LOG(LogLevel::WARNING) << "Unknown message type: " << static_cast<int>(type);
                break;
        }
    }

disconnect:
    LOG(LogLevel::INFO) << "Client disconnected.";
    closesocket(client_socket);
}

int main() {
    Logger::init("server", Logger::Level::INFO, 7);
    LOG(LogLevel::INFO) << "Starting server...";

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    signal(SIGINT, signal_handler);

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    listen(serverSocket, SOMAXCONN);

    LOG(LogLevel::INFO) << "Server is listening on port " << PORT << "...";
    server_fd = serverSocket;

    while (is_running) {
        SOCKET new_socket = accept(serverSocket, nullptr, nullptr);
        if (new_socket != INVALID_SOCKET) {
            LOG(LogLevel::INFO) << "New client connected.";
            threads.emplace_back(handle_client, new_socket);
        }
    }

    for (auto& th : threads)
        if (th.joinable())
            th.join();

    closesocket(serverSocket);
    WSACleanup();
    Logger::shutdown();

    return 0;
}