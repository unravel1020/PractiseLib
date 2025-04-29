#include <iostream>
#include <thread>
#include <mutex>
#include<condition_variable>

class ReadWriteLock {
public:
    void lock_read() {
        std::unique_lock<std::mutex> lock(mutex_);
        while (writers_ > 0) {
            reader_condition_.wait(lock);
        }
        readers_++;
    }

    void unlock_read() {
        std::lock_guard<std::mutex> lock(mutex_);
        readers_--;
        if (readers_ == 0) {
            writer_condition_.notify_one();
        }
    }

    void lock_write() {
        std::unique_lock<std::mutex> lock(mutex_);
        writers_++;
        while (readers_ > 0 || active_writer_) {
            writer_condition_.wait(lock);
        }
        active_writer_ = true;
    }

    void unlock_write() {
        std::lock_guard<std::mutex> lock(mutex_);
        writers_--;
        active_writer_ = false;
        if (writers_ > 0) {
            writer_condition_.notify_one();
        } else {
            reader_condition_.notify_all();
        }
    }

private:
    std::mutex mutex_; //共享锁
    std::condition_variable reader_condition_;  //读者线程锁
    std::condition_variable writer_condition_;  //写者线程锁
    int readers_ = 0;   //读者线程数
    int writers_ = 0;   //写者线程数
    bool active_writer_ = false;    //是否有活跃的写者（用来限定活跃的写者只有一个）
};

ReadWriteLock rw_mutex;
int shared_data = 0;

void reader(int id) {
    rw_mutex.lock_read();
    std::cout << "Reader " << id << " reads " << shared_data << std::endl;
    rw_mutex.unlock_read();
}

void writer(int value) {
    rw_mutex.lock_write();
    shared_data = value;
    std::cout << "Writer writes " << shared_data << std::endl;
    rw_mutex.unlock_write();
}

int main() {
    // 启动多个读者线程
    std::thread t1(reader, 1), t2(reader, 2), t3(reader, 3);

    // 启动一个写者线程
    std::thread tw(writer, 42);

    t1.join();
    t2.join();
    t3.join();
    tw.join();

    return 0;
}