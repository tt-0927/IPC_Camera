/*
 *  File Name: semaphore.h
 *  Created on: 2022年6月30日
 *  Author: zjc
 *  description: 信号量
 */

#ifndef BL_SHARE_SEMAPHORE_H
#define BL_SHARE_SEMAPHORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <mutex>
#include <condition_variable>
using namespace std;

class CSemaphore
{
public:
    CSemaphore(int cMax, int count = 0) : count(count) {}
    ~CSemaphore() {
        cond.notify_all();
    }

    //V操作，唤醒
    void release()
    {
        unique_lock<mutex> unique(mt);
        ++count;
        if (count > cMax) {
            count = cMax;
        }
        if (count <= 0)
            cond.notify_one();
    }
    //P操作，阻塞
    void acquire()
    {
        unique_lock<mutex> unique(mt);
        --count;
        if (count < 0)
            cond.wait(unique);
    }

private:
    mutex mt;
    condition_variable cond;
    int count;
    int cMax;
};
#ifdef __cplusplus
}
#endif

#endif
