// #include <iostream>
// #include <cstring>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <unistd.h>

// #define PORT 8080 // 服务端监听的端口号

// int main() {
//     int server_fd, new_socket; // 文件描述符
//     struct sockaddr_in address;
//     int opt = 1;
//     int addrlen = sizeof(address);
//     char buffer[1024] = {0}; // 用于存储接收到的数据

//     // 创建 socket 文件描述符
//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
//         perror("Socket creation failed");
//         exit(EXIT_FAILURE);
//     }

//     // 设置 socket 选项（允许端口复用）
//     if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
//         perror("Setsockopt failed");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     // 绑定地址和端口
//     address.sin_family = AF_INET;         // IPv4
//     address.sin_addr.s_addr = INADDR_ANY; // 监听所有本地地址
//     address.sin_port = htons(PORT);       // 端口号

//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
//         perror("Bind failed");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     // 开始监听
//     if (listen(server_fd, 3) < 0) { // 最大等待队列长度为 3
//         perror("Listen failed");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     std::cout << "Server is listening on port " << PORT << "...\n";

//     // 接受客户端连接
//     if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
//         perror("Accept failed");
//         close(server_fd);
//         exit(EXIT_FAILURE);
//     }

//     // 读取客户端发送的数据
//     read(new_socket, buffer, 1024);
//     std::cout << "Message from client: " << buffer << "\n";

//     // 向客户端发送响应
//     const char *response = "Hello from server!";
//     send(new_socket, response, strlen(response), 0);
//     std::cout << "Response sent to client.\n";

//     // 关闭连接
//     close(new_socket);
//     close(server_fd);

//     return 0;
// }

//以上为Linux版本的代码，在Windows下编译会报错，因为Windows没有SOCK_STREAM和SOL_SOCKET等宏定义，需要使用不同的库和函数。

// #include <iostream>
// #include <winsock2.h>

// #pragma comment(lib, "ws2_32.lib") // 链接 Winsock 库

// #define PORT 8080

// int main() {
//     WSADATA wsaData;
//     SOCKET server_fd, new_socket;
//     struct sockaddr_in address;
//     int addrlen = sizeof(address);
//     char buffer[1024] = {0};

//     // 初始化 Winsock
//     if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//         std::cerr << "WSAStartup failed.\n";
//         return -1;
//     }

//     // 创建 socket
//     if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
//         std::cerr << "Socket creation failed.\n";
//         WSACleanup();
//         return -1;
//     }

//     // 设置地址和端口
//     address.sin_family = AF_INET;
//     address.sin_addr.s_addr = INADDR_ANY;
//     address.sin_port = htons(PORT);

//     // 绑定 socket
//     if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
//         std::cerr << "Bind failed.\n";
//         closesocket(server_fd);
//         WSACleanup();
//         return -1;
//     }

//     // 开始监听
//     if (listen(server_fd, 3) == SOCKET_ERROR) {
//         std::cerr << "Listen failed.\n";
//         closesocket(server_fd);
//         WSACleanup();
//         return -1;
//     }

//     std::cout << "Server is listening on port " << PORT << "...\n";

//     // 接受客户端连接
//     if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) == INVALID_SOCKET) {
//         std::cerr << "Accept failed.\n";
//         closesocket(server_fd);
//         WSACleanup();
//         return -1;
//     }

//     // 读取客户端发送的数据
//     recv(new_socket, buffer, 1024, 0);
//     std::cout << "Message from client: " << buffer << "\n";

//     // 向客户端发送响应
//     const char *response = "Hello from server!";
//     send(new_socket, response, strlen(response), 0);
//     std::cout << "Response sent to client.\n";

//     // 关闭连接
//     closesocket(new_socket);
//     closesocket(server_fd);
//     WSACleanup();

//     return 0;
// }

//以上是Windows版本的代码，在Linux下编译会报错，因为Windows没有SOCK_STREAM和SOL_SOCKET等宏定义，需要使用不同的库和函数。

#include <iostream>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib") // 链接 Winsock 库

#define PORT 8080
#define RECV_TIMEOUT_MS 25000 // 接收超时时间，单位为毫秒（例如 5 秒）

int main() {
    WSADATA wsaData;
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};

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

    // 接受客户端连接
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) == INVALID_SOCKET) {
        std::cerr << "Accept failed.\n";
        closesocket(server_fd);
        WSACleanup();
        return -1;
    }

    std::cout << "Client connected.\n";

    // 设置接收超时时间
    DWORD timeout = RECV_TIMEOUT_MS;
    if (setsockopt(new_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR) {
        std::cerr << "Failed to set recv timeout: " << WSAGetLastError() << "\n";
        closesocket(new_socket);
        closesocket(server_fd);
        WSACleanup();
        return -1;
    }

    // 持续通信
    while (true) {
        memset(buffer, 0, sizeof(buffer)); // 清空缓冲区
        int bytes_received = recv(new_socket, buffer, sizeof(buffer), 0);
        if (bytes_received > 0) {
            std::cout << "Message from client: " << buffer << "\n";

            // 向客户端发送响应
            const char *response = "Hello from server!";
            send(new_socket, response, strlen(response), 0);
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
    closesocket(new_socket);
    closesocket(server_fd);
    WSACleanup();

    return 0;
}