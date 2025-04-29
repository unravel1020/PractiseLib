#include <iostream>
#include <thread>
#include <mutex>

std::mutex mutex1;
std::mutex mutex2;


//1.死锁demo（互相竞争并且不退让导致死锁）
// void thread1() {
//     std::lock_guard<std::mutex> lock1(mutex1);
//     std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 引入延迟
//     std::lock_guard<std::mutex> lock2(mutex2); // 尝试获取第二个锁
//     std::cout << "Thread 1 completes." << std::endl;
// }

// void thread2() {
//     std::lock_guard<std::mutex> lock2(mutex2);
//     std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 引入延迟
//     std::lock_guard<std::mutex> lock1(mutex1); // 尝试获取第一个锁
//     std::cout << "Thread 2 completes." << std::endl;
// }

//2.解决办法1.固定顺序获取锁（确保所有线程以相同顺序获取锁，即可避免死锁，因为总有个领先的会率先抵达终点并释放所有锁，然后第二名再完成，第三名...依次类推
// void thread1() {
//     std::lock_guard<std::mutex> lock1(mutex1);
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     std::lock_guard<std::mutex> lock2(mutex2);
//     std::cout << "Thread 1 completes." << std::endl;
// }

// void thread2() {
//     std::lock_guard<std::mutex> lock1(mutex1); // 按相同顺序获取锁
//     std::this_thread::sleep_for(std::chrono::milliseconds(100));
//     std::lock_guard<std::mutex> lock2(mutex2);
//     std::cout << "Thread 2 completes." << std::endl;
// }



//解决办法2.一次锁定多个锁（仍有风险，原理只是让更快的线程尽可能一次抢走所有锁，这样后面的线程就不会再进入竞争锁的队列了，直到前一个线程完成了所有操作i释放所有锁为止。
void thread1() {
    std::lock(mutex1, mutex2); // 同时锁定两个锁
    std::lock_guard<std::mutex> lock1(mutex1, std::adopt_lock);
    std::lock_guard<std::mutex> lock2(mutex2, std::adopt_lock);
    std::cout << "Thread 1 completes." << std::endl;
}

void thread2() {
    std::lock(mutex1, mutex2); // 同时锁定两个锁
    std::lock_guard<std::mutex> lock1(mutex1, std::adopt_lock);
    std::lock_guard<std::mutex> lock2(mutex2, std::adopt_lock);
    std::cout << "Thread 2 completes." << std::endl;
}

int main() {
    std::thread t1(thread1);
    std::thread t2(thread2);

    t1.join();
    t2.join();

    return 0;
}