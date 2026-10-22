/**
 * @FilePath     : audio_wav.cpp
 * @Author       : zhouzirui
 * @Date         : 2024-12-27 14:13:05
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2024-12-31 17:00:25
 * @Description  :写pcm数据至wav格式
 */

#include "audio_wav.h"
#include <iostream>
#include <fstream>
#include <string.h>

CAudioWav::CAudioWav() : m_file(nullptr), m_nSampleRate(0), m_nBitsPerSample(0), m_nChannels(0) {}

CAudioWav::~CAudioWav()
{
    uninit();
}

void CAudioWav::init(int sample_rate, int bits_per_sample, int channels)
{
    m_nSampleRate = sample_rate;
    m_nBitsPerSample = bits_per_sample;
    m_nChannels = channels;
}

int CAudioWav::open(const std::string &filename)
{
    m_file = new std::ofstream(filename, std::ios::out | std::ios::binary | std::ios::trunc);
    if (!m_file->is_open())
    {
        delete m_file;
        m_file = nullptr;
        return -1;
    }

    // 初始化WAV文件头
    memset(&m_stHeader, 0, sizeof(WavHeader));
    strcpy(m_stHeader.riff, "RIFF");
    m_stHeader.file_size = 0;
    strcpy(m_stHeader.wave, "WAVE");
    strcpy(m_stHeader.fmt, "fmt ");
    m_stHeader.fmt_size = 16;
    m_stHeader.format = 1;
    m_stHeader.channels = m_nChannels;
    m_stHeader.sample_rate = m_nSampleRate;
    m_stHeader.byte_rate = m_nSampleRate * m_nChannels * (m_nBitsPerSample / 8);
    m_stHeader.block_align = m_nChannels * (m_nBitsPerSample / 8);
    m_stHeader.bits_per_sample = m_nBitsPerSample;
    strcpy(m_stHeader.data, "data");
    m_stHeader.data_size = 0;

    // 写入WAV文件头
    m_file->write((char *)&m_stHeader, sizeof(WavHeader));

    return 0;
}

int CAudioWav::write(uint8_t *audio_data, int audio_data_size)
{
    if (!m_file)
    {
        return -1;
    }

    // 写入音频数据
    m_file->seekp(sizeof(WavHeader) + m_stHeader.data_size);
    m_file->write((char *)audio_data, audio_data_size);
    m_file->flush();

    // 更新数据大小
    m_stHeader.data_size += audio_data_size;

    // 更新文件大小
    m_stHeader.file_size = sizeof(WavHeader) - 8 + m_stHeader.data_size;

    // 更新文件头
    m_file->seekp(4);
    m_file->write((char *)&m_stHeader.file_size, 4);
    m_file->seekp(40);
    m_file->write((char *)&m_stHeader.data_size, 4);

    return 0;
}

int CAudioWav::close()
{
    if (!m_file)
    {
        return -1;
    }

    m_file->close();
    delete m_file;
    m_file = nullptr;

    return 0;
}

void CAudioWav::uninit()
{
    if (m_file)
    {
        m_file->close();
        delete m_file;
        m_file = nullptr;
    }
}
