/**
 * @FilePath     : codec_mp2.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-10 10:52:50
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-10 15:03:44
 * @Description  :MP2编码器类实现文件
 */

#include "codec_mp2.hpp"
#include "twolame.h"
#include <algorithm>
#include <cstring>

CCodecMp2::CCodecMp2()
    : m_pOptions(nullptr), m_bInitialized(false), m_samplesPerFrame(0), m_frameLength(0)
{
}

CCodecMp2::~CCodecMp2()
{
    Destroy();
}

/**
 * @brief 初始化编码器
 * @param config 编码器配置参数
 * @return EncodeResult::SUCCESS 成功，其他值表示失败
 */
CCodecMp2::EncodeResult CCodecMp2::Initialize(const Mp2Config_S &config)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    // 如果已经初始化，先销毁
    if (m_bInitialized)
    {
        Destroy();
    }

    // 验证配置参数
    if (!ValidateConfig(config))
    {
        return EncodeResult::ERROR_PARAMS;
    }

    // 初始化TwoLAME编码器
    m_pOptions = twolame_init();
    if (!m_pOptions)
    {
        return EncodeResult::ERROR_INIT;
    }

    // 应用配置
    EncodeResult result = ApplyConfig(config);
    if (result != EncodeResult::SUCCESS)
    {
        twolame_close(&m_pOptions);
        m_pOptions = nullptr;
        return result;
    }

    // 初始化参数
    if (twolame_init_params(m_pOptions) != 0)
    {
        twolame_close(&m_pOptions);
        m_pOptions = nullptr;
        return EncodeResult::ERROR_INIT;
    }

    // 保存配置和缓存信息
    m_config = config;
    m_samplesPerFrame = TWOLAME_SAMPLES_PER_FRAME;
    m_frameLength = twolame_get_framelength(m_pOptions);
    m_bInitialized = true;

    return EncodeResult::SUCCESS;
}

/**
 * @brief 检查编码器是否已初始化
 * @return true 已初始化，false 未初始化
 */
bool CCodecMp2::IsInitialized() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_bInitialized;
}

/**
 * @brief 获取当前配置
 * @return 当前编码器配置
 */
CCodecMp2::Mp2Config_S CCodecMp2::GetConfig() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_config;
}

/**
 * @brief 编码PCM音频数据 (16位整型)
 * @param pInputPcm 输入的PCM数据指针
 * @param inputSamples 输入采样点数量（每声道）
 * @param pOutputMp2 输出MP2数据缓冲区
 * @param outputBufferSize 输出缓冲区大小（字节）
 * @param pOutputBytes 实际输出的字节数（输出参数）
 * @return EncodeResult::SUCCESS 成功，其他值表示失败
 */
CCodecMp2::EncodeResult CCodecMp2::Encode(const int16_t *pInputPcm,
                                          int inputSamples,
                                          uint8_t *pOutputMp2,
                                          int outputBufferSize,
                                          int *pOutputBytes)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bInitialized)
    {
        return EncodeResult::ERROR_NOT_INITIALIZED;
    }

    if (!pInputPcm || !pOutputMp2 || !pOutputBytes || inputSamples <= 0 || outputBufferSize <= 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    int bytesEncoded = 0;

    if (m_config.nChannels == 1)
    {
        // 单声道编码
        bytesEncoded = twolame_encode_buffer(m_pOptions,
                                             pInputPcm,
                                             pInputPcm, // 左右声道相同
                                             inputSamples,
                                             pOutputMp2,
                                             outputBufferSize);
    }
    else
    {
        // 立体声编码 - 假设输入数据为非交错格式 (LLLL...RRRR...)
        const int16_t *leftChannel = pInputPcm;
        const int16_t *rightChannel = pInputPcm + inputSamples;

        bytesEncoded = twolame_encode_buffer(m_pOptions,
                                             leftChannel,
                                             rightChannel,
                                             inputSamples,
                                             pOutputMp2,
                                             outputBufferSize);
    }

    if (bytesEncoded < 0)
    {
        return EncodeResult::ERROR_ENCODE;
    }

    *pOutputBytes = bytesEncoded;
    return EncodeResult::SUCCESS;
}

/**
 * @brief 编码交错PCM音频数据 (16位整型)
 * @param pInputPcm 输入的交错PCM数据指针 (L,R,L,R...)
 * @param inputSamples 输入采样点数量（每声道）
 * @param pOutputMp2 输出MP2数据缓冲区
 * @param outputBufferSize 输出缓冲区大小（字节）
 * @param pOutputBytes 实际输出的字节数（输出参数）
 * @return EncodeResult::SUCCESS 成功，其他值表示失败
 */
CCodecMp2::EncodeResult CCodecMp2::EncodeInterleaved(const int16_t *pInputPcm,
                                                     int inputSamples,
                                                     uint8_t *pOutputMp2,
                                                     int outputBufferSize,
                                                     int *pOutputBytes)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bInitialized)
    {
        return EncodeResult::ERROR_NOT_INITIALIZED;
    }

    if (!pInputPcm || !pOutputMp2 || !pOutputBytes || inputSamples <= 0 || outputBufferSize <= 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    int bytesEncoded = twolame_encode_buffer_interleaved(m_pOptions,
                                                         pInputPcm,
                                                         inputSamples,
                                                         pOutputMp2,
                                                         outputBufferSize);

    if (bytesEncoded < 0)
    {
        return EncodeResult::ERROR_ENCODE;
    }

    *pOutputBytes = bytesEncoded;
    return EncodeResult::SUCCESS;
}

/**
 * @brief 编码浮点PCM音频数据 (32位浮点)
 * @param pInputPcm 输入的浮点PCM数据指针
 * @param inputSamples 输入采样点数量（每声道）
 * @param pOutputMp2 输出MP2数据缓冲区
 * @param outputBufferSize 输出缓冲区大小（字节）
 * @param pOutputBytes 实际输出的字节数（输出参数）
 * @return EncodeResult::SUCCESS 成功，其他值表示失败
 */
CCodecMp2::EncodeResult CCodecMp2::EncodeFloat(const float *pInputPcm,
                                               int inputSamples,
                                               uint8_t *pOutputMp2,
                                               int outputBufferSize,
                                               int *pOutputBytes)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bInitialized)
    {
        return EncodeResult::ERROR_NOT_INITIALIZED;
    }

    if (!pInputPcm || !pOutputMp2 || !pOutputBytes || inputSamples <= 0 || outputBufferSize <= 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    int bytesEncoded = 0;

    if (m_config.nChannels == 1)
    {
        // 单声道编码
        bytesEncoded = twolame_encode_buffer_float32(m_pOptions,
                                                     pInputPcm,
                                                     pInputPcm, // 左右声道相同
                                                     inputSamples,
                                                     pOutputMp2,
                                                     outputBufferSize);
    }
    else
    {
        // 立体声编码 - 假设输入数据为非交错格式 (LLLL...RRRR...)
        const float *leftChannel = pInputPcm;
        const float *rightChannel = pInputPcm + inputSamples;

        bytesEncoded = twolame_encode_buffer_float32(m_pOptions,
                                                     leftChannel,
                                                     rightChannel,
                                                     inputSamples,
                                                     pOutputMp2,
                                                     outputBufferSize);
    }

    if (bytesEncoded < 0)
    {
        return EncodeResult::ERROR_ENCODE;
    }

    *pOutputBytes = bytesEncoded;
    return EncodeResult::SUCCESS;
}

/**
 * @brief 编码交错浮点PCM音频数据 (32位浮点)
 * @param pInputPcm 输入的交错浮点PCM数据指针 (L,R,L,R...)
 * @param inputSamples 输入采样点数量（每声道）
 * @param pOutputMp2 输出MP2数据缓冲区
 * @param outputBufferSize 输出缓冲区大小（字节）
 * @param pOutputBytes 实际输出的字节数（输出参数）
 * @return EncodeResult::SUCCESS 成功，其他值表示失败
 */
CCodecMp2::EncodeResult CCodecMp2::EncodeFloatInterleaved(const float *pInputPcm,
                                                          int inputSamples,
                                                          uint8_t *pOutputMp2,
                                                          int outputBufferSize,
                                                          int *pOutputBytes)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bInitialized)
    {
        return EncodeResult::ERROR_NOT_INITIALIZED;
    }

    if (!pInputPcm || !pOutputMp2 || !pOutputBytes || inputSamples <= 0 || outputBufferSize <= 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    int bytesEncoded = twolame_encode_buffer_float32_interleaved(m_pOptions,
                                                                 pInputPcm,
                                                                 inputSamples,
                                                                 pOutputMp2,
                                                                 outputBufferSize);

    if (bytesEncoded < 0)
    {
        return EncodeResult::ERROR_ENCODE;
    }

    *pOutputBytes = bytesEncoded;
    return EncodeResult::SUCCESS;
}

/**
 * @brief 刷新编码器缓冲区
 * @param pOutputMp2 输出MP2数据缓冲区
 * @param outputBufferSize 输出缓冲区大小（字节）
 * @param pOutputBytes 实际输出的字节数（输出参数）
 * @return EncodeResult::SUCCESS 成功，其他值表示失败
 */
CCodecMp2::EncodeResult CCodecMp2::Flush(uint8_t *pOutputMp2,
                                         int outputBufferSize,
                                         int *pOutputBytes)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bInitialized)
    {
        return EncodeResult::ERROR_NOT_INITIALIZED;
    }

    if (!pOutputMp2 || !pOutputBytes || outputBufferSize <= 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    int bytesEncoded = twolame_encode_flush(m_pOptions, pOutputMp2, outputBufferSize);

    if (bytesEncoded < 0)
    {
        return EncodeResult::ERROR_ENCODE;
    }

    *pOutputBytes = bytesEncoded;
    return EncodeResult::SUCCESS;
}

/**
 * @brief 获取每帧样本数
 * @return 每帧样本数 (通常为1152)
 */
int CCodecMp2::GetSamplesPerFrame() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_samplesPerFrame;
}

/**
 * @brief 获取每帧字节数
 * @return 每帧字节数
 */
int CCodecMp2::GetFrameLength() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_frameLength;
}

/**
 * @brief 获取建议的输出缓冲区大小
 * @return 建议的输出缓冲区大小（字节）
 */
int CCodecMp2::GetRecommendedOutputBufferSize() const
{
    std::lock_guard<std::mutex> lock(m_mutex);
    // 建议输出缓冲区大小为帧长度的2倍，确保有足够空间
    return m_frameLength > 0 ? m_frameLength * 2 : 4096;
}

/**
 * @brief 重置编码器
 * @return EncodeResult::SUCCESS 成功，其他值表示失败
 */
CCodecMp2::EncodeResult CCodecMp2::Reset()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (!m_bInitialized)
    {
        return EncodeResult::ERROR_NOT_INITIALIZED;
    }

    // 保存当前配置
    Mp2Config_S savedConfig = m_config;

    // 销毁当前编码器
    if (m_pOptions)
    {
        twolame_close(&m_pOptions);
        m_pOptions = nullptr;
    }
    m_bInitialized = false;

    // 重新初始化
    return Initialize(savedConfig);
}

/**
 * @brief 销毁编码器，释放资源
 */
void CCodecMp2::Destroy()
{
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_pOptions)
    {
        twolame_close(&m_pOptions);
        m_pOptions = nullptr;
    }

    m_bInitialized = false;
    m_samplesPerFrame = 0;
    m_frameLength = 0;
}

/**
 * @brief 获取错误描述字符串
 * @param result 错误代码
 * @return 错误描述字符串
 */
const char *CCodecMp2::GetErrorString(EncodeResult result)
{
    switch (result)
    {
    case EncodeResult::SUCCESS:
        return "成功";
    case EncodeResult::ERROR_INIT:
        return "初始化失败";
    case EncodeResult::ERROR_PARAMS:
        return "参数错误";
    case EncodeResult::ERROR_ENCODE:
        return "编码失败";
    case EncodeResult::ERROR_BUFFER_TOO_SMALL:
        return "输出缓冲区太小";
    case EncodeResult::ERROR_NOT_INITIALIZED:
        return "编码器未初始化";
    default:
        return "未知错误";
    }
}

/**
 * @brief 获取TwoLAME库版本信息
 * @return 版本字符串
 */
const char *CCodecMp2::GetVersion()
{
    return get_twolame_version();
}

bool CCodecMp2::ValidateConfig(const Mp2Config_S &config) const
{
    // 验证采样率
    const int validSampleRates[] = {8000, 16000, 22050, 24000, 32000, 44100, 48000};
    bool validSampleRate = false;
    for (int rate : validSampleRates)
    {
        if (config.nSampleRate == rate)
        {
            validSampleRate = true;
            break;
        }
    }
    if (!validSampleRate)
    {
        return false;
    }

    // 验证声道数
    if (config.nChannels < 1 || config.nChannels > 2)
    {
        return false;
    }

    // 验证码率 - MPEG-1和MPEG-2支持的码率范围
    const int validBitrates[] = {32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384};
    bool validBitrate = false;
    for (int bitrate : validBitrates)
    {
        if (config.nBitrate == bitrate)
        {
            validBitrate = true;
            break;
        }
    }
    if (!validBitrate)
    {
        return false;
    }

    // 验证心理声学模型
    if (config.nPsyModel < 0 || config.nPsyModel > 4)
    {
        return false;
    }

    // 验证VBR质量等级
    if (config.fVbrLevel < -10.0f || config.fVbrLevel > 10.0f)
    {
        return false;
    }

    // 验证详细程度
    if (config.nVerbosity < 0 || config.nVerbosity > 10)
    {
        return false;
    }

    // 验证采样率和码率的组合是否有效
    // MPEG-2用于低采样率(< 32kHz)，有特定的码率限制
    if (config.nSampleRate < 32000)
    {
        // MPEG-2的码率限制
        if (config.nBitrate > 160)
        {
            return false; // MPEG-2不支持高码率
        }
    }

    // 验证声道数和码率的合理性
    if (config.nChannels == 1)
    {
        // 单声道的码率通常不需要太高
        if (config.nBitrate > 192)
        {
            return false;
        }
    }

    return true;
}

CCodecMp2::EncodeResult CCodecMp2::ApplyConfig(const Mp2Config_S &config)
{
    if (!m_pOptions)
    {
        return EncodeResult::ERROR_NOT_INITIALIZED;
    }

    // 设置详细程度
    if (twolame_set_verbosity(m_pOptions, config.nVerbosity) != 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    // 设置输入声道数
    if (twolame_set_num_channels(m_pOptions, config.nChannels) != 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    // 设置采样率
    if (twolame_set_in_samplerate(m_pOptions, config.nSampleRate) != 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }
    if (twolame_set_out_samplerate(m_pOptions, config.nSampleRate) != 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    // 根据采样率自动设置MPEG版本
    TWOLAME_MPEG_version version = (config.nSampleRate < 32000) ? TWOLAME_MPEG2 : TWOLAME_MPEG1;
    if (twolame_set_version(m_pOptions, version) != 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    // 设置MPEG模式（根据声道数）
    TWOLAME_MPEG_mode mode;
    if (config.nChannels == 1)
    {
        mode = TWOLAME_MONO;
    }
    else
    {
        mode = TWOLAME_STEREO; // 默认立体声，也可以考虑TWOLAME_JOINT_STEREO
    }
    if (twolame_set_mode(m_pOptions, mode) != 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    // 设置码率
    if (twolame_set_bitrate(m_pOptions, config.nBitrate) != 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    // 设置心理声学模型
    if (twolame_set_psymodel(m_pOptions, config.nPsyModel) != 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    // 设置VBR选项
    if (twolame_set_VBR(m_pOptions, config.bEnableVBR ? TRUE : FALSE) != 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    if (config.bEnableVBR)
    {
        if (twolame_set_VBR_level(m_pOptions, config.fVbrLevel) != 0)
        {
            return EncodeResult::ERROR_PARAMS;
        }
    }

    // 设置CRC错误保护
    if (twolame_set_error_protection(m_pOptions, config.bEnableCRC ? TRUE : FALSE) != 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    // 设置默认的填充模式
    if (twolame_set_padding(m_pOptions, TWOLAME_PAD_NO) != 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    // 设置默认的预加重
    if (twolame_set_emphasis(m_pOptions, TWOLAME_EMPHASIS_N) != 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    // 设置版权和原创标志为默认值
    if (twolame_set_copyright(m_pOptions, FALSE) != 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }
    if (twolame_set_original(m_pOptions, FALSE) != 0)
    {
        return EncodeResult::ERROR_PARAMS;
    }

    // 最后初始化参数
    if (twolame_init_params(m_pOptions) != 0)
    {
        return EncodeResult::ERROR_INIT;
    }

    // 缓存帧信息
    m_samplesPerFrame = TWOLAME_SAMPLES_PER_FRAME; // 固定为1152
    m_frameLength = twolame_get_framelength(m_pOptions);

    return EncodeResult::SUCCESS;
}