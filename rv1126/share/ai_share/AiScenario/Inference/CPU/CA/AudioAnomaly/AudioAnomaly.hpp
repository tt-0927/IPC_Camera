/*
*  File Name: AudioAnomaly.h
*  Created on: 2024年9月10日
*  Author: wcp
*  description : 音频异常数据算法
*  Modify date: 2024年9月10日
*/

#include <fftw3.h>
// #include <sndfile.h>
#include <iostream>
#include <vector>
#include <cmath>

namespace AudioAnomaly_NS{
class cAudioAnomaly
{
	public:
		cAudioAnomaly();
		~cAudioAnomaly();

	public:
		/**
		 * @brief 静音检测算法
		 * @param [double*] fFrameData: pcm音频数据
		 * @param [int] nFrameSize: pcm音频长度
		 * @param [float] fThreshold: 静音能量的阈值（默认：0.01）
		 * @param [bool&] bFlag: 是否出现静音异常
		 * @return [int] 0成功，-1失败
		 * @note
		 */
		int silentDetecte(double* fFrameData, int nFrameSize ,float fThreshold, bool& bFlag);

		/**
		 * @brief 声音大忽小检测算法
		 * @param [double*] fFrameData: pcm音频数据
		 * @param [int] nFrameSize: pcm音频长度
		 * @param [float] fThreshold: 音频能量波动的阈值（默认：0.01）
		 * @param [bool&] bFlag: 是否出现声音大忽小异常
		 * @return [int] 0成功，-1失败
		 * @note
		 */
		int fluctuateDetect(double* fFrameData, int nFrameSize ,float fThreshold, bool& bFlag);

	private:
		/* 声音忽大忽小算法，上一个音频的平均能量 */
		bool bFirstFlu = true;
		double dPreviousEnergy = 0.0;
};
}
