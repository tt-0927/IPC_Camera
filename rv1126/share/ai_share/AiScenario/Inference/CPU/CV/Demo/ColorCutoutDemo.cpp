
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "ColorCutout.hpp"

using namespace ColorCutout_NS;

int main()
{
	CutoutParam_S sCutoutParam;
	CColorCutout* demo = new CColorCutout();

	sCutoutParam.aInputImage = cv::imread("test.jpg");
	sCutoutParam.eBgColor    = BLUE; /* 绿幕 */
	sCutoutParam.fColorThres = 175.0; /* 颜色阈值 */
	sCutoutParam.fEdgeThres  = 0.9;	  /* 边缘细腻度阈值 */

    bool result = demo->inference(sCutoutParam);
    cv::imwrite("./ColorCutoutDemo.png", sCutoutParam.aOutputImage);
    
    delete demo;
    
	return 0;
}
