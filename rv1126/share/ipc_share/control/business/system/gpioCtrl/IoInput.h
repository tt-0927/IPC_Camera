/*** 
 * @FilePath     : IoInput.h
 * @Author       : cyc
 * @Date         : 2025-08-18 15:13:22
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-05 09:49:15
 * @Description  : 
 */


#pragma once

#include "gpio_ctrl.h"

#include <set>
#include <thread>
#include <chrono>
#include <functional>
#include <map>

class IoInput
{
public:
    using Observer = std::function<void(int)>;
    IoInput();
    ~IoInput();

    int enable_number(std::set<int> numberSet);
    
    int enable_number(std::map<int, bool> numberSet);

    void set_observer(Observer observer);

    void start_detectiond();

    void stop_detectiond();


private:
    void th_detection();

private:
    bool m_bExit = false;
    std::map<int, bool> m_listenMap;
    std::set<int> m_listenNumber;
    std::thread m_thDetection;
    Observer m_observer = nullptr;

    /* 记录每个IO的上一次状态 */
    std::map<int, int> m_lastState; 
    /* 记录每个IO的上一次触发状态 */ 
    std::map<int, bool> m_lastTriggerState; 
};
