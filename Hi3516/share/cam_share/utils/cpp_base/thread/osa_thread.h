

#ifndef CORE_SOURCE_SHARE_CPP_OSA_BASE_INCLUDE_
#define CORE_SOURCE_SHARE_CPP_OSA_BASE_INCLUDE_


#include <iostream>
#include <thread>
#include <atomic>
#include <sys/prctl.h>
class OSAThread
{
public:
    OSAThread();
    ~OSAThread();

    virtual int run() = 0;
    int start();

    /**
     *  Detach the thread from the calling process.
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    int detach();

    /**
     *  Join the calling process with the thread
     *
     *  @return 0 if normal, -1 if errno set, errno code otherwise.
     */
    int join();

private:
    std::thread tid_;
    std::atomic<bool> isRuning_;

};



#endif //CORE_SOURCE_SHARE_CPP_OSA_BASE_INCLUDE_

