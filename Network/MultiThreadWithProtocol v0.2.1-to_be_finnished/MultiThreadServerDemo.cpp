#include <string>
#include <vector>
#include <csignal>
#include <atomic>
#include <iostream>
#include <winsock2.h>
#include <thread>
#include <chrono>

#include "Logger.h"
#include "Protocol.h"
#include "Shared.h"
#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define RECV_TIMEOUT_MS 5000

// 引入命名空间
using namespace net::logging;
using namespace net::protocol;

struct ClientContext;
std::vector<ClientContext*> ClientContextPool::pool;
std::mutex ClientContextPool::poolMutex;

std::vector<std::thread> threads;
SOCKET server_fd;

struct ClientContext {
    SOCKET socket;
    CRITICAL_SECTION lock; // 线程安全访问
    std::deque<std::string> sendQueue;
    std::string pendingData; 
    bool sending;

    // 接收相关
    WSABUF wsaBuf; // rename from recvBuf
    char buffer[4096]; // rename from recvBuffer
    DWORD bytesTransferred;
    DWORD flags;
    OVERLAPPED overlapped; // rename from recvOverlapped

    ClientContext() : socket(INVALID_SOCKET), sending(false) {
        InitializeCriticalSection(&lock);
    }

    ~ClientContext() {
        DeleteCriticalSection(&lock);
    }
};

void post_send(ClientContext* ctx, const std::string& msg) {
    EnterCriticalSection(&ctx->lock);
    if (!msg.empty()) {
        ctx->sendQueue.push_back(msg);
    }

    if (ctx->sendQueue.empty()) {
        LeaveCriticalSection(&ctx->lock);
        return;
    }
    ctx->sending = true;

    // 如果当前没有发送进行，则启动一次异步发送
    const std::string& data = ctx->sendQueue.front();
    ZeroMemory(&ctx->sendOverlapped, sizeof(OVERLAPPED));
    size_t len = data.size();
    if (len > sizeof(ctx->sendBuffer)) {
        LOG(LogLevel::ERR) << "Message too large to send.";
        ctx->sending = false;
        LeaveCriticalSection(&ctx->lock);
        return;
    }
    memcpy(ctx->sendBuffer, data.data(), len); // 拷贝进 sendBuffer
    ctx->sendBuf.buf = ctx->sendBuffer;
    ctx->sendBuf.len = static_cast<ULONG>(len);

    DWORD bytesSent;
    int result = WSASend(ctx->socket, &ctx->sendBuf, 1, &bytesSent, 0, &ctx->sendOverlapped, NULL);
    if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        LOG(LogLevel::ERR) << "WSASend failed: " << WSAGetLastError();
        ctx->sending = false;
        close_client(ctx);
    }
    LeaveCriticalSection(&ctx->lock);
}

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

void process_message(ClientContext* ctx, MessageType type, const std::string& data) {
    switch (type) {
        case MessageType::DATA:
            LOG(LogLevel::INFO) << "Received data: " << data.substr(0, 100);
            post_send(ctx, Protocol::serialize(MessageType::DATA, "Hello from server!"));
            break;

        case MessageType::PING:
            LOG(LogLevel::INFO) << "Received PING, sending PONG.";
            post_send(ctx, Protocol::serialize(MessageType::PONG, ""));
            break;

        default:
            LOG(LogLevel::WARNING) << "Unknown message type: " << static_cast<int>(type);
            break;
    }
}
void handle_client_io(ClientContext* ctx, DWORD bytesReceived) {
    update_client_activity(ctx->socket);
    ctx->pendingData.append(ctx->buffer, bytesReceived);

    while (ctx->pendingData.size() >= sizeof(MessageHeader)) {
        MessageHeader* header = reinterpret_cast<MessageHeader*>(ctx->pendingData.data());
        uint32_t payloadLen = ntohl(header->length);
        size_t totalSize = sizeof(MessageHeader) + payloadLen;

        if (ctx->pendingData.size() < totalSize)
            break;

        MessageType type;
        std::string data;
        if (Protocol::deserialize(ctx->pendingData.data(), totalSize, type, data)) {
            process_message(ctx, type, data);
        } else {
            LOG(LogLevel::ERR) << "Deserialize failed.";
        }

        ctx->pendingData.erase(0, totalSize);
    }

    post_recv(ctx);
}
void post_recv(ClientContext* ctx) {
    ZeroMemory(&ctx->recvOverlapped, sizeof(OVERLAPPED));
    ctx->recvBuf.buf = ctx->buffer;
    ctx->recvBuf.len = sizeof(ctx->buffer);
    ctx->recvFlags = 0;

    DWORD bytesRecv;
    int result = WSARecv(ctx->socket, &ctx->recvBuf, 1, &bytesRecv, &ctx->recvFlags,
                         &ctx->recvOverlapped, NULL);
    if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
        LOG(LogLevel::ERR) << "WSARecv failed: " << WSAGetLastError();
        ClientContextPool::release(ctx);
    }
}

std::map<SOCKET, std::chrono::steady_clock::time_point> activeClients;

void update_client_activity(SOCKET sock) {
    std::lock_guard<std::mutex> lock(clientMutex);
    activeClients[sock] = std::chrono::steady_clock::now();
}

void check_client_timeouts() {
    auto now = std::chrono::steady_clock::now();
    std::vector<SOCKET> toRemove;

    for (auto& [sock, lastActive] : activeClients) {
        if ((now - lastActive) > std::chrono::seconds(10)) {
            toRemove.push_back(sock);
        }
    }

    for (auto sock : toRemove) {
        LOG(LogLevel::INFO) << "Client timeout: " << sock;
        closesocket(sock);
        activeClients.erase(sock);
    }
}

class ClientContextPool {
    public:
        static const size_t MAX_POOL_SIZE = 1024; // 最大缓存 1024 个上下文
    
        static ClientContext* get() {
            std::lock_guard<std::mutex> lock(poolMutex);
            if (!pool.empty()) {
                auto* ctx = pool.back();
                pool.pop_back();
                return ctx;
            }
            return new ClientContext();
        }
    
        static void release(ClientContext* ctx) {
            std::lock_guard<std::mutex> lock(poolMutex);
            if (pool.size() < MAX_POOL_SIZE) {
                pool.push_back(ctx);
            } else {
                delete ctx;
            }
        }
    
    private:
        static std::vector<ClientContext*> pool;
        static std::mutex poolMutex;
    };

    void close_client(ClientContext* ctx) {
        closesocket(ctx->socket);
        ClientContextPool::release(ctx);
    }
    
int main() {
    Logger::init("server", Logger::Level::INFO, 7);
    LOG(LogLevel::INFO) << "Starting server...";

    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    signal(SIGINT, signal_handler);

    // 创建完成端口
    HANDLE ioCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);

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

    // 将监听套接字绑定到完成端口
    CreateIoCompletionPort((HANDLE)serverSocket, ioCompletionPort, (ULONG_PTR)NULL, 0);
    
    std::thread([=]() {
        while (is_running) {
            check_client_timeouts();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }).detach();

    // 启动多个工作线程
    const int numThreads = std::thread::hardware_concurrency();
    for (int i = 0; i < numThreads; ++i) {
        std::thread([ioCompletionPort]() {
            while (true) {
                DWORD transferred;
                ULONG_PTR key;
                LPOVERLAPPED overlapped;
                if (!GetQueuedCompletionStatus(ioCompletionPort, &transferred, &key, &overlapped, INFINITE)) {
                    LOG(LogLevel::ERR) << "IOCP error in worker thread.";
                    continue;
                }

                if (key == 0) {
                    // 新连接事件
                    SOCKET clientSocket = (SOCKET)overlapped;
                    LOG(LogLevel::INFO) << "New client connected: " << clientSocket;

                    // 投递接收请求
                    auto* ctx =  ClientContextPool::get();
                    ctx->socket = clientSocket;
                    ctx->wsaBuf.buf = ctx->buffer;
                    ctx->wsaBuf.len = sizeof(ctx->buffer);
                    ctx->flags = 0;
                    ZeroMemory(&ctx->overlapped, sizeof(OVERLAPPED));

                    WSARecv(clientSocket, &ctx->wsaBuf, 1, &ctx->bytesTransferred, &ctx->flags,
                            &ctx->overlapped, NULL);

                    // 绑定到完成端口
                    CreateIoCompletionPort((HANDLE)clientSocket, ioCompletionPort, (ULONG_PTR)ctx, 0);
                } else {
                    // 处理客户端数据
                    auto* ctx = reinterpret_cast<ClientContext*>(key);
                    if (transferred == 0) {
                        LOG(LogLevel::INFO) << "Client disconnected: " << ctx->socket;
                        ClientContextPool::release(ctx);
                        continue;
                    }

                    if(ctx->sending){
                        //发送完成，尝试下一个
                        EnterCriticalSection(&ctx->lock);
                        ctx->sendQueue.pop_front();
                        if(!ctx->sendQueue.empty()){
                            //队列还有数据，发送下一个
                            const std::string& data = ctx->sendQueue.front();
                            size_t len = data.size();
                            if(len <= sizeof(ctx->sendBuffer)){
                                memcpy(ctx->sendBuffer, data.data(), len);                                ctx->sendBuf.buf = ctx->sendBuffer;
                                ctx->sendBuf.len = static_cast<ULONG>(len);

                                DWORD bytesSent;
                                int result = WSASend(ctx->socket, &ctx->sendBuf, 1, &bytesSent, 0, &ctx->sendOverlapped, NULL);
                                if(result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING){ 
                                    LOG(LogLevel::ERR) << "WSASend failed with error: " << WSAGetLastError();
                                    ctx->sending = false;
                                    close_client(ctx);
                                }
                            }else{
                                LOG(LogLevel::ERR) << "Next message too large " << WSAGetLastError();
                                ctx->sending = false;
                                close_client(ctx);
                            }
                        }else{
                            ctx->sending = false;
                        }
                        LeaveCriticalSection(&ctx->lock);
                    }else{
                        // 解析协议并响应
                        handle_client_io(ctx, transferred);
                    }
                }
            }
        }).detach();
    }

    // 循环等待 accept
    while (is_running) {
        SOCKET clientSocket;
        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &addrLen);
        if (clientSocket != INVALID_SOCKET) {
            std::string ip = inet_ntoa(clientAddr.sin_addr);
            LOG(LogLevel::INFO) << "New client connected from " << ip << ":" << ntohs(clientAddr.sin_port);
            PostQueuedCompletionStatus(ioCompletionPort, 0, 0, (LPOVERLAPPED)clientSocket);
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