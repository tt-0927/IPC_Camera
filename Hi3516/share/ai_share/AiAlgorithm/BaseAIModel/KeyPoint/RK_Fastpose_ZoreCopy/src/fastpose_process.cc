#include "fastpose_process.h"

namespace Fastpose_NS{
void PostProcessImage(float* pOutputs, std::vector<float>& vPoints)
{
	/* 遍历26个人体点，进行存储 */
	for(int p=0;p<26;p++)
	{
		vPoints.push_back(pOutputs[p*3+0]);
		vPoints.push_back(pOutputs[p*3+1]);
		vPoints.push_back(pOutputs[p*3+2]);
	}
}
};
