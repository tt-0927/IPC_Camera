
#ifndef _RKNN_DBOCR_PROCEE_H_
#define _RKNN_DBOCR_PROCEE_H_

#include <stdint.h>
#include <vector>
#include <string>
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include <clipper.hpp>

using namespace ClipperLib;

#define OBJ_CLASS_NUM 1
#define PROP_BOX_SIZE     (5+OBJ_CLASS_NUM)
#define LABEL_NUMS 20



int post_process(float *input0,int dest_width,int dest_height,float fThreshold,float fBoxThresh,int nMaxCandidates,float fUnclipRatio,std::vector<std::vector<cv::Point2f>>& boxes,std::vector<float>& new_scores);

#endif //_RKNN_DBOCR_PROCEE_H_

