/*
 * @FilePath     : stream_ai.c
 * @Author       : zhouzirui
 * @Date         : 2024-11-26 14:50:04
 * @LastEditors: leiyy leiyy@kfb.cn
 * @LastEditTime: 2025-11-24 15:19:43
 * @Description  : 音频输入模块
 */
#include <stdio.h>

#include "stream_ai.h"
#include "stream_ao.h"

/*麦克风声卡*/
#define SOUND_CARD_MICIN      "micin"
/*麦克风声卡*/
#define SOUND_CARD_LINEIN      "linein"
/* 音频AI Micin开启降噪 VQE设置 */
#define AUDIO_MICIN_ENABLE_NR_VQE_CONFIG ("/opt/cam/.config/design_data/config_aivqe_micin_nr.json")
/* 音频AI Micin关闭降噪 VQE设置 */
#define AUDIO_MICIN_DISABLE_NR_VQE_CONFIG ("/opt/cam/.config/design_data/config_aivqe_micin.json")
/* 音频AI Linein开启降噪 VQE设置 */
#define AUDIO_LINEIN_ENABLE_NR_VQE_CONFIG ("/opt/cam/.config/design_data/config_aivqe_linein_nr.json")
/* 音频AI Linein关闭降噪 VQE设置 */
#define AUDIO_LINEIN_DISABLE_NR_VQE_CONFIG ("/opt/cam/.config/design_data/config_aivqe_linein.json")

/**
 * @brief       : 查找声卡号对应的声卡名称
 * @author      : zhouzirui
 * @param        {int} card_num     声卡号
 * @param        {char} *pCardName  声卡名称
 * @return       {*}
 */
int get_card_name(int card_num, char *pCardName) // int *pLen
{
    FILE *file = fopen("/proc/asound/cards", "r");
    if (file == NULL)
    {
        return ERR; // 打开文件失败
    }

    char line[LENGTH256];
    int current_card_num = 0;

    while (fgets(line, sizeof(line), file) != NULL)
    {
        if (sscanf(line, "%d", &current_card_num) == 1)
        {
            if (current_card_num == card_num)
            {
                char *temp = strchr(line, '[');
                if (temp != NULL)
                {
                    temp++; // 跳过 '['
                    char *end = strchr(temp, ']');
                    if (end != NULL)
                    {
                        *end = '\0'; // 结束字符串
                    }
                    // 复制声卡名到提供的字符数组中
                    strncpy(pCardName, temp, 256 - 1);
                    pCardName[256 - 1] = '\0'; // 确保字符串结束

                    // 设置返回的长度
                    // if (pLen != NULL)
                    // {
                    //     *pLen = strlen(pCardName); // 返回声卡名的长度
                    // }
                    fclose(file);
                    return OK; // 成功
                }
            }
        }
    }

    fclose(file);
    return ERR; // 未找到对应的声卡名
}

RkAi_S *streamAi_init(int nAiDevice, Audio_NS::AudioConfig_S stAudioConfig)
{
    int nRet = OK;
    RkAiNeedParam_S stNeedParam;
    memset(&stNeedParam, 0, sizeof(RkAiNeedParam_S));

    if(stAudioConfig.enInputType == Audio_NS::AudioInputType_E::MICIN)
    {
        snprintf(stNeedParam.aDevName, sizeof(SOUND_CARD_MICIN), "%s", SOUND_CARD_MICIN);
    }
    else if(stAudioConfig.enInputType == Audio_NS::AudioInputType_E::LINEIN)
    {
        snprintf(stNeedParam.aDevName, sizeof(SOUND_CARD_LINEIN), "%s", SOUND_CARD_LINEIN);
    }
    stNeedParam.enbitWidth = AUDIO_BIT_WIDTH_16;
    /*默认录音格式为16KHz*/
    stNeedParam.enSampleRate = AUDIO_SAMPLE_RATE_16000;
    stNeedParam.enSoundMode = AUDIO_SOUND_MODE_MONO;
    stNeedParam.u32FrameNum = FRAME_NUM_DEFAULT;
    stNeedParam.u32PointNumPerFrame = EVS_SAMPLES_PER_FRAME;
    stNeedParam.nChnNum = 1;

    if(stAudioConfig.enFormat == Audio_NS::AudioFormat_E::AAC)
    {
        stAudioConfig.enSampRate = Audio_NS::AudioSamprate_E::AUDIO_SAMPRATE_16000;
        stNeedParam.u32PointNumPerFrame = AACLC_SAMPLES_PER_FRAME;
    }
    else if(stAudioConfig.enFormat == Audio_NS::AudioFormat_E::MP3)
    {
        stNeedParam.u32PointNumPerFrame = MP3_SAMPLES_PER_FRAME;
    }else if(stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711A || stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G711U || stAudioConfig.enFormat == Audio_NS::AudioFormat_E::G726)
    {
        stNeedParam.u32PointNumPerFrame = AMR_SAMPLES_PER_FRAME;
    }
    stNeedParam.bResampleEnable = RK_FALSE;
    stNeedParam.enResampleRate = (AUDIO_SAMPLE_RATE_E)stAudioConfig.enSampRate;
    if(stAudioConfig.enSampRate != (Audio_NS::AudioSamprate_E)stNeedParam.enSampleRate)
    {
        stNeedParam.bResampleEnable = RK_TRUE;
    }

    /* 默认启用VQE */
    stNeedParam.bVqeEnable = RK_TRUE;
    /* 开启aed，声音异常侦测 */
    stNeedParam.bAedEnable = RK_FALSE;

    /*分配句柄*/
    RkAi_S *pHandle = rockitAi_alloc(stNeedParam);
    if (pHandle == nullptr)
    {
        dlog_error("分配rockit Ai句柄失败");
        return nullptr;
    }

    pHandle->stExParam.nDevChannels = 2;
    pHandle->stExParam.nDevSampleRate = AUDIO_SAMPLE_RATE_16000;
    /* 双声道，左声道为MIC拾音数据，右声道为播放的右声道的回采数据 */
    pHandle->stExParam.enTrackMode = AUDIO_TRACK_FRONT_LEFT;
    /* Vqe配置文件路径 根据输入类型与降噪、是否启用降噪，区分算法配置 */
    if (stAudioConfig.enInputType == Audio_NS::AudioInputType_E::MICIN)
    {
        if (stAudioConfig.bDenoise)
        {
            memcpy(pHandle->stExParam.aVqeCfgPath, AUDIO_MICIN_ENABLE_NR_VQE_CONFIG, sizeof(AUDIO_MICIN_ENABLE_NR_VQE_CONFIG));
        }
        else
        {
            memcpy(pHandle->stExParam.aVqeCfgPath, AUDIO_MICIN_DISABLE_NR_VQE_CONFIG, sizeof(AUDIO_MICIN_DISABLE_NR_VQE_CONFIG));
        }
    }
    else if (stAudioConfig.enInputType == Audio_NS::AudioInputType_E::LINEIN)
    {
        if (stAudioConfig.bDenoise)
        {
            memcpy(pHandle->stExParam.aVqeCfgPath, AUDIO_LINEIN_ENABLE_NR_VQE_CONFIG, sizeof(AUDIO_LINEIN_ENABLE_NR_VQE_CONFIG));
        }
        else
        {
            memcpy(pHandle->stExParam.aVqeCfgPath, AUDIO_LINEIN_DISABLE_NR_VQE_CONFIG, sizeof(AUDIO_LINEIN_DISABLE_NR_VQE_CONFIG));
        }
    }

    pHandle->stExParam.s64RefChannelType = 2;
    pHandle->stExParam.s64RecChannelType = 1;
    /* AEC 需配置AO 设备号与通道号 */
    pHandle->stExParam.nAoDev = AO_SPEAKER_CHN;
    pHandle->stExParam.nAoChn = AO_SPEAKER_CHN;

    /*初始化*/
    nRet = pHandle->rockitAi_init(pHandle);
    if (nRet != OK)
    {
        dlog_error("初始化 Ai 失败");
        free(pHandle);
        pHandle = nullptr;
        return nullptr;
    }

    dlog_info("音频流采集初始化成功");
    return pHandle;
}

int streamAi_uninit(RkAi_S *pHandle)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }

    /*反初始化ai*/
    int nRet = pHandle->rockitAi_uninit(pHandle);
    if (nRet != RK_SUCCESS)
    {
        dlog_error("去初始化 Ai 失败");
        return ERR;
    }
    rockitAi_release(pHandle);

    dlog_info("音频流采集去初始化成功");
    return OK;
}

int streamAi_set_track_mode(RkAi_S *pHandle, AUDIO_TRACK_MODE_E enTrackMode)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }

    /* 设置AI声道模式 */
    if (OK != pHandle->rockitAi_set_track_mode(pHandle, enTrackMode))
    {
        dlog_error("设置AI声道模式失败");
        return ERR;
    }

    return OK;
}

int streamAi_set_vqe_nr(RkAi_S *pHandle, int nAiChn, bool bDenoise)
{
    if (pHandle == NULL)
    {
        dlog(LOG_ERROR, "句柄为空");
        return ERR_PTR_NULL;
    }

    if (RK_SUCCESS != pHandle->rockitAi_whether_enable_vqe_nr(pHandle, nAiChn, (RK_BOOL) bDenoise))
    {
        dlog_error("设置噪声消除失败");
        return ERR;
    }

    return OK;
}
