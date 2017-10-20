#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <atomic>
#include <vector>
#include <deque>
 
class ThreadPool;
  
// our worker thread objects
class Worker {
public:
    Worker(ThreadPool &s) : pool(s) { }
    void operator()();
private:
    ThreadPool &pool;
};
  
// the actual thread pool
class ThreadPool {
public:
    ThreadPool(size_t);
    template<class F>
    void enqueue(F f);
    ~ThreadPool();
private:
    friend class Worker;
 
    // need to keep track of threads so we can join them
    std::vector< std::thread > workers;
 
    // the task queue
    std::deque< std::function<void()> > tasks;
 
    // synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
};

void Worker::operator()()
{
    std::function<void()> task;
    while(true)
    {
        {   // acquire lock
            std::unique_lock<std::mutex> 
                lock(pool.queue_mutex);
             
            // look for a work item
            while(!pool.stop && pool.tasks.empty())
            { // if there are none wait for notification
                pool.condition.wait(lock);
            }
 
            if(pool.stop) // exit if the pool is stopped
                return;
 
            // get the task from the queue
            task = pool.tasks.front();
            pool.tasks.pop_front();
 
        }   // release lock
 
        // execute the task
        task();
    }
}

// the constructor just launches some amount of workers
ThreadPool::ThreadPool(size_t threads)
    :   stop(false)
{
    for(size_t i = 0;i<threads;++i)
        workers.push_back(std::thread(Worker(*this)));
}
   
// the destructor joins all threads
ThreadPool::~ThreadPool()
{
    // stop all threads
    stop = true;
    condition.notify_all();
     
    // join them
    for(size_t i = 0;i<workers.size();++i)
        workers[i].join();
}

// add new work item to the pool
template<class F>
void ThreadPool::enqueue(F f)
{
    { // acquire lock
        std::unique_lock<std::mutex> lock(queue_mutex);
         
        // add the task
        tasks.push_back(std::function<void()>(f));
    } // release lock
     
    // wake up one thread
    condition.notify_one();
}

int main()
{
    // create a thread pool of 4 worker threads
    ThreadPool pool(4);
     
    // queue a bunch of "work items"
    for(int i = 0;i<8;++i)
    {
        pool.enqueue([i]
        {
            std::cout << "hello " << i << std::endl;
            
            
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            std::cout << "world " << i << std::endl;
        });
    }
    return 0;
}

/*
consider this:
you have one worker thread. It is sitting right past the line
¡°while(!pool.stop && pool.tasks.empty())¡±
so it is inside the for loop. Now the thread that owns the ThreadPool wants to destroy the ThreadPool object and calls the destructor. So now it calls:
¡°stop = true;
condition.notify_all();¡±
Now the worker thread continues. It will now do
¡°pool.condition.wait(lock);¡±
Since this wait happens after the notify, the thread misses the notify_all! But there won¡¯t be another one thus the worker thread will never terminate and the other thread joins forever in the destructor!
*/