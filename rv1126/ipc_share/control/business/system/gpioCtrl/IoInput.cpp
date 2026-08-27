/*** 
 * @FilePath     : IoInput.cpp
 * @Author       : cyc
 * @Date         : 2025-08-18 15:13:22
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-05 09:59:57
 * @Description  : 
 */

 #include "IoInput.h"
 #include "dlog.h"

IoInput::IoInput()
{

}

IoInput::~IoInput()
{
    stop_detectiond();
    m_lastState.clear();
    m_lastTriggerState.clear();
}

int IoInput::enable_number(std::set<int> numberSet)
{
    m_listenNumber = numberSet;
    return 0;
}

int IoInput::enable_number(std::map<int, bool> stListenMap)
{
    m_listenMap = stListenMap;
    
    /* 初始化状态记录 */ 
    for (const auto& pair : m_listenMap)
    {
        int nIo = pair.first;
        /* 读取初始状态 */ 
        int nInitState = CGpioCtrl::instance()->alarm_input_read(nIo);
        if (nInitState >= 0)
        {
            m_lastState[nIo] = nInitState;
            
            /* 根据当前模式判断初始触发状态 */ 
            bool bNormallyOpen = pair.second;
            if (bNormallyOpen)
            {
                m_lastTriggerState[nIo] = (nInitState == GPIO_HIGHT);
            }
            else
            {
                m_lastTriggerState[nIo] = (nInitState == GPIO_LOW);
            }

        }
    }
    
    return 0;
}

void IoInput::set_observer(Observer observer)
{
    m_observer = observer;
}

void IoInput::start_detectiond()
{
    m_bExit = false;
    m_thDetection = std::thread(&IoInput::th_detection, this);
}

void IoInput::stop_detectiond()
{
    m_bExit = true;
    if (m_thDetection.joinable())
    {
        m_thDetection.join();
    }
}

void IoInput::th_detection()
{
    pthread_setname_np(pthread_self(), "IoInputDet");

    while (!m_bExit)
    {
        for (const auto& pair : m_listenMap)
        {
            int nIo = pair.first;     
            bool bNormallyOpen = pair.second;
            if (nIo < 0 || nIo >= GPIO_INPUT_COUNT)
            {
                continue;
            }

            int nRet = CGpioCtrl::instance()->alarm_input_read(nIo);
            if (nRet < 0)
            {
                continue;
            }

            /* 获取上一次的状态 */ 
            int nLastGpioState = m_lastState[nIo];  
            /* 上次的触发状态 */
            bool bLastTriggerState = m_lastTriggerState[nIo]; 

            /* 判断当前是否应该触发报警 */
            bool bCurrentTriggerState = false; 

            /* 常开 */
            if(bNormallyOpen)
            {
                /* 高电平 */
                if (nRet == GPIO_HIGHT)
                {
                    bCurrentTriggerState = true;
                }
            }
            /* 常闭 */
            else
            {
                /* 低电平 */
                if (nRet == GPIO_LOW)
                {
                    bCurrentTriggerState = true;
                }

            }

            /* 只有在触发状态发生变化，且当前变为触发状态时，才报警 */ 
            if (bCurrentTriggerState && !bLastTriggerState)
            {
                
                if (m_observer)
                {
                    m_observer(nIo);
                }
            }

            /* 更新状态记录 */ 
            m_lastState[nIo] = nRet;
            m_lastTriggerState[nIo] = bCurrentTriggerState;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}