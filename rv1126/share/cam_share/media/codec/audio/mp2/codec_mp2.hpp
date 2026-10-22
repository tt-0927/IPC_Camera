/**
 * @FilePath     : codec_mp2.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-10 10:52:55
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-10 11:28:47
 * @Description  : MP2编码器类头文件 - 基于TwoLAME库的简洁封装
 */

#pragma once

#include <memory>
#include <mutex>
#include <vector>
#include <cstdint>

// 前向声明，避免包含twolame.h在头文件中
struct twolame_options_struct;
typedef struct twolame_options_struct twolame_options;

/**
 * @brief MP2编码器类
 *
 * 基于TwoLAME库的MP2编码器封装类，提供简洁易用的接口
 * 支持多种音频格式和参数配置
 */
class CCodecMp2
{
public:
    /**
     * @brief 编码器配置结构体
     */
    typedef struct Mp2Config
    {
        int nSampleRate; /* 采样率 (Hz) - 支持8000, 16000, 22050, 24000, 32000, 44100, 48000 */
        int nChannels;   /* 声道数 - 1(单声道) 或 2(立体声) */
        int nBitrate;    /* 码率 (kbps) - 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384 */
        int nPsyModel;   /* 心理声学模型 - 0-4 (推荐3) */
        bool bEnableVBR; /* 是否启用可变码率 */
        float fVbrLevel; /* VBR质量等级 - -10.0 到 10.0 (默认5.0) */
        bool bEnableCRC; /* 是否启用CRC错误保护 */
        int nVerbosity;  /* 详细程度 - 0-10 (0=静默) */

        /*默认构造函数 - 设置常用参数*/
        Mp2Config()
        {
            nSampleRate = 44100;
            nChannels = 2;
            nBitrate = 192;
            nPsyModel = 3;
            bEnableVBR = false;
            fVbrLevel = 5.0f;
            bEnableCRC = false;
            nVerbosity = 0;
        }
    }Mp2Config_S;

    /**
     * @brief 编码结果枚举
     */
    enum class EncodeResult
    {
        SUCCESS = 0,                 /* 编码成功 */
        ERROR_INIT = -1,             /* 初始化失败 */
        ERROR_PARAMS = -2,           /* 参数错误 */
        ERROR_ENCODE = -3,           /* 编码失败 */
        ERROR_BUFFER_TOO_SMALL = -4, /* 输出缓冲区太小 */
        ERROR_NOT_INITIALIZED = -5   /* 编码器未初始化 */
    };

public:
    /**
     * @brief 构造函数
     */
    CCodecMp2();

    /**
     * @brief 析构函数
     */
    ~CCodecMp2();

    /**
     * @brief 禁用拷贝构造和赋值
     */
    CCodecMp2(const CCodecMp2 &) = delete;
    CCodecMp2 &operator=(const CCodecMp2 &) = delete;

    /**
     * @brief 初始化编码器
     * @param config 编码器配置参数
     * @return EncodeResult::SUCCESS 成功，其他值表示失败
     */
    EncodeResult Initialize(const Mp2Config_S &config);

    /**
     * @brief 检查编码器是否已初始化
     * @return true 已初始化，false 未初始化
     */
    bool IsInitialized() const;

    /**
     * @brief 获取当前配置
     * @return 当前编码器配置
     */
    Mp2Config_S GetConfig() const;

    /**
     * @brief 编码PCM音频数据 (16位整型)
     * @param pInputPcm 输入的PCM数据指针
     * @param inputSamples 输入采样点数量（每声道）
     * @param pOutputMp2 输出MP2数据缓冲区
     * @param outputBufferSize 输出缓冲区大小（字节）
     * @param pOutputBytes 实际输出的字节数（输出参数）
     * @return EncodeResult::SUCCESS 成功，其他值表示失败
     */
    EncodeResult Encode(const int16_t *pInputPcm,
                        int inputSamples,
                        uint8_t *pOutputMp2,
                        int outputBufferSize,
                        int *pOutputBytes);

    /**
     * @brief 编码交错PCM音频数据 (16位整型)
     * @param pInputPcm 输入的交错PCM数据指针 (L,R,L,R...)
     * @param inputSamples 输入采样点数量（每声道）
     * @param pOutputMp2 输出MP2数据缓冲区
     * @param outputBufferSize 输出缓冲区大小（字节）
     * @param pOutputBytes 实际输出的字节数（输出参数）
     * @return EncodeResult::SUCCESS 成功，其他值表示失败
     */
    EncodeResult EncodeInterleaved(const int16_t *pInputPcm,
                                   int inputSamples,
                                   uint8_t *pOutputMp2,
                                   int outputBufferSize,
                                   int *pOutputBytes);

    /**
     * @brief 编码浮点PCM音频数据 (32位浮点)
     * @param pInputPcm 输入的浮点PCM数据指针
     * @param inputSamples 输入采样点数量（每声道）
     * @param pOutputMp2 输出MP2数据缓冲区
     * @param outputBufferSize 输出缓冲区大小（字节）
     * @param pOutputBytes 实际输出的字节数（输出参数）
     * @return EncodeResult::SUCCESS 成功，其他值表示失败
     */
    EncodeResult EncodeFloat(const float *pInputPcm,
                             int inputSamples,
                             uint8_t *pOutputMp2,
                             int outputBufferSize,
                             int *pOutputBytes);

    /**
     * @brief 编码交错浮点PCM音频数据 (32位浮点)
     * @param pInputPcm 输入的交错浮点PCM数据指针 (L,R,L,R...)
     * @param inputSamples 输入采样点数量（每声道）
     * @param pOutputMp2 输出MP2数据缓冲区
     * @param outputBufferSize 输出缓冲区大小（字节）
     * @param pOutputBytes 实际输出的字节数（输出参数）
     * @return EncodeResult::SUCCESS 成功，其他值表示失败
     */
    EncodeResult EncodeFloatInterleaved(const float *pInputPcm,
                                        int inputSamples,
                                        uint8_t *pOutputMp2,
                                        int outputBufferSize,
                                        int *pOutputBytes);

    /**
     * @brief 刷新编码器缓冲区
     * @param pOutputMp2 输出MP2数据缓冲区
     * @param outputBufferSize 输出缓冲区大小（字节）
     * @param pOutputBytes 实际输出的字节数（输出参数）
     * @return EncodeResult::SUCCESS 成功，其他值表示失败
     */
    EncodeResult Flush(uint8_t *pOutputMp2,
                       int outputBufferSize,
                       int *pOutputBytes);

    /**
     * @brief 获取每帧样本数
     * @return 每帧样本数 (通常为1152)
     */
    int GetSamplesPerFrame() const;

    /**
     * @brief 获取每帧字节数
     * @return 每帧字节数
     */
    int GetFrameLength() const;

    /**
     * @brief 获取建议的输出缓冲区大小
     * @return 建议的输出缓冲区大小（字节）
     */
    int GetRecommendedOutputBufferSize() const;

    /**
     * @brief 重置编码器
     * @return EncodeResult::SUCCESS 成功，其他值表示失败
     */
    EncodeResult Reset();

    /**
     * @brief 销毁编码器，释放资源
     */
    void Destroy();

    /**
     * @brief 获取错误描述字符串
     * @param result 错误代码
     * @return 错误描述字符串
     */
    static const char *GetErrorString(EncodeResult result);

    /**
     * @brief 获取TwoLAME库版本信息
     * @return 版本字符串
     */
    static const char *GetVersion();

private:
    /**
     * @brief 验证配置参数
     * @param config 配置参数
     * @return true 参数有效，false 参数无效
     */
    bool ValidateConfig(const Mp2Config_S &config) const;

    /**
     * @brief 应用配置到TwoLAME选项
     * @param config 配置参数
     * @return EncodeResult::SUCCESS 成功，其他值表示失败
     */
    EncodeResult ApplyConfig(const Mp2Config_S &config);

private:
    /*线程安全互斥锁*/
    mutable std::mutex m_mutex;
    /*TwoLAME编码器选项*/
    twolame_options *m_pOptions;
    /*当前配置*/
    Mp2Config_S m_config;
    /*初始化状态*/
    bool m_bInitialized;

    /********缓存的帧信息********/
    /*每帧样本数*/
    int m_samplesPerFrame;
    /*每帧字节数*/
    int m_frameLength;
};
