
#include "osa_thread.h"


static void osa_thread(void *argv)
{
    OSAThread* ctx = static_cast<OSAThread*>(argv);
    ctx->run();
    return ;
}

OSAThread::OSAThread()
    :isRuning_(false)
{
}

OSAThread::~OSAThread()
{
}

int OSAThread::start()
{
    if(!isRuning_.load()){
        tid_ = std::thread(osa_thread,this);
        isRuning_.store(true);
    }
    return 0;
}

int OSAThread::detach()
{
    if(isRuning_.load()){
        tid_.detach();
    }
    return 0;
}

int OSAThread::join()
{
    if(isRuning_.load()){
        tid_.join();    //等待线程结束
        isRuning_.store(false);
    }
    return 0;
}




