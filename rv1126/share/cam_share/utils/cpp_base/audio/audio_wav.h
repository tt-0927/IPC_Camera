/**
 * @FilePath     : audio_wav.h
 * @Author       : zhouzirui
 * @Date         : 2024-12-27 14:13:02
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2024-12-31 17:04:10
 * @Description  :写pcm数据至wav格式
 */

#ifndef __AUDIO_WAV_H__
#define __AUDIO_WAV_H__

#include <string>

/* 定义WAV文件头结构体 */
struct WavHeader {
    /** RIFF头标识 */
    char riff[4];
    /** 文件大小（除去RIFF头和文件大小本身） */
    uint32_t file_size;
    /** WAVE头标识 */
    char wave[4];
    /** fmt头标识 */
    char fmt[4];
    /** fmt头大小 */
    uint32_t fmt_size;
    /** 音频格式（1为PCM） */
    uint16_t format;
    /** 通道数 */
    uint16_t channels;
    /** 采样率 */
    uint32_t sample_rate;
    /** 每秒字节数 */
    uint32_t byte_rate;
    /** 块对齐 */
    uint16_t block_align;
    /** 位深 */
    uint16_t bits_per_sample;
    /** 数据头标识 */
    char data[4];
    /** 数据大小 */
    uint32_t data_size;
};

class CAudioWav {
public:
    CAudioWav();
    ~CAudioWav();

    /**
     * @brief       : 初始化音频WAV文件
     * @author      : zhouzirui
     * @param        {int} sample_rate 采样率
     * @param        {int} bits_per_sample 位深
     * @param        {int} channels 通道数
     * @return       {*}
     */
    void init(int sample_rate, int bits_per_sample, int channels);

    /**
     * @brief       : 打开音频WAV文件
     * @author      : zhouzirui
     * @param        {string&} filename 文件路径名
     * @return       {*}0表示成功，-1表示失败
     */
    int open(const std::string& filename);

    /**
     * @brief       : 写入音频数据
     * @author      : zhouzirui
     * @param        {uint8_t*} audio_data 音频数据
     * @param        {int} audio_data_size 音频数据大小
     * @return       {*}0表示成功，-1表示失败
     */
    int write(uint8_t* audio_data, int audio_data_size);

    /**
     * @brief       : 关闭音频WAV文件
     * @author      : zhouzirui
     * @return       {*}0表示成功，-1表示失败
     */
    int close();

    /**
     * @brief       : 释放音频WAV文件资源
     * @author      : zhouzirui
     * @return       {*}
     */
    void uninit();

private:
    /** 音频WAV文件流 */
    std::ofstream* m_file;
    /** 采样率 */
    int m_nSampleRate;
    /** 位深 */
    int m_nBitsPerSample;
    /** 通道数 */
    int m_nChannels;
    /** WAV文件头 */
    WavHeader m_stHeader;
};

#endif // __AUDIO_WAV_H__
