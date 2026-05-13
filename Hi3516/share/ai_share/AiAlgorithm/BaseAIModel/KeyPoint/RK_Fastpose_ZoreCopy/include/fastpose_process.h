
#ifndef _RKNN_FASTPOSE_POSTPROCESS_H_
#define _RKNN_FASTPOSE_POSTPROCESS_H_

#include <stdint.h>
#include <vector>
#include <string>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"

/* 模型的后处理 */
namespace Fastpose_NS{
	void PostProcessImage(float* pOutputs, std::vector<float>& vPoints);
}
#endif //_RKNN_FASTPOSE_POSTPROCESS_H_
