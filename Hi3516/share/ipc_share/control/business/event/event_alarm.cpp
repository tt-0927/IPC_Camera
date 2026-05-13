/*** 
 * @FilePath     : event_alarm.cpp
 * @Author       : cyc
 * @Date         : 2025-08-18 16:10:35
 * @LastEditors  : cyc
 * @LastEditTime : 2025-09-05 08:57:18
 * @Description  : 普通事件报警
 */

#include <sys/stat.h>
#include <unistd.h>
#include <filesystem>
#include <vector>
#include "event_alarm.h"
#include "event_configure.h"
#include "system_manage.h"
#include "event_linkage.h"
#include "IpcRet.h"

namespace
{
    /**
     * @brief   : 是否支持警报IO
     * @return   {bool} true:支持 false:不支持
     */
    inline bool is_alarm_io_supported()
    {
#if CAP_ALARM_IO // 报警IO能力
        return true;
#else
        return false;
#endif
    }
} // namespace

CEventAlarm::CEventAlarm()
{

}

CEventAlarm::~CEventAlarm()
{
    
}

int CEventAlarm::set_alarm(Alarm::IoInputInfo_S &stIOInputInfo)
{
   if (!is_alarm_io_supported())
   {
       dlog_error("当前设备不支持报警输出配置");
       return ERR_WEB_NOT_SUPPORT;
   }

   // 处理需要复制的IO输入信息
   for (auto i : stIOInputInfo.copyTo)
   {
       Alarm::IoInputInfo_S stCopyInfo;
       stCopyInfo.nIoNumer = i;
       // 获取复制信息的当前配置
       CEventConfigure::instance()->get_configure(stCopyInfo);
       // 更新复制信息的配置
       stCopyInfo.bNormallyOpen = stIOInputInfo.bNormallyOpen;
       stCopyInfo.nDealType = stIOInputInfo.nDealType;
       stCopyInfo.stLinkageList = stIOInputInfo.stLinkageList;
       stCopyInfo.aAlarmTime = stIOInputInfo.aAlarmTime;
       // 设置复制信息的新的配置
       CEventConfigure::instance()->set_configure(stCopyInfo);
       dlog_debug("AlarmIOInputInfo copy to %d", i);
   }
   // 清空复制列表
   stIOInputInfo.copyTo.clear();
   // 设置原始IO输入信息的配置
   CEventConfigure::instance()->set_configure(stIOInputInfo);
   std::map<int, bool> stListenMap;
   std::set<Alarm::IoInputInfo_S> ioInputInfos;
   // 获取所有IO输入信息
   CEventConfigure::instance()->get_configure(ioInputInfos);
   // 收集需要启用的IO输入编号
   for (auto &stIOInputInfo : ioInputInfos)
   {
        dlog_debug("stIOInputInfo.bNormallyOpen:%u",stIOInputInfo.bNormallyOpen);
        stListenMap[stIOInputInfo.nIoNumer] = stIOInputInfo.bNormallyOpen;  
   }
   // 启用收集到的IO输入编号
   SystemManage::instance()->enable_ioInputNumber(stListenMap);
   return 0;
}

int CEventAlarm::set_alarm(std::set<Alarm::IoInputInfo_S> &ioInputInfos)
{
    if (!is_alarm_io_supported())
    {
        dlog_error("当前设备不支持报警输出配置");
        return ERR_WEB_NOT_SUPPORT;
    }

    std::map<int, bool> stListenMap;
    // 遍历IO输入信息集合，设置每个IO输入信息的配置
    for (auto &stIOInputInfo : ioInputInfos)
    {
        if (stIOInputInfo.nIoNumer < 0 || stIOInputInfo.nIoNumer >= GPIO_INPUT_COUNT)
        {
            continue;
        }

        if (stIOInputInfo.nDealType != 0)
        {
            stListenMap[stIOInputInfo.nIoNumer] = stIOInputInfo.bNormallyOpen;
        }
        CEventConfigure::instance()->set_configure(stIOInputInfo);
    }
    // 启用收集到的IO输入编号
    SystemManage::instance()->enable_ioInputNumber(stListenMap);
    return 0;
}

int CEventAlarm::get_alarm(std::set<Alarm::IoInputInfo_S> &ioInputInfos)
{
    if (!is_alarm_io_supported())
    {
        dlog_error("当前设备不支持报警输出配置");
        return ERR_WEB_NOT_SUPPORT;
    }

    /* 从事件配置实例中获取IO输入信息集合 */ 
    CEventConfigure::instance()->get_configure(ioInputInfos);

    /* 确保有完整的GPIO_INPUT_COUNT个报警输入配置 */
    if (ioInputInfos.size() < GPIO_INPUT_COUNT)
    {
        dlog_info("初始化报警输入配置，当前数量: %zu，需要: %d", ioInputInfos.size(), GPIO_INPUT_COUNT);
        
        for (int i = 0; i < GPIO_INPUT_COUNT; i++)
        {
            /* 检查是否已存在该ID的配置 */
            auto it = std::find_if(ioInputInfos.begin(), ioInputInfos.end(),
                [i](const Alarm::IoInputInfo_S& info) {
                    return info.nIoNumer == i;
                });
            
            if (it == ioInputInfos.end())
            {
                /* 不存在则创建新的配置 */
                Alarm::IoInputInfo_S stIOInputInfo;
                stIOInputInfo.nIoNumer = i;
                stIOInputInfo.ioName = "报警输入" + std::to_string(i + 1);
                
                /* 初始化7天的布防时间（默认为空） */
                stIOInputInfo.aAlarmTime.clear();
                stIOInputInfo.aAlarmTime.assign(WEEK_DAYS, std::vector<Common::SchedTime_S>(1));
                
                /* 设置并保存新配置 */
                CEventConfigure::instance()->set_configure(stIOInputInfo);
                ioInputInfos.insert(stIOInputInfo);
                
                dlog_info("创建报警输入 %d 的默认配置", i);
            }
        }
    }

    /* 验证每个配置的布防时间完整性 */
    std::set<Alarm::IoInputInfo_S> updatedInfos;
    for (const auto &info : ioInputInfos)
    {
        Alarm::IoInputInfo_S updatedInfo = info;
        
        /* 确保布防时间有7天 */
        if (updatedInfo.aAlarmTime.size() < WEEK_DAYS)
        {
            updatedInfo.aAlarmTime.resize(WEEK_DAYS);
            dlog_info("修正报警输入 %d 的布防时间配置", updatedInfo.nIoNumer);
        }
        
        updatedInfos.insert(updatedInfo);
    }

    ioInputInfos = std::move(updatedInfos);
    return 0;
}

int CEventAlarm::get_alarm(std::set<Alarm::IoOutputInfo_S>& ioOutputInfos)
{
    if (!is_alarm_io_supported())
    {
        dlog_error("当前设备不支持报警输出配置");
        return ERR_WEB_NOT_SUPPORT;
    }

    /* 从事件配置实例中获取IO输出信息集合 */
    CEventConfigure::instance()->get_configure(ioOutputInfos);

    /* 确保有完整的GPIO_OUTPUT_COUNT个报警输出配置 */
    if (ioOutputInfos.size() < GPIO_OUTPUT_COUNT)
    {
        dlog_info("初始化报警输出配置，当前数量: %zu，需要: %d", ioOutputInfos.size(), GPIO_OUTPUT_COUNT);

        for (int i = 0; i < GPIO_OUTPUT_COUNT; i++)
        {
            /* 检查是否已存在该ID的配置 */
            auto it = std::find_if(ioOutputInfos.begin(),
                                   ioOutputInfos.end(),
                                   [i](const Alarm::IoOutputInfo_S& info)
                                   {
                                       return info.nIoNumer == i;
                                   });

            if (it == ioOutputInfos.end())
            {
                /* 不存在则创建新的配置 */
                Alarm::IoOutputInfo_S stIOOutputInfo;
                stIOOutputInfo.nIoNumer = i;
                stIOOutputInfo.ioName = "报警输出" + std::to_string(i + 1);

                /* 初始化7天的布防时间（默认为空） */
                stIOOutputInfo.aAlarmTime.clear();
                stIOOutputInfo.aAlarmTime.assign(WEEK_DAYS, std::vector<Common::SchedTime_S>(1));

                /* 设置并保存新配置 */
                CEventConfigure::instance()->set_configure(stIOOutputInfo);
                ioOutputInfos.insert(stIOOutputInfo);

                dlog_info("创建报警输出 %d 的默认配置", i);
            }
        }
    }

    /* 获取当前硬件状态并验证每个配置的布防时间完整性 */
    std::set<Alarm::IoOutputInfo_S> updatedInfos;
    for (const auto& stIOOutputInfo : ioOutputInfos)
    {
        Alarm::IoOutputInfo_S updatedInfo = stIOOutputInfo;

        /* 确保布防时间有7天 */
        if (updatedInfo.aAlarmTime.size() < WEEK_DAYS)
        {
            updatedInfo.aAlarmTime.resize(WEEK_DAYS);
            dlog_info("修正报警输出 %d 的布防时间配置", updatedInfo.nIoNumer);
        }

        /* 获取当前硬件状态 */
        int nRet = CGpioCtrl::instance()->alarm_output_state(updatedInfo.nIoNumer);
        if (nRet >= 0)
        {
            updatedInfo.enState = Alarm::IoOutputState_E(nRet);
        }
        else
        {
            updatedInfo.enState = Alarm::IoOutputState_E::OFF;
        }

        updatedInfos.insert(updatedInfo);
    }

    ioOutputInfos = std::move(updatedInfos);
    return 0;
}

int CEventAlarm::set_alarm(Alarm::IoOutputInfo_S &stIOOutputInfo)
{
    if (!is_alarm_io_supported())
    {
        dlog_error("当前设备不支持报警输出配置");
        return ERR_WEB_NOT_SUPPORT;
    }

    /* 参数验证 */
    if (stIOOutputInfo.nIoNumer < 0 || stIOOutputInfo.nIoNumer >= GPIO_OUTPUT_COUNT)
    {
        dlog_error("无效的报警输出号: %d，应在[0,%d]范围内", stIOOutputInfo.nIoNumer, GPIO_OUTPUT_COUNT - 1);
        return -1;
    }
    /* 确保布防时间配置完整（7天） */
    if (stIOOutputInfo.aAlarmTime.size() < WEEK_DAYS)
    {
        dlog_info("补全报警输出 %d 的布防时间配置", stIOOutputInfo.nIoNumer);
        stIOOutputInfo.aAlarmTime.resize(WEEK_DAYS);
    }
    /* 处理需要复制的IO输出信息 */
    for (auto targetId : stIOOutputInfo.copyTo)
    {
        if (targetId < 0 || targetId >= GPIO_OUTPUT_COUNT)
        {
            dlog_warn("跳过无效的复制目标ID: %d", targetId);
            continue;
        }
        
        if (targetId == stIOOutputInfo.nIoNumer)
        {
            dlog_warn("跳过自我复制: %d", targetId);
            continue;
        }
        
        Alarm::IoOutputInfo_S stCopyInfo;
        stCopyInfo.nIoNumer = targetId;
        
        /* 获取复制目标的当前配置 */
        CEventConfigure::instance()->get_configure(stCopyInfo);
        
        /* 更新复制目标的配置，保持ID和名称不变 */
        std::string originalName = stCopyInfo.ioName;
        stCopyInfo.nDelayTime = stIOOutputInfo.nDelayTime;
        stCopyInfo.aAlarmTime = stIOOutputInfo.aAlarmTime;
        
        /* 如果原始名称为空，则使用默认名称 */
        if (originalName.empty())
        {
            stCopyInfo.ioName = "报警输出" + std::to_string(targetId + 1);
        }
        
        /* 设置复制目标的新配置 */
        CEventConfigure::instance()->set_configure(stCopyInfo);
        dlog_info("报警输出 %d 的配置已复制到 %d", stIOOutputInfo.nIoNumer, targetId);
    }
    /* 清空复制列表，避免重复处理 */
    stIOOutputInfo.copyTo.clear();
    /* 设置原始IO输出信息的配置 */
    CEventConfigure::instance()->set_configure(stIOOutputInfo);
    /* 处理硬件状态控制 */
    if (stIOOutputInfo.enState == Alarm::IoOutputState_E::HUMAN_ON)
    {
        CGpioCtrl::instance()->alarm_output_on(stIOOutputInfo.nIoNumer);
        dlog_info("手动开启报警输出 %d", stIOOutputInfo.nIoNumer);
    }
    else if (stIOOutputInfo.enState == Alarm::IoOutputState_E::HUMAN_OFF)
    {
        CGpioCtrl::instance()->alarm_output_off(stIOOutputInfo.nIoNumer);
        dlog_info("手动关闭报警输出 %d", stIOOutputInfo.nIoNumer);
    }
    dlog_info("报警输出 %d 配置更新完成，延时: %ds，状态: %d", 
                stIOOutputInfo.nIoNumer, 
                stIOOutputInfo.nDelayTime,
                (int)stIOOutputInfo.enState);
    return 0;
}

int CEventAlarm::set_alarm(std::set<Alarm::IoOutputInfo_S> &ioOutputInfos)
{
    if (!is_alarm_io_supported())
    {
        dlog_error("当前设备不支持报警输出配置");
        return ERR_WEB_NOT_SUPPORT;
    }

    /* 遍历IO输出信息集合，设置每个IO输出信息的配置 */
    for (auto &stIOOutputInfo : ioOutputInfos)
    {
        if (stIOOutputInfo.nIoNumer < 0 || stIOOutputInfo.nIoNumer >= GPIO_OUTPUT_COUNT)
        {
            dlog_warn("跳过无效的报警输出号: %d", stIOOutputInfo.nIoNumer);
            continue;
        }
        
        /* 设置配置 */
        CEventConfigure::instance()->set_configure(stIOOutputInfo);
        
        /* 处理硬件状态控制 */
        if (stIOOutputInfo.enState == Alarm::IoOutputState_E::HUMAN_ON)
        {
            CGpioCtrl::instance()->alarm_output_on(stIOOutputInfo.nIoNumer);
        }
        else if (stIOOutputInfo.enState == Alarm::IoOutputState_E::HUMAN_OFF)
        {
            CGpioCtrl::instance()->alarm_output_off(stIOOutputInfo.nIoNumer);
        }
        
    }
    return 0;
}

/**
 * @brief 编辑自定义音频文件信息
 * @param stCustomOperation 自定义操作信息
 * @return 0：成功 非0：失败
 */
int CEventAlarm::edit_audioAlarmCustom_info(const Alarm::CustomOperation_S &stCustomOperation)
{
    dlog_info("edit_audioAlarmCustom_info: type=%d, name=%s, path=%s", 
            (int)stCustomOperation.enCustomType, 
            stCustomOperation.strName.c_str(), 
            stCustomOperation.strPath.c_str());

    switch (stCustomOperation.enCustomType)
    {
        case Alarm::CustomOperationType_E::CUSTOM_EDIT:
        {
            return HandleCustomEdit(stCustomOperation);
        }
        case Alarm::CustomOperationType_E::CUSTOM_PLAY:
        {
            return HandleCustomPlay(stCustomOperation);
        }
        case Alarm::CustomOperationType_E::CUSTOM_DEL:
        {
            return HandleCustomDelete(stCustomOperation);
        }
        default:
        {
            dlog_error("未知的自定义操作类型: %d", (int)stCustomOperation.enCustomType);
            return ERR;
        }
    }
}

/**
 * @brief 设置自定义音频文件信息
 * @param stCustomOperation 自定义操作信息
 * @return 0：成功 非0：失败
 */
int CEventAlarm::set_audioAlarmCustom_info(const Alarm::CustomOperation_S &stCustomOperation)
{
    dlog_info("set_audioAlarmCustom_info: customeName=%s, fileName=%s",
              stCustomOperation.strName.c_str(),
              stCustomOperation.strFileName.c_str());

    /* 先将文件从上传路径剪切到配置路径 */
    std::string strSourcePath = UPLOAD_PATH + stCustomOperation.strFileName;
    std::string strDestPath = CUSTOM_AUDIO_CONFIG_PATH + stCustomOperation.strFileName;

    /* 检查源文件是否存在 */
    if (access(strSourcePath.c_str(), F_OK) != 0)
    {
        dlog_error("源文件不存在: %s", strSourcePath.c_str());
        return ERR;
    }

    /* 确保目标目录存在 */
    std::string strDestDir = CUSTOM_AUDIO_CONFIG_PATH;
    if (access(strDestDir.c_str(), F_OK) != 0)
    {
        /* 创建目录，可能需要递归创建 */
        if (mkdir(strDestDir.c_str(), 0755) != 0)
        {
            dlog_error("创建目标目录失败: %s", strDestDir.c_str());
            return ERR;
        }
    }

    /* 执行文件剪切操作（移动文件） */
    if (rename(strSourcePath.c_str(), strDestPath.c_str()) != 0)
    {
        dlog_error("剪切文件失败: %s -> %s, errno=%d", strSourcePath.c_str(), strDestPath.c_str(), errno);
        return ERR;
    }

    dlog_info("文件剪切成功: %s -> %s", strSourcePath.c_str(), strDestPath.c_str());

    /* 获取当前声音报警配置 */
    Alarm::SoundOutputAlarm_S stSoundAlarm;
    if (CEventConfigure::instance()->get_configure(stSoundAlarm) != 0)
    {
        dlog_error("获取声音报警配置失败");
        return ERR;
    }

    /* 创建新的自定义音频项 */
    Alarm::CustomAudio_S newCustomAudio;
    newCustomAudio.bChoose = stCustomOperation.nEnable;
    newCustomAudio.strCustomeName = stCustomOperation.strName;
    newCustomAudio.strPath = strDestPath;

    /* 检查是否已存在相同路径的音频文件，如果存在则更新 */
    bool bFound = false;
    for (auto &customAudio : stSoundAlarm.aCustomAudio)
    {
        if (customAudio.strPath == newCustomAudio.strPath)
        {
            customAudio.strCustomeName = newCustomAudio.strCustomeName;
            customAudio.bChoose = newCustomAudio.bChoose;
            bFound = true;
            dlog_info("更新现有自定义音频: %s", newCustomAudio.strPath.c_str());
            break;
        }
    }

    /* 如果没有找到，且自定义音频列表未满，则添加新项 */
    if (!bFound)
    {
        /* 最多3个自定义音频 */
        if (stSoundAlarm.aCustomAudio.size() < 3)
        {
            stSoundAlarm.aCustomAudio.push_back(newCustomAudio);
            dlog_info("添加新的自定义音频: %s", newCustomAudio.strPath.c_str());
        }
        else
        {
            dlog_error("自定义音频列表已满，最多支持3个");
            /* 剪切失败时，删除文件 */
            remove(strDestPath.c_str());
            return ERR;
        }
    }

    /* 保存更新后的配置 */
    if (CEventConfigure::instance()->set_configure(stSoundAlarm) != 0)
    {
        dlog_error("保存声音报警配置失败");
        /* 保存失败时，删除文件 */
        remove(strDestPath.c_str());
        return ERR;
    }

    return OK;
}

 /**
  * @brief 处理自定义音频编辑操作
  * @param stCustomOperation 自定义操作信息
  * @return 0：成功 非0：失败
  */
 int CEventAlarm::HandleCustomEdit(const Alarm::CustomOperation_S &stCustomOperation)
 {
     /* 获取当前声音报警配置 */ 
     Alarm::SoundOutputAlarm_S stSoundAlarm;
     if (CEventConfigure::instance()->get_configure(stSoundAlarm) != 0)
     {
         dlog_error("获取声音报警配置失败");
         return ERR;
     }
     /* 根据路径查找匹配的自定义音频并修改名称 */ 
     bool bFound = false;
     for (auto &customAudio : stSoundAlarm.aCustomAudio)
     {
         if (customAudio.strPath == stCustomOperation.strPath)
         {
             customAudio.strCustomeName = stCustomOperation.strName;
             bFound = true;
             dlog_info("编辑自定义音频名称: 路径=%s, 新名称=%s", 
                       stCustomOperation.strPath.c_str(), 
                       stCustomOperation.strName.c_str());
             break;
         }
     }
     if (!bFound)
     {
         dlog_error("未找到匹配的自定义音频文件: %s", stCustomOperation.strPath.c_str());
         return ERR;
     }
     /* 保存更新后的配置 */ 
     if (CEventConfigure::instance()->set_configure(stSoundAlarm) != 0)
     {
         dlog_error("保存声音报警配置失败");
         return ERR;
     }
     return OK;
 }
 
 /**
  * @brief 处理自定义音频播放操作
  * @param stCustomOperation 自定义操作信息
  * @return 0：成功 非0：失败
  */
int CEventAlarm::HandleCustomPlay(const Alarm::CustomOperation_S &stCustomOperation)
{
    dlog_info("播放自定义音频: 路径=%s, 名称=%s", 
            stCustomOperation.strPath.c_str(), 
            stCustomOperation.strName.c_str());
    CEventLinkage::instance()->play_audio(stCustomOperation.strPath.c_str(),1);
    return OK;
}

 /**
  * @brief 处理自定义音频删除操作
  * @param stCustomOperation 自定义操作信息
  * @return 0：成功 非0：失败
  */
 int CEventAlarm::HandleCustomDelete(const Alarm::CustomOperation_S &stCustomOperation)
 {
     /* 从配置中删除对应项 */ 
     Alarm::SoundOutputAlarm_S stSoundAlarm;
     if (CEventConfigure::instance()->get_configure(stSoundAlarm) != 0)
     {
         dlog_error("获取声音报警配置失败");
         return ERR;
     }
     /* 查找并删除匹配的自定义音频项 */ 
     auto it = std::remove_if(stSoundAlarm.aCustomAudio.begin(), 
                             stSoundAlarm.aCustomAudio.end(),
                             [&stCustomOperation](const Alarm::CustomAudio_S &audio) {
                                 return audio.strPath == stCustomOperation.strPath;
                             });
     
     if (it != stSoundAlarm.aCustomAudio.end())
     {
         stSoundAlarm.aCustomAudio.erase(it, stSoundAlarm.aCustomAudio.end());
         dlog_info("从配置中删除自定义音频: %s", stCustomOperation.strPath.c_str());
     }
     else
     {
         dlog_warn("配置中未找到要删除的自定义音频: %s", stCustomOperation.strPath.c_str());
     }
     /* 保存更新后的配置 */ 
     if (CEventConfigure::instance()->set_configure(stSoundAlarm) != 0)
     {
         dlog_error("保存声音报警配置失败");
         return ERR;
     }
     /* 删除实际文件 */ 
     if (!stCustomOperation.strPath.empty())
     {
         try
         {
             if (std::filesystem::exists(stCustomOperation.strPath))
             {
                 if (std::filesystem::remove(stCustomOperation.strPath))
                 {
                     dlog_info("成功删除音频文件: %s", stCustomOperation.strPath.c_str());
                 }
                 else
                 {
                     dlog_error("删除音频文件失败: %s", stCustomOperation.strPath.c_str());
                     return ERR;
                 }
             }
             else
             {
                 dlog_warn("要删除的音频文件不存在: %s", stCustomOperation.strPath.c_str());
             }
         }
         catch (const std::filesystem::filesystem_error& ex)
         {
             dlog_error("删除文件时发生异常: %s, 文件: %s", ex.what(), stCustomOperation.strPath.c_str());
             return ERR;
         }
     }
     return OK;
 }
/**
 * @brief 获取自定义音频信息列表
 * @param customAudioList 自定义音频列表
 * @return 0：成功 非0：失败
 */
 int CEventAlarm::get_audioAlarmCustom_info(std::vector<Alarm::CustomAudio_S> &customAudioList)
 {
     // 获取当前声音报警配置
     Alarm::SoundOutputAlarm_S stSoundAlarm;
     if (CEventConfigure::instance()->get_configure(stSoundAlarm) != 0)
     {
         dlog_error("获取声音报警配置失败");
         return ERR;
     }
     /* 清空输出列表 */ 
     customAudioList.clear();
     /* 使用copy_if算法筛选有名称的自定义音频 */ 
     std::copy_if(stSoundAlarm.aCustomAudio.begin(), 
                  stSoundAlarm.aCustomAudio.end(),
                  std::back_inserter(customAudioList),
                  [](const Alarm::CustomAudio_S &audio) {
                      return !audio.strCustomeName.empty();
                  });
     dlog_info("获取有效自定义音频列表，总共 %zu 个，有效 %zu 个", 
               stSoundAlarm.aCustomAudio.size(), 
               customAudioList.size());
     return OK;
 }
