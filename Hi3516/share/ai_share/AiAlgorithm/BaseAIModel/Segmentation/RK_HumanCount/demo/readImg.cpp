#include "readImg.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

int read_img(char *pModelPath, char *ImgData)
{
	cv::Mat test = cv::imread(pModelPath);
	int dataSize = test.rows * test.cols * test.channels();
	ImgData = new char[dataSize];
	memcpy(ImgData, test.data, dataSize);
	return 0;
}
