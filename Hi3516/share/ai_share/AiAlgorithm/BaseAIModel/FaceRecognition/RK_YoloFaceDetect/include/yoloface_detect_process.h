
#ifndef __RK_YOLOFACE_DETECT_POSTPROCESS_H__
#define __RK_YOLOFACE_DETECT_POSTPROCESS_H__ 

#include <stdint.h>
#include <vector>
#include <string>

const int post_process(float *input0, float *input1, float *input2, int model_in_h, int model_in_w, float conf_threshold,
                 float nms_threshold, std::vector<float> &vPoints);

#endif //__RK_YOLOFACE_DETECT_POSTPROCESS_H__
