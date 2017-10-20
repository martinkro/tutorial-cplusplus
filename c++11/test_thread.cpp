// g++ -std=c++11 test_thread.cpp

#include <thread>
#include <iostream>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <chrono>

void hello(){
    std::cout << "Hello from thread " << std::endl;
}

void test1()
{
    std::thread t1(hello);
    t1.join();
}
void test2()
{
    std::vector<std::thread> threads;
    for(int i = 0; i < 5; ++i){
        threads.push_back(std::thread([](){
            std::cout << "Hello from thread " << std::this_thread::get_id() << std::endl;
        }));
    }
    for(auto& thread : threads){
        thread.join();
    }
}

struct Counter {
    int value;
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
    Counter() : value(0) {}
    void increment(){
        ++value;
    }
    void decrement(){
        if(value == 0){
            throw "Value cannot be less than 0";
        }
        --value;
    }
};

// Wrapper
struct ConcurrentSafeCounter {
    std::mutex mutex;
    Counter counter;
    void increment(){
        // 智能锁
        std::lock_guard<std::mutex> guard(mutex);
        counter.increment();
    }
    void decrement(){
        std::lock_guard<std::mutex> guard(mutex);
        counter.decrement();
    }
    
    int value()
    {
        return counter.value;
    }
};

void test3()
{
    Counter counter;
    counter.value = 0;
    std::vector<std::thread> threads;
    for(int i = 0; i < 5; ++i){
        threads.push_back(std::thread([&counter](){
            for(int i = 0; i < 10000; ++i){
                counter.increment();
            }
        }));
    }
    for(auto& thread : threads){
        thread.join();
    }
    std::cout << counter.value << std::endl;
}

void test4()
{
    ConcurrentSafeCounter counter;
    
    std::vector<std::thread> threads;
    for(int i = 0; i < 5; ++i){
        threads.push_back(std::thread([&counter](){
            for(int i = 0; i < 10000; ++i){
                counter.increment();
            }
        }));
    }
    for(auto& thread : threads){
        thread.join();
    }
    std::cout << counter.value() << std::endl;
}

// 递归锁 能够被同一个线程重复上锁
struct Complex {
    std::recursive_mutex mutex;
    int i;
    Complex() : i(0) {}
    void mul(int x){
        std::lock_guard<std::recursive_mutex> lock(mutex);
        i *= x;
    }
    void div(int x){
        std::lock_guard<std::recursive_mutex> lock(mutex);
        i /= x;
    }
    void both(int x, int y){
        std::lock_guard<std::recursive_mutex> lock(mutex);
        mul(x);
        div(y);
    }
};

// 计时锁 GCC 4.9不支持
/*
struct TestTimeMutex
{
    std::timed_mutex mutex;
    void work(){
        std::chrono::milliseconds timeout(100);
        while(true){
            if(mutex.try_lock_for(timeout)){
                std::cout << std::this_thread::get_id() << ": do work with the mutex" << std::endl;
                std::chrono::milliseconds sleepDuration(250);
                std::this_thread::sleep_for(sleepDuration);
                mutex.unlock();
                std::this_thread::sleep_for(sleepDuration);
            } else {
                std::cout << std::this_thread::get_id() << ": do work without mutex" << std::endl;
                std::chrono::milliseconds sleepDuration(100);
                std::this_thread::sleep_for(sleepDuration);
            }
        }
    }
};
*/

// Call once
std::once_flag flag;
void do_something(){
    std::call_once(flag, [](){std::cout << "Called once" << std::endl;});
    std::cout << "Called each time" << std::endl;
}

void test_call_once()
{
    std::thread t1(do_something);
    std::thread t2(do_something);
    std::thread t3(do_something);
    std::thread t4(do_something);
    t1.join();
    t2.join();
    t3.join();
    t4.join();
    //return 0;
}

// 条件变量 生成者 消费者
struct BoundedBuffer {
    int* buffer;
    int capacity;
    int front;
    int rear;
    int count;
    std::mutex lock;
    std::condition_variable not_full;
    std::condition_variable not_empty;
    BoundedBuffer(int capacity) : capacity(capacity), front(0), rear(0), count(0) {
        buffer = new int[capacity];
    }
    ~BoundedBuffer(){
        delete[] buffer;
    }
    void deposit(int data){
        std::unique_lock<std::mutex> l(lock);
        not_full.wait(l, [this](){return this->count != this->capacity; });
        buffer[rear] = data;
        rear = (rear + 1) % capacity;
        ++count;
        not_empty.notify_one();
    }
    int fetch(){
        std::unique_lock<std::mutex> l(lock);
        not_empty.wait(l, [this](){return this->count != 0; });
        int result = buffer[front];
        front = (front + 1) % capacity;
        --count;
        not_full.notify_one();
        return result;
    }
};
void consumer(int id, BoundedBuffer& buffer){
    for(int i = 0; i < 50; ++i){
        int value = buffer.fetch();
        std::cout << "Consumer " << id << " fetched " << value << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }
}
void producer(int id, BoundedBuffer& buffer){
    for(int i = 0; i < 75; ++i){
        buffer.deposit(i);
        std::cout << "Produced " << id << " produced " << i << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
int test_condition_variable(){
    BoundedBuffer buffer(200);
    std::thread c1(consumer, 0, std::ref(buffer));
    std::thread c2(consumer, 1, std::ref(buffer));
    std::thread c3(consumer, 2, std::ref(buffer));
    std::thread p1(producer, 0, std::ref(buffer));
    std::thread p2(producer, 1, std::ref(buffer));
    c1.join();
    c2.join();
    c3.join();
    p1.join();
    p2.join();
    return 0;
}

// atomic what is lock-free
struct AtomicCounter {
    std::atomic<int> value;

    void increment(){
        ++value;
    }

    void decrement(){
        --value;
    }

    int get(){
        return value.load();
    }
};

// 异步任务 future
int test_future_void(){
    auto future = std::async(std::launch::async, [](){
        std::cout << "I'm a thread" << std::endl;
    });

    future.get();

    return 0;
}

// wait for
int test_future_1(){
    auto f1 = std::async(std::launch::async, [](){
        std::this_thread::sleep_for(std::chrono::seconds(9));
        return 42;
    });

    auto f2 = std::async(std::launch::async, [](){
        std::this_thread::sleep_for(std::chrono::seconds(3));
        return 13;
    });

    auto f3 = std::async(std::launch::async, [](){
        std::this_thread::sleep_for(std::chrono::seconds(6));
        return 666;
    });

    auto timeout = std::chrono::milliseconds(10);

    while(f1.valid() || f2.valid() || f3.valid()){
        if(f1.valid() && f1.wait_for(timeout) == std::future_status::ready){
            std::cout << "Task1 is done! " << f1.get() << std::endl;
        }

        if(f2.valid() && f2.wait_for(timeout) == std::future_status::ready){
            std::cout << "Task2 is done! " << f2.get() << std::endl;
        }

        if(f3.valid() && f3.wait_for(timeout) == std::future_status::ready){
            std::cout << "Task3 is done! " << f3.get() << std::endl;
        }

        std::cout << "I'm doing my own work!" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "I'm done with my own work!" << std::endl;
    }

    std::cout << "Everything is done, let's go back to the tutorial" << std::endl;

    return 0;
}

// paramter
int test_future_2(){
    std::vector<std::future<size_t>> futures;

    for (size_t i = 0; i < 10; ++i) {
        futures.emplace_back(std::async(std::launch::async, [](size_t param){
            std::this_thread::sleep_for(std::chrono::seconds(param));
            return param;
        }, i));
    }

    std::cout << "Start querying" << std::endl;

    for (auto &future : futures) {
      std::cout << future.get() << std::endl;
    }

    return 0;
}

int main(){
    
    //test2();
    //test3();
    //test_call_once();
    //test_condition_variable();
    test_future_void();
    test_future_1();
    test_future_2();
    return 0;
}