
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

#if 0 //调用方式
// 继承自 OSAThread 的具体线程类
class MyThread : public OSAThread
{
public:
    MyThread() {}
    virtual ~MyThread() {}

    int run() override {
        for (int i = 0; i < 10; ++i) {
            std::cout << "Thread running: " << i << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 模拟工作
        }
        return 0;
    }
};

// 主函数
int main()
{
    MyThread myThread; // 创建线程实例
    myThread.start();  // 启动线程

    // 等待线程完成
    myThread.join();

    std::cout << "Thread has finished." << std::endl;
    return 0;
}
#endif

