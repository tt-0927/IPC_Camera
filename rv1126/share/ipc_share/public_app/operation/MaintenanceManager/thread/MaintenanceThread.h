/*
 * @FilePath     : MaintenanceThread.h
 * @Author       : xiezhh
 * @Date         : 2024-06-04 17:23
 * @LastEditors  : xiezhh
 * @LastEditTime : 2024-06-25 15:24
 * @Description  : 一个简单的线程抽象基类，子类必须实现run函数
 */
#ifndef MAINTENANCETHREAD_H
#define MAINTENANCETHREAD_H

#include <thread>
#include <atomic>

class CMaintenanceThread
{
public:
    CMaintenanceThread();
    ~CMaintenanceThread();

    /**
     * @brief 启动线程函数
     */
    void start();
    /**
     * @brief 停止线程函数
     */
    void stop();
    /**
     * @brief 当前线程是否正在运行
     * @return [bool] true：正在运行， false：已停止
     */
    bool isRuning();

private:
    /**
     * @brief 线程调用的静态函数
     * @param [void *] this指针
     */
    static void stdThreadRunFunction(void *pClass);

protected:
    /**
     * @brief 线程运行函数
     * @note 在这个函数中进行while循环
     */
    virtual void run() = 0;

protected:
    /* 线程指针 */
    std::thread *m_pThread = nullptr;
    /* 是否正在运行的标志 */
    std::atomic<bool> m_bIsRunFlag = false;
};

#endif // MAINTENANCETHREAD_H
