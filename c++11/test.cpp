
#include "ThreadPool.hpp"
using namespace MyNamespace;

int main()
{
    
    //Timer saturationTimer;
    //const auto startTime = saturationTimer.tick();
    std::vector<ThreadPool::TaskFuture<void>> v;
    for(std::uint32_t i = 0u; i < 21u; ++i)
    {
        v.push_back(DefaultThreadPool::submitJob([]()
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }));
    }
    for(auto& item: v)
    {
        item.get();
    }
    //const auto dt = saturationTimer.tick() - startTime;
    return 0;
}