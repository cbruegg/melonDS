//
// Created by mail on 08/05/2020.
//

#ifndef MELONDS_SEMAPHORE_H
#define MELONDS_SEMAPHORE_H

class Semaphore {
public:
    explicit Semaphore(int count_ = 0)
            : count(count_) {
    }

    void notify();

    void wait();

    void reset();

private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;
};

#endif //MELONDS_SEMAPHORE_H
