
#ifndef _RKNN_STUDENT_BEHAVIOR_POSTPROCESS_H_
#define _RKNN_STUDENT_BEHAVIOR_POSTPROCESS_H_

#include <stdint.h>
#include <vector>
#include <string>

int post_process_v1(float *input0, float *input1, float *input2, int model_in_h, int model_in_w, float conf_threshold,
                 float nms_threshold, std::vector<float> &vPoints);

#endif //_RKNN_STUDENT_BEHAVIOR_POSTPROCESS_H_
