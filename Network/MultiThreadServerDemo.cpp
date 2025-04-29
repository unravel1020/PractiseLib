#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <winsock2.h>
#include <csignal>
#include <unistd.h>
#include <atomic>

#pragma comment(lib, "ws2_32.lib") // 链接 Winsock 库

#define PORT 8080
#define RECV_TIMEOUT_MS 5000 // 接收超时时间，单位为毫秒（例如 5 秒）

std::atomic<bool> is_running(true);
std::vector<std::thread> threads;
SOCKET server_fd;

void stop_server(){
    is_running = false;
}


void signal_handLer(int signal){
    std::cout << "Interrupt signal (" << signal << ") received.\n";
 
    // 清理并关闭
    // 终止程序    
    stop_server();
    
    for(auto &th : threads){
        if(th.joinable())
            th.join();
    }

    closesocket(server_fd);
    WSACleanup();
    
    std::cout << "Server stopped.\n";
    std::cout << "Cleaned  up resources.\n";

    exit(0);  
}
void handle_client(SOCKET client_socket) {
    char buffer[1024] = {0};
    DWORD timeout = RECV_TIMEOUT_MS;
    
    // 设置接收超时时间
    if (setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
        std::cerr << "Failed to set recv timeout: " << WSAGetLastError() << "\n";
        closesocket(client_socket);
        return;
    }

    while (true) {
        memset(buffer, 0, sizeof(buffer)); // 清空缓冲区
        int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            std::cout << "Message from client: " << buffer << "\n";

            // 向客户端发送响应
            const char *response = "Hello from server!";
            send(client_socket, response, strlen(response), 0);
            std::cout << "Response sent to client.\n";
        } else if (bytes_received == 0) {
            std::cout << "Client gracefully closed the connection.\n";
            break; // 客户端正常关闭连接
        } else {
            int error_code = WSAGetLastError();
            if (error_code == WSAETIMEDOUT) {
                std::cout << "Receive timed out. No data received from client.\n";
            } else {
                std::cout << "Error in receiving data from client: " << error_code << "\n";
            }
            break; // 发生错误或超时，退出循环
        }
    }

    // 关闭连接
    closesocket(client_socket);
}

int main() {
    WSADATA wsaData;

    struct sockaddr_in address;
    signal(SIGINT, signal_handLer); // 注册信号处理函数
    int addrlen = sizeof(address);

    // 初始化 Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return -1;
    }

    // 创建 socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return -1;
    }

    // 设置地址和端口
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 绑定 socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Bind failed.\n";
        closesocket(server_fd);
        WSACleanup();
        return -1;
    }

    // 开始监听
    if (listen(server_fd, 3) == SOCKET_ERROR) {
        std::cerr << "Listen failed.\n";
        closesocket(server_fd);
        WSACleanup();
        return -1;
    }

    std::cout << "Server is listening on port " << PORT << "...\n";


    // 主循环：接受客户端连接并启动新线程处理
    while (is_running) {
        SOCKET new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
        if (new_socket == INVALID_SOCKET) {
            std::cerr << "Accept failed.\n";
            continue;
        }
        std::cout << "New client connected.\n";

        // 为每个客户端创建一个新线程
        threads.emplace_back(handle_client, new_socket);
    }

    // 等待所有线程结束（实际上这里不会到达）
    for (auto& th : threads) {
        if (th.joinable()) {
            th.join();
        }
    }

    // 关闭监听 socket 和 Winsock 库
    closesocket(server_fd);
    WSACleanup();

    return 0;
}