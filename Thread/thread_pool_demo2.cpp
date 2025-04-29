#include <iostream>
#include <vector>
#include <deque>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

// 定义任务结构体，包含优先级和实际任务
struct Task
{
    int priority; // 越小优先级越高
    std::function<void()> task;

    // 重载小于运算符，使得优先级队列能够根据优先级排序
    bool operator<(const Task &other) const
    {
        return priority > other.priority; // 注意这里是 >，因为我们希望优先级越小排在前面
    }
};

class WorkStealingPriorityThreadPool
{
public:
    // 构造函数，初始化线程池
    WorkStealingPriorityThreadPool(size_t num_threads) : stop(false)
    {
        tasks.resize(num_threads); // 初始化任务队列
        queue_mutexes.resize(num_threads); // 初始化互斥锁
        condition_variables.resize(num_threads); // 初始化条件变量

        for (size_t i = 0; i < num_threads; ++i)
        {
            workers.emplace_back([this, i]
                                 {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(queue_mutexes[i]);
                        if (!getTask(i, task)) {
                            condition_variables[i].wait(lock, [this, i] { 
                                return stop || !tasks[i].first.empty() || !tasks[i].second.empty(); 
                            });
                            if (stop && tasks[i].first.empty() && tasks[i].second.empty()) {
                                return;
                            }
                            getTask(i, task);
                        }
                    }
                    if (task) {
                        task(); // 执行任务
                    }
                } });
        }
    }

    // 析构函数，确保所有线程安全退出
    ~WorkStealingPriorityThreadPool()
    {
        stop = true;
        for (auto &cv : condition_variables)
        {
            cv.notify_all(); // 唤醒所有等待的线程
        }
        for (std::thread &worker : workers)
        {
            if (worker.joinable())
            {
                worker.join(); // 确保线程安全退出
            }
        }
    }

    // 提交任务到线程池
    void enqueue(int priority, std::function<void()> f)
    {
        size_t target_thread = std::hash<std::thread::id>{}(std::this_thread::get_id()) % tasks.size();

        {
            std::lock_guard<std::mutex> lock(queue_mutexes[target_thread]);
            tasks[target_thread].first.push({priority, std::move(f)}); // 将任务加入优先级队列
        }
        condition_variables[target_thread].notify_one(); // 通知对应线程有新任务
    }

private:
    // 获取任务（优先从高优先级队列获取，其次从窃取队列获取）
    bool getTask(size_t current_thread, std::function<void()> &out_task)
    {
        if (!tasks[current_thread].first.empty())
        {
            out_task = std::move(tasks[current_thread].first.top().task);
            tasks[current_thread].first.pop();
            return true;
        }
        else if (!tasks[current_thread].second.empty())
        {
            out_task = std::move(tasks[current_thread].second.front());
            tasks[current_thread].second.pop_front();
            return true;
        }
        else if (getTaskFromOtherQueues(current_thread, out_task))
        {
            return true;
        }
        return false;
    }

    // 从其他线程的任务队列中窃取任务
    bool getTaskFromOtherQueues(size_t current_thread, std::function<void()> &out_task)
    {
        for (size_t i = 0; i < tasks.size(); ++i)
        {
            if (i == current_thread)
                continue;

            std::unique_lock<std::mutex> lock(queue_mutexes[i], std::try_to_lock);
            if (lock.owns_lock() && !tasks[i].second.empty())
            {
                out_task = std::move(tasks[i].second.back());
                tasks[i].second.pop_back();
                return true;
            }
        }
        return false;
    }

    struct ThreadTasks
    {
        std::priority_queue<Task> first;          // 高优先级队列
        std::deque<std::function<void()>> second; // 窃取队列
    };

    std::vector<std::thread> workers;                         // 工作线程
    std::vector<ThreadTasks> tasks;                           // 每个线程的任务队列
    std::vector<std::mutex> queue_mutexes;                    // 每个线程的任务队列锁
    std::vector<std::condition_variable> condition_variables; // 条件变量，用于线程间通信
    std::atomic<bool> stop;                                   // 标志位，用于控制线程池停止
};

int main()
{
    WorkStealingPriorityThreadPool pool(4);

    // 测试样例 1：提交不同优先级的任务
    pool.enqueue(1, []
                 { std::cout << "High priority task\n"; });
    pool.enqueue(3, []
                 { std::cout << "Low priority task\n"; });
    pool.enqueue(2, []
                 { std::cout << "Medium priority task\n"; });

    // 测试样例 2：提交多个高优先级任务
    for (int i = 0; i < 5; ++i)
    {
        pool.enqueue(1, [i]
                     { std::cout << "High priority task " << i << "\n"; });
    }

    // 测试样例 3：提交多个低优先级任务
    for (int i = 0; i < 5; ++i)
    {
        pool.enqueue(5, [i]
                     { std::cout << "Low priority task " << i << "\n"; });
    }

    // 测试样例 4：模拟长时间运行的任务
    pool.enqueue(1, []
                 {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "Long-running high priority task completed\n"; });

    pool.enqueue(5, []
                 {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Long-running low priority task completed\n"; });

    // 等待所有任务完成
    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}