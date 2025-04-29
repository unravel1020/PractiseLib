#include <iostream>
#include <vector>             // 用于存储线程集合
#include <queue>              // 用于任务队列
#include <thread>             // 用于创建和管理线程
#include <mutex>              // 用于互斥锁
#include <condition_variable> // 用于线程间的同步
#include <functional>         // 用于 std::function

class ThreadPool
{
public:
    // 构造函数：初始化线程池
    ThreadPool(size_t num_threads) : stop(false)
    {
        for (size_t i = 0; i < num_threads; ++i)
        {
            workers.emplace_back([this]
                                 {
                // 每个线程启动时打印其 ID
                std::cout << "Thread " << std::this_thread::get_id() << " is starting..." << std::endl;

                // 线程的主要工作循环
                while (true) {
                    std::function<void()> task; // 定义一个任务（可调用对象）
                    {
                        std::unique_lock<std::mutex> lock(this->queue_mutex); // 加锁保护任务队列
                        // 等待条件变量的通知，直到满足以下任一条件：
                        // 1. 线程池停止运行 (stop == true)
                        // 2. 任务队列不为空 (tasks.empty() == false)
                        this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });

                        if (this->stop && this->tasks.empty()) {
                            // 如果线程池停止且任务队列为空，则退出线程
                            std::cout << "Thread " << std::this_thread::get_id() << " is exiting..." << std::endl;
                            return;
                        }

                        // 从任务队列中取出一个任务
                        task = std::move(this->tasks.front());
                        this->tasks.pop();
                    } // 释放锁

                    // 执行任务
                    task();
                } });
        }
    }

    // 析构函数：销毁线程池
    ~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex); // 加锁保护任务队列
            stop = true;                                    // 设置停止标志
        }
        condition.notify_all(); // 唤醒所有等待的线程
        for (std::thread &worker : workers)
        {
            worker.join(); // 等待所有线程完成
        }
    }

    // 添加任务到线程池
    template <class F>
    void enqueue(F &&f)
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex); // 加锁保护任务队列
            tasks.emplace(std::forward<F>(f));              // 将任务添加到队列中
        }
        condition.notify_one(); // 唤醒一个等待的线程来执行任务
    }

private:
    std::vector<std::thread> workers;        // 工作线程集合
    std::queue<std::function<void()>> tasks; // 任务队列，存储待执行的任务
    std::mutex queue_mutex;                  // 互斥锁，用于保护任务队列
    std::condition_variable condition;       // 条件变量，用于线程间的同步
    bool stop;                               // 是否停止线程池的标志
};

// 创建一个全局互斥锁，用于保护 std::cout
std::mutex cout_mutex;

// 线程安全的日志输出函数
void safe_print(const std::string &message)
{
    std::lock_guard<std::mutex> lock(cout_mutex); // 使用互斥锁保护 std::cout
    std::cout << message << std::endl;
}

int main()
{
    ThreadPool pool(4); // 创建一个包含 4 个工作线程的线程池

    for (int i = 0; i < 100; ++i)
    {
        pool.enqueue([i]
                     {
            // 使用线程安全的日志函数输出信息
            safe_print("Task " + std::to_string(i) + " is running on thread " +
                       std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id()))); });
    }

    return 0;
}