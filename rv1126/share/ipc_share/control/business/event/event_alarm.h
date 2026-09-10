/*** 
 * @FilePath     : event_alarm.h
 * @Author       : cyc
 * @Date         : 2025-08-18 16:10:50
 * @LastEditors  : cyc
 * @LastEditTime : 2025-08-22 10:27:19
 * @Description  : 普通事件报警
 */

#pragma once

#include "Singleton.h"
#include "alarm_define.h"

class CEventAlarm : public CSingleton<CEventAlarm>
{
    CEventAlarm();
    public:
        ~CEventAlarm();    
        friend class CSingleton<CEventAlarm>;
    
        /* 报警输入 */
        int set_alarm(Alarm::IoInputInfo_S &stIoInputInfo);
        int set_alarm(std::set<Alarm::IoInputInfo_S> &ioInputInfos);
        int get_alarm(std::set<Alarm::IoInputInfo_S> &ioInputInfos);
        /* 报警输出 */
        int set_alarm(Alarm::IoOutputInfo_S &stIoInputInfo);
        int set_alarm(std::set<Alarm::IoOutputInfo_S> &ioOutputInfos);
        int get_alarm(std::set<Alarm::IoOutputInfo_S> &ioOutputInfos);
        
        /* *****  自定义音频管理 ********* */
        /**
         * @brief 编辑自定义音频文件信息
         * @param stCustomOperation 自定义操作信息
         * @return 0：成功 非0：失败
         */
         int edit_audioAlarmCustom_info(const Alarm::CustomOperation_S &stCustomOperation);
         /**
          * @brief 设置自定义音频文件信息
          * @param stCustomOperation 自定义操作信息
          * @return 0：成功 非0：失败
          */
         int set_audioAlarmCustom_info(const Alarm::CustomOperation_S &stCustomOperation);
         /**
          * @brief 获取自定义音频信息列表
          * @param customAudioList 自定义音频列表
          * @return 0：成功 非0：失败
          */
         int get_audioAlarmCustom_info(std::vector<Alarm::CustomAudio_S> &customAudioList);

    private:
         /**
          * @brief 处理自定义音频编辑操作
          * @param stCustomOperation 自定义操作信息
          * @return 0：成功 非0：失败
          */
         int HandleCustomEdit(const Alarm::CustomOperation_S &stCustomOperation);
         /**
          * @brief 处理自定义音频播放操作
          * @param stCustomOperation 自定义操作信息
          * @return 0：成功 非0：失败
          */
         int HandleCustomPlay(const Alarm::CustomOperation_S &stCustomOperation);
         /**
          * @brief 处理自定义音频删除操作
          * @param stCustomOperation 自定义操作信息
          * @return 0：成功 非0：失败
          */
         int HandleCustomDelete(const Alarm::CustomOperation_S &stCustomOperation);
};      

