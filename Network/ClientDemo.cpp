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
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib") // 链接 Winsock 库

#define PORT 8080

int main() {
    WSADATA wsaData;
    SOCKET sock;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    // 初始化 Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed.\n";
        return -1;
    }

    // 创建 socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        std::cerr << "Socket creation failed.\n";
        WSACleanup();
        return -1;
    }

    // 设置服务器地址和端口
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 连接到服务器
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        std::cerr << "Connection failed.\n";
        closesocket(sock);
        WSACleanup();
        return -1;
    }

    std::cout << "Connected to server.\n";

    // 持续通信
    while (true) {
        std::cout << "Enter message to send (type 'exit' to quit): ";
        std::string message;
        std::getline(std::cin, message);

        if (message == "exit") {
            std::cout << "Exiting...\n";
            send(sock, message.c_str(), message.size(), 0); // 发送退出消息
            break; // 用户输入 exit，退出循环
        }

        // 发送消息到服务器
        send(sock, message.c_str(), message.size(), 0);
        std::cout << "Message sent to server.\n";

        // 接收服务器的响应
        memset(buffer, 0, sizeof(buffer)); // 清空缓冲区
        int bytes_received = recv(sock, buffer, sizeof(buffer), 0);
        if (bytes_received <= 0) {
            std::cout << "Server disconnected or error occurred.\n";
            break; // 服务器断开连接或发生错误，退出循环
        }

        std::cout << "Response from server: " << buffer << "\n";
    }

    // 关闭连接
    closesocket(sock);
    WSACleanup();

    return 0;
}