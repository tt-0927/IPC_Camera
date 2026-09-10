#include "MaintenanceThread.h"

void CMaintenanceThread::stdThreadRunFunction(void *pClass)
{
    static_cast<CMaintenanceThread *>(pClass)->run();
}

CMaintenanceThread::CMaintenanceThread()
{
}

CMaintenanceThread::~CMaintenanceThread()
{
    if (isRuning())
    {
        stop();
    }
}

void CMaintenanceThread::start()
{
    if (!m_bIsRunFlag.load() && nullptr == m_pThread)
    {
        m_bIsRunFlag.store(true);
        m_pThread = new std::thread(stdThreadRunFunction, (void *)this);
    }
}

void CMaintenanceThread::stop()
{
    if (m_bIsRunFlag.load() && nullptr != m_pThread)
    {
        m_bIsRunFlag.store(false);
        m_pThread->join();
        delete m_pThread;
        m_pThread = nullptr;
    }
}

bool CMaintenanceThread::isRuning()
{
    return m_bIsRunFlag.load();
}
