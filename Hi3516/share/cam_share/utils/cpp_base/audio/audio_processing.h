/**
 * @FilePath     : audio_processing.h
 * @Author       : zhouzirui
 * @Date         : 2025-01-06 17:24:54
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-10-09 14:34:07
 * @Description  : pcm音频处理。复制声道、混音等
 */

#ifndef __AUDIO_PROCESSING_H__
#define __AUDIO_PROCESSING_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief       : 将双通道PCM数据分离为左右声道的数据
 * @author      : zhouzirui
 * @param        {int16_t*} input   输入双声道音频
 * @param        {int8_t*} left     输出左声道音频
 * @param        {int8_t*} right    输出右声道音频
 * @param        {int} numSamples   16位采样位深的输入双声道音频长度
 * @return       {*}
 */
void audio_processing_splitStereoChannels(int16_t* input, int8_t* left, int8_t* right, int numSamples);

/**
 * @brief       : 从双通道PCM数据中提取左声道
 * @author      : zhouzirui
 * @param        {int16_t} *input   输入双声道音频
 * @param        {int16_t} *left    输出左声道音频
 * @param        {int} numSamples   16位采样位深的输入双声道音频长度
 * @return       {*}
 */
void audio_processing_extractLeftChannel(int16_t *input, int16_t *left, int numSamples);

/**
 * @brief       : 16bit 复制单声道至立体声
 * @author      : zhouzirui
 * @param        {char*} monoData   单声道音频
 * @param        {int} monoSize     单声道音频长度
 * @param        {char*} stereoData 输出双声道音频
 * @return       {*}
 */
void audio_processing_convertMonoToStereo(const  char* monoData, int monoSize,  char* stereoData);

/**
 * @brief       : 混音算法
 * @author      : zhouzirui
 * @param        {int8_t} *srcData  输入：一路音频数据
 * @param        {int8_t} *dstData  输入：一路音频数据  输出：混音后音频数据
 * @param        {int} nSize    一路音频数据长度
 * @return       {*}0：成功 -1：失败
 */
int audio_processing_mix(int8_t *srcData, int8_t *dstData, int nSize);

/**
 * @brief       : 混音算法（两通道混合为一通道）
 * @author      : zhouzirui
 * @param        {int8_t} *srcData  输入：两通道音频数据
 * @param        {int8_t} *dstData  输出：混音后音频数据（单通道）
 * @param        {int} nSize        输入数据长度（字节数，必须为4的倍数）
 * @return       {*}0：成功 -1：失败
 */
int audio_processing_mixToMono(int8_t *srcData, int8_t *dstData, int nSize);

/**
 * @brief   : 音频增益
 * @param    {int8_t} *data 处理的音频数据
 * @param    {int} bytes 数据长度
 * @param    {float} shift 增益大小
 */
void audio_processing_volChange(int8_t *data, int bytes, float shift);

/**
 * @brief   : 计算PCM音频帧的RMS值
 * @param    {short} *pSamples 音频采样数据
 * @param    {int} nSampleCount 采样点数量
 * @return   {float} RMS值
 */
float audio_processing_calculateRMS(const short *pSamples, int nSampleCount);

/**
 * @brief   : 将RMS值转换为分贝值
 * @param    {float} fRMS RMS值
 * @param    {int} nBitsPerSample 采样位数(8/16/32)
 * @return   {float} 分贝值(dB SPL)
 */
float audio_processing_convertRMSToDecibel(float fRMS, int nBitsPerSample);

#ifdef __cplusplus
}
#endif
#endif // __AUDIO_PROCESSING_H__