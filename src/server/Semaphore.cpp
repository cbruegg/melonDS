//
// Created by mail on 08/05/2020.
//

#include <condition_variable>
#include "Semaphore.h"

void Semaphore::notify() {
    std::unique_lock<std::mutex> lock(mtx);
    count++;
    //notify the waiting thread
    cv.notify_one();
}

void Semaphore::wait() {
    std::unique_lock<std::mutex> lock(mtx);
    while (count == 0) {
        //wait on the mutex until notify is called
        cv.wait(lock);
    }
    count--;
}

void Semaphore::reset() {
    std::unique_lock<std::mutex> lock(mtx);
    lock.lock();
    count = 0;
    lock.release();
}
