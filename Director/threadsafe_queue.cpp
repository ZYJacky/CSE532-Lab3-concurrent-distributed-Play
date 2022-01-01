// cpp on linux need both template declaration and definition to be included
#ifndef TEMPLATE_HEADERS_INCLUDE_SOURCE

#include "threadsafe_queue.h"

#endif /* TEMPLATE_HEADERS_INCLUDE_SOURCE */

template<typename T>
threadsafe_queue<T>::threadsafe_queue()
{}

// push and notify
template<typename T>
void threadsafe_queue<T>::push(T new_value)
{
    std::lock_guard<std::mutex> lk(mut);
    data_queue.push(std::move(new_value));
    data_cond.notify_one();                                    
}

// wait and pop until nofified if empty
template<typename T>
void threadsafe_queue<T>::wait_and_pop(T& value)                                    
{
    std::unique_lock<std::mutex> lk(mut);
    data_cond.wait(lk,[this]{return !data_queue.empty();});
    value = std::move(data_queue.front());
    data_queue.pop();
}

// wait and pop until nofified if empty
template<typename T>
std::shared_ptr<T> threadsafe_queue<T>::wait_and_pop()                              
{
    std::unique_lock<std::mutex> lk(mut);
    data_cond.wait(lk,[this]{return !data_queue.empty();});    
    std::shared_ptr<T> res(
        std::make_shared<T>(std::move(data_queue.front()))
    );
    data_queue.pop();
    return res;
}

// clear queue
template<typename T>
void threadsafe_queue<T>::clear()                              
{
    std::unique_lock<std::mutex> lk(mut);
    while(!data_queue.empty()) data_queue.pop();
}

// try popping an element, returen false if empty
template<typename T>
bool threadsafe_queue<T>::try_pop(T& value)
{
    std::lock_guard<std::mutex> lk(mut);
    if(data_queue.empty()) return false;
    value=std::move(data_queue.front());
    data_queue.pop();
    return true;
}

// try poping a element, default construct if empty
template<typename T>
std::shared_ptr<T> threadsafe_queue<T>::try_pop()
{
    std::lock_guard<std::mutex> lk(mut);
    if(data_queue.empty())
        return std::shared_ptr<T>();                           
    std::shared_ptr<T> res(
        std::make_shared<T>(std::move(data_queue.front())));
    data_queue.pop();
    return res;
}

// is the queue empty?
template<typename T>
bool threadsafe_queue<T>::empty() const
{
    std::lock_guard<std::mutex> lk(mut);
    return data_queue.empty();
}