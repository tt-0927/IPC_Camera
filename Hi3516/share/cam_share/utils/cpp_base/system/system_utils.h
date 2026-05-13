/**
 * @FilePath     : system_utils.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-11-11 15:31:43
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-11-11 15:41:06
 * @Description  : 系统工具
 */

#pragma once

#include <thread>
#include <string>

/**
 * @brief   : 设置线程优先级
 * @param    {thread} &thr 线程句柄
 * @param    {int} policy 线程轮训策略
 * @param    {int} prio 优先级（1-99）
 */
void setThreadPriority(std::thread &thr, int policy, int prio);

/**
 * @brief   : 打印线程策略等信息
 * @param    {thread} &thr 线程句柄
 * @param    {string} &name 线程名 默认：为空
 */
void printThreadSchedInfo(std::thread &thr, const std::string &name = "");
