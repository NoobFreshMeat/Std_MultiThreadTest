// main.cpp

#include <iostream>
#include <string>
#include <chrono>
#include <future>
#include <vector>
#include <thread>
#include "ThreadPool.h"

int main()
{
    ThreadPool threadPool(4);
    std::vector< std::future<std::string> > results;
    for (int i = 0; i < 8; ++i)
    {
         results.emplace_back(
             threadPool.enqueue(
                 [i]{
                     std::cout << "Hello " << i << "World" << std::endl;
                     std::this_thread::sleep_for(std::chrono::seconds(1));
                     std::cout << "thread " << i << "finished" << std::endl;
                 }
              )
         );
    }

    for(auto && result : results)
    {
        std::cout << result.get() << std::endl;
    }
    
    return 0;
}
