/**
 * @FilePath     : event_linkage_action_async.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-15 16:29:58
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-03 15:47:36
 * @Description  : 事件联动异步动作执行器模块
 */

#pragma once

#include <atomic>
#include <map>
#include <mutex>
#include <string>

#include "event_linkage_types.h"

class EventLinkageAsyncAction
{
public:
    /**
     * @brief   : 执行异步联动任务
     * @param    {LinkageTask_S} &stTask 联动任务快照
     * @param    {std::atomic<bool>} &bRunningFlag 运行标志
     */
    void execute(const LinkageTask_S &stTask, std::atomic<bool> &bRunningFlag);

    /**
     * @brief   : 播放音频
     * @param    {std::string} strAudioPath 音频文件路径
     * @param    {int} nTimes 播放次数
     * @param    {std::atomic<bool>} &bRunningFlag 运行标志
     */
    void play_audio(const std::string &strAudioPath, int nTimes, std::atomic<bool> &bRunningFlag);

    /**
     * @brief   : 根据当前配置获取音频文件路径
     * @param    {std::string} &strAudioPath 输出音频路径
     * @return   {int} 0：成功 非0：失败
     */
    int get_audio_file_path(std::string &strAudioPath);

    /**
     * @brief   : 获取当前正在播放的音频路径
     * @return   {std::string} 正在播放的音频文件路径，未播放则返回空字符串
     */
    std::string get_playing_audio_path();

private:
    /**
     * @brief   : 执行邮件联动
     * @param    {LinkageTask_S} &stTask 联动任务快照
     * @param    {std::atomic<bool>} &bRunningFlag 运行标志
     */
    void execute_email(const LinkageTask_S &stTask, std::atomic<bool> &bRunningFlag);

    /**
     * @brief   : 执行音频联动
     * @param    {LinkageTask_S} &stTask 联动任务快照
     * @param    {std::atomic<bool>} &bRunningFlag 运行标志
     */
    void execute_audio(const LinkageTask_S &stTask, std::atomic<bool> &bRunningFlag);

    /**
     * @brief   : 执行闪光灯联动
     * @param    {std::atomic<bool>} &bRunningFlag 运行标志
     */
    void execute_warning_light(std::atomic<bool> &bRunningFlag);

    /**
     * @brief   : 执行报警输出联动
     * @param    {LinkageTask_S} &stTask 联动任务快照
     * @param    {std::atomic<bool>} &bRunningFlag 运行标志
     */
    void execute_alarm_io(const LinkageTask_S &stTask, std::atomic<bool> &bRunningFlag);

    /**
     * @brief   : 执行 onvif 日志联动
     * @param    {LinkageTask_S} &stTask 联动任务快照
     */
    void execute_log(const LinkageTask_S &stTask);

    /**
     * @brief   : 根据配置选择音频文件
     * @param    {std::string} &strAudioPath 输出音频路径
     * @param    {int} &nTimes 播放次数
     * @return   {int} 0：成功 非0：失败
     */
    int select_audio_file(std::string &strAudioPath, int &nTimes);

private:
    /* 当前播放的音频路径 */
    std::string m_strPlayingAudioPath;
    /* 音频路径互斥锁 */
    std::mutex m_audioPathMutex;
};
