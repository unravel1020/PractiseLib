#include<iostream>
#include<thread>
#include<mutex>
#include<condition_variable>

std::mutex mtx;
std::condition_variable cv;
bool ready = false;

void printMessage(const std::string& message, bool isFirst)
{
    for(int i = 0; i < 5; ++i)
    {
        // // mtx.lock();  1.使用互斥锁
        // std::lock_guard<std::mutex> guard(mtx);  //2.std::lock_guard会在作用域中自动加解锁，作用域结束就解锁。
        std::unique_lock<std::mutex> lock(mtx);  //3.使用条件变量加互斥锁
        if(isFirst){
            cv.wait(lock, []{return !ready;});
        }else{
            cv.wait(lock, []{return ready;});
        }

        std::cout << message << " " << i << std::endl;
        
        ready = !ready; //3.切换ready状态
        
        cv.notify_all();  //3.
        lock.unlock(); //3.
        // mtx.unlock();  1.使用互斥锁
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

int main()
{
    std::thread t1(printMessage,"Thread 1",true);
    std::thread t2(printMessage,"Thread 2",false);

    t1.join();
    t2.join();

    std::cout << "ALL threads finished" << std::endl;
    return 0;
}

