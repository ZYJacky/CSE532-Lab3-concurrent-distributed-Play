/*
 *   threadsafe_queue.h     
 *
 *   Declaration of a threadsafe queue inspired by Anthony Williams' C++ Concurrency in Action, Second Addition introduced in chapter six
 *
 *   Created: 9/16/21
 *   Last edited: 11/8/21
 */

#ifndef THREADSAFE_QUEUE_H

#define THREADSAFE_QUEUE_H

#include <mutex>
#include <queue>
#include <condition_variable>
#include <memory>

template<typename T>
class threadsafe_queue
{
    private:
        mutable std::mutex mut;
        std::queue<T> data_queue;
        std::condition_variable data_cond;

    public:
        threadsafe_queue();

        void push(T new_value);
        void wait_and_pop(T& value);                                    
        std::shared_ptr<T> wait_and_pop();                              
        bool try_pop(T& value);
        std::shared_ptr<T> try_pop();
        void clear();  
        bool empty() const;
};

#ifdef TEMPLATE_HEADERS_INCLUDE_SOURCE
#include "threadsafe_queue.cpp"
#endif

#endif  // THREADSAFE_QUEUE_H