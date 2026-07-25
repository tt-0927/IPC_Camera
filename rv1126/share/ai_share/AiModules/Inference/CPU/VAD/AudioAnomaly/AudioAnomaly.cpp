/*
 *  File Name: AudioAnomaly.cpp
 *  Created on: 2024年9月10日
 *  Author: wcp
 *  description : 音频异常数据算法
 *  Modify date: 2024年9月10日
 */

#include "AudioAnomaly.hpp"
using namespace Inference_NS;

cAudioAnomaly::cAudioAnomaly()
{
}
cAudioAnomaly::~cAudioAnomaly()
{
}

/* 静音检测算法 */
int cAudioAnomaly::silentDetecte(double *fFrameData, int nFrameSize, float fThreshold, bool &bFlag)
{
	if (!fFrameData)
	{
		printf("[cAudioAnomaly 静音检测算法],输入的指针为空\n");
		return -1;
	}
	if (nFrameSize < 0)
	{
		printf("[cAudioAnomaly 静音检测算法],nFrameSize[%d],小于0\n", nFrameSize);
		return -1;
	}

	/* 执行 FFT */
	fftw_complex *pOutData = (fftw_complex *)fftw_malloc(sizeof(fftw_complex) * (nFrameSize / 2 + 1));
	fftw_plan plan = fftw_plan_dft_r2c_1d(nFrameSize, fFrameData, pOutData, FFTW_ESTIMATE);
	fftw_execute(plan);
	fftw_destroy_plan(plan);

	/* 计算幅度,检测静音 */
	std::vector<double> vMagnitudes(nFrameSize / 2 + 1);
	for (size_t j = 0; j < vMagnitudes.size(); j++)
	{
		vMagnitudes[j] = sqrt(pOutData[j][0] * pOutData[j][0] + pOutData[j][1] * pOutData[j][1]);
		/* 找到非静音部分 */
		if (vMagnitudes[j] > fThreshold)
		{
			bFlag = false;
			break;
		}
		bFlag = true;
	}

	fftw_free(pOutData);
	return 0;
}

/* 声音大忽小检测算法 */
int cAudioAnomaly::fluctuateDetect(double *fFrameData, int nFrameSize, float fThreshold, bool &bFlag)
{
	if (!fFrameData)
	{
		printf("[cAudioAnomaly 声音大忽小检测算法],输入的指针为空\n");
		return -1;
	}
	if (nFrameSize < 0)
	{
		printf("[cAudioAnomaly 声音大忽小检测算法],nFrameSize[%d],小于0\n", nFrameSize);
		return -1;
	}

	/* 计算当前帧的能量 */
	double dEnergy = 0.0;
	for (size_t i = 0; i < nFrameSize; i++)
	{
		dEnergy += fFrameData[i] * fFrameData[i];
	}
	/* 平均能量 */
	dEnergy /= nFrameSize;

	/* 第一次检测能量，则缓存默认没有异常 */
	if (!bFirstFlu)
	{
		/* 检测能量变化 */
		double dChangeValue = fabs(dEnergy - dPreviousEnergy);
		bFlag = (dChangeValue > fThreshold);
	}
	else
	{
		bFirstFlu = false;
		bFlag = false;
	}

	/* 更新前一帧的能量 */
	dPreviousEnergy = dEnergy;
	return 0;
}

/* 计算RMS值 */
double cAudioAnomaly::calculateRMS(double *fFrameData, int nFrameSize)
{
	double dSumSquares = 0.0;
	for(size_t i=0; i<nFrameSize; i++)
	{
		dSumSquares += fFrameData[i] * fFrameData[i]; /* 计算平方和 */ 
	}
	double dRms = std::sqrt(dSumSquares / nFrameSize); 
	return dRms;
}

/* 将RMS值转换为分贝 */
double cAudioAnomaly::calculateDB(double *fFrameData, int nFrameSize)
{
	double dRms = calculateRMS(fFrameData, nFrameSize);
	if (dRms <= 0.0)
	{
		return -INFINITY; /* 避免对数负数或零 */
	}
	return 20.0 * std::log10(dRms);
}