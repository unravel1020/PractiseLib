#include <iostream>
#include <vector>
#include <deque>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

// ��������ṹ�壬�������ȼ���ʵ������
struct Task
{
    int priority; // ԽС���ȼ�Խ��
    std::function<void()> task;

    // ����С���������ʹ�����ȼ������ܹ��������ȼ�����
    bool operator<(const Task &other) const
    {
        return priority > other.priority; // ע�������� >����Ϊ����ϣ�����ȼ�ԽС����ǰ��
    }
};

class WorkStealingPriorityThreadPool
{
public:
    // ���캯������ʼ���̳߳�
    WorkStealingPriorityThreadPool(size_t num_threads) : stop(false)
    {
        tasks.resize(num_threads); // ��ʼ���������
        queue_mutexes.resize(num_threads); // ��ʼ��������
        condition_variables.resize(num_threads); // ��ʼ����������

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
                        task(); // ִ������
                    }
                } });
        }
    }

    // ����������ȷ�������̰߳�ȫ�˳�
    ~WorkStealingPriorityThreadPool()
    {
        stop = true;
        for (auto &cv : condition_variables)
        {
            cv.notify_all(); // �������еȴ����߳�
        }
        for (std::thread &worker : workers)
        {
            if (worker.joinable())
            {
                worker.join(); // ȷ���̰߳�ȫ�˳�
            }
        }
    }

    // �ύ�����̳߳�
    void enqueue(int priority, std::function<void()> f)
    {
        size_t target_thread = std::hash<std::thread::id>{}(std::this_thread::get_id()) % tasks.size();

        {
            std::lock_guard<std::mutex> lock(queue_mutexes[target_thread]);
            tasks[target_thread].first.push({priority, std::move(f)}); // ������������ȼ�����
        }
        condition_variables[target_thread].notify_one(); // ֪ͨ��Ӧ�߳���������
    }

private:
    // ��ȡ�������ȴӸ����ȼ����л�ȡ����δ���ȡ���л�ȡ��
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

    // �������̵߳������������ȡ����
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
        std::priority_queue<Task> first;          // �����ȼ�����
        std::deque<std::function<void()>> second; // ��ȡ����
    };

    std::vector<std::thread> workers;                         // �����߳�
    std::vector<ThreadTasks> tasks;                           // ÿ���̵߳��������
    std::vector<std::mutex> queue_mutexes;                    // ÿ���̵߳����������
    std::vector<std::condition_variable> condition_variables; // ���������������̼߳�ͨ��
    std::atomic<bool> stop;                                   // ��־λ�����ڿ����̳߳�ֹͣ
};

int main()
{
    WorkStealingPriorityThreadPool pool(4);

    // �������� 1���ύ��ͬ���ȼ�������
    pool.enqueue(1, []
                 { std::cout << "High priority task\n"; });
    pool.enqueue(3, []
                 { std::cout << "Low priority task\n"; });
    pool.enqueue(2, []
                 { std::cout << "Medium priority task\n"; });

    // �������� 2���ύ��������ȼ�����
    for (int i = 0; i < 5; ++i)
    {
        pool.enqueue(1, [i]
                     { std::cout << "High priority task " << i << "\n"; });
    }

    // �������� 3���ύ��������ȼ�����
    for (int i = 0; i < 5; ++i)
    {
        pool.enqueue(5, [i]
                     { std::cout << "Low priority task " << i << "\n"; });
    }

    // �������� 4��ģ�ⳤʱ�����е�����
    pool.enqueue(1, []
                 {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        std::cout << "Long-running high priority task completed\n"; });

    pool.enqueue(5, []
                 {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Long-running low priority task completed\n"; });

    // �ȴ������������
    std::this_thread::sleep_for(std::chrono::seconds(5));

    return 0;
}