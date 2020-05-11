//
// Created by mail on 11/05/2020.
//

#ifndef MELONDS_QUEUE_H
#define MELONDS_QUEUE_H


#include <mutex>
#include <condition_variable>
#include <deque>

template<typename T>
class Queue {
private:
    std::mutex d_mutex;
    std::condition_variable d_condition;
    std::deque<T> d_queue;
public:
    void push(T const &value);

    T pop();
};

#endif //MELONDS_QUEUE_H
