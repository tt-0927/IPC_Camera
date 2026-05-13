#include "rk_slidewindow_track.h"

/* 构造函数 */
RK_ClIDEWINDOW_TRACK::RK_ClIDEWINDOW_TRACK()
{
	MainBox.x = 640;
	MainBox.y = 640;
	MainBox.w = 160;
	MainBox.h = 320;
}
RK_ClIDEWINDOW_TRACK::RK_ClIDEWINDOW_TRACK(char* cFacenetPath):feature_demo(cFacenetPath)
{
	MainBox.x = 640;
	MainBox.y = 640;
	MainBox.w = 160;
	MainBox.h = 320;
}
RK_ClIDEWINDOW_TRACK::RK_ClIDEWINDOW_TRACK(float fXc,float fYc, float fWc, float fHc, char* cFacenetPath):feature_demo(cFacenetPath)
{
	MainBox.x = fXc;
	MainBox.y = fYc;
	MainBox.w = fWc;
	MainBox.h = fHc;
}
/* 析构函数 */
RK_ClIDEWINDOW_TRACK::~RK_ClIDEWINDOW_TRACK()
{
	
}

/* 获取人物距离滑动窗口中心点的距离 */
float RK_ClIDEWINDOW_TRACK::Point_instance(float Sx, float Sy)
{
	return std::sqrt(std::pow((Sx - MainBox.w / 2), 2) + std::pow((Sy - MainBox.h / 2), 2));
}

/* 获取距离框中心最近的框*/
void RK_ClIDEWINDOW_TRACK::get_id(std::vector<float> vPoints,std::vector<float>& vNewPoints)
{
	float instance = 1000000;
	float x1,y1,x2,y2;
	for (int i = 0; i < vPoints.size()/6; i++)
	{
	    	float instance_ = Point_instance((vPoints[6*i] + vPoints[6*i+2]) / 2, (vPoints[6*i+1] + vPoints[6*i+3]) / 2);
	    	if(instance>instance_)
	    	{
	    		x1 = vPoints[6*i];
	    		y1 = vPoints[6*i+1];
	    		x2 = vPoints[6*i+2];
	    		y2 = vPoints[6*i+3];
	    		instance = instance_;
	    	}
	}
	if (instance != 1000000)
        {
        	vNewPoints.insert(vNewPoints.end(),{x1,y1,x2,y2});
        }	
}

/* 余弦相识度 */
float cosine_similarity(const float* vec1, const float* vec2, int size) {
    float dot_product = std::inner_product(vec1, vec1 + size, vec2, 0.0f);
    
    float norm1 = std::sqrt(std::inner_product(vec1, vec1 + size, vec1, 0.0f));
    float norm2 = std::sqrt(std::inner_product(vec2, vec2 + size, vec2, 0.0f));
    
    float similarity = dot_product / (norm1 * norm2);
    return similarity;
}

/* 从人头坐标，扩充到上半身特征 */
void RK_ClIDEWINDOW_TRACK::get_head2body_feature(cv::Mat aImg,float x1,float y1, float x2, float y2 ,float* pFeatures)
{
	float ww,hh;
	std::vector<float> vAiPoints;
	ww = x2-x1;
	hh = y2-y1;
	x1 -= ww/1.5;
	x2 += ww/1.5;
	y2 += hh*1.5;
	if(x1<0)x1=0;
	if(x2>MainBox.w)x2=MainBox.w;
	if(y2>MainBox.h)y2=MainBox.h;
	vAiPoints.insert(vAiPoints.end(),{x1,y1,x2,y2});
	feature_demo.GetFeatures(aImg,vAiPoints,pFeatures);
}

/* 通过特征，获取最相似的一个框 */
bool RK_ClIDEWINDOW_TRACK::get_feature(cv::Mat aImg,std::vector<float> vPoints,std::vector<float>& vNewPoints)
{
	/* 扩充特征提取区域 */
	float x1,y1,x2,y2;
	/* 获取特征 */	
	float* pFeatures = new float[512];
	float* pSaveFeatures = new float[512];
	/*计算余弦相似度，拿最大的框*/ 
	int max_idex=-1;
	float similarity=0;
	float cs=0;
	for (int i = 0; i < vPoints.size()/6; i++)
	{
		x1 = vPoints[6*i];
		y1 = vPoints[6*i+1];
		x2 = vPoints[6*i+2];
		y2 = vPoints[6*i+3];
    		
    	get_head2body_feature(aImg,x1,y1, x2, y2 ,pFeatures);

		/*第一帧特征在滑动窗口内才分析，功能完善*/
		if(_flag && (x1 > 0 && y1 > 0 && x2 < aImg.cols && y2 < aImg.rows))
		{
			/*复制vFeatures[0]的内容到pFeatures*/ 
			for(int nNum=0;nNum<512;nNum++)
				pMyFeatures[nNum]= pFeatures[nNum]; 
			//printf("特征获取成功\n");
			_flag = false;
		}
		
		cs = cosine_similarity(pFeatures, pMyFeatures, 512);
		//printf("================ 相似度：%f ======================\n",cs);
		if(cs>similarity)
		{
			max_idex = i;
			similarity = cs;
			/* 临时保存最大的特征 */
			for(int nNum = 0;nNum < 512; nNum++)
				pSaveFeatures[nNum]=pFeatures[nNum];
		}
	}
	if(max_idex!=-1 and similarity>fSimilarityThreshold)
	{
		vNewPoints.insert(vNewPoints.end(),{vPoints[6*max_idex],vPoints[6*max_idex+1],vPoints[6*max_idex+2],vPoints[6*max_idex+3]});
		for(int nNum = 0 ; nNum < 512; nNum++)
				pMyFeatures[nNum]=pSaveFeatures[nNum];
	}
	

	return true;
}

/* 更换下一个截图框 */
void RK_ClIDEWINDOW_TRACK::get_newPoint(cv::Mat aFrame, std::vector<float> vPoints)
{
	if (vPoints.size() != 0)
        {
        	int x1 = vPoints[0];
        	int y1 = vPoints[1];
        	int x2 = vPoints[2];
        	int y2 = vPoints[3];
        	int xp = (x1 + x2 - MainBox.w) / 2;
        	int yp = (y1 + y2 - MainBox.h) / 2;
	        MainBox.x += xp;
        	MainBox.y += yp;
        	/* 调整滑动窗口 */
		//changeBox(aFrame);
        } 
}

/* 滑动窗口框调整 */
/*void RK_ClIDEWINDOW_TRACK::changeBox(cv::Mat aFrame)
{
    int nWidth = aFrame.cols;
    int nHeight = aFrame.rows;
    if (MainBox.x < MainBox.w / 2)
    {
    	 MainBox.x = MainBox.w / 2;
    }
    if (MainBox.y < MainBox.h / 2)
    {
   	MainBox.y = MainBox.h / 2;
    }           
    if (MainBox.x + MainBox.w / 2 > nWidth)
    {
       MainBox.x = nWidth - MainBox.w / 2;
    }
    if (MainBox.y + MainBox.h / 2 > nHeight)
    {
       MainBox.y = nHeight - MainBox.h / 2;
    }
}*/
void RK_ClIDEWINDOW_TRACK::changeBox(cv::Mat aFrame,int& x1_, int& y1_, int& x2_, int& y2_,
	int& top, int& left, int& bottom, int& right)
{
        x1_ = MainBox.x-MainBox.w/2;
        y1_ = MainBox.y-MainBox.h/2;
        x2_ = MainBox.x+MainBox.w/2;
        y2_ = MainBox.y+MainBox.h/2;
        top=left = 0;
        bottom = MainBox.h;
        right = MainBox.w;
        if (MainBox.x < MainBox.w / 2)
        {
		x1_ = 0;
         	left = MainBox.w/2-MainBox.x;
        }
            
        if (MainBox.y < MainBox.h / 2)
        {
        	y1_ = 0;
            	top = MainBox.h/2-MainBox.y;
        }
        if (MainBox.x + MainBox.w / 2 > aFrame.cols)
        {
        	x2_ = aFrame.cols;
          	right = MainBox.w + (aFrame.cols-MainBox.x);
        }
        if (MainBox.y + MainBox.h / 2 > aFrame.rows)
        {
        	y2_ = aFrame.rows;
           	bottom = MainBox.h + (aFrame.rows-MainBox.y);
        }
        // 调整框和裁剪的位置
        if(x2_-x1_ != right-left)
        {
		int s = x2_-x1_-right+left;
		if(s+right>MainBox.w)
		{
			left -= s - (MainBox.w - right);
			right = MainBox.w;
		}
		else
			right += s;
        }
        if(y2_-y1_ != bottom-top)
        {
            int s = y2_-y1_-bottom+top;
            if(s+bottom>MainBox.h)
            {
                top -= s - (MainBox.h - bottom);
                bottom = MainBox.h;
            }
            else
                bottom += s;
        }
        
}


/* 输入原图获取滑动窗口图 */
/*void RK_ClIDEWINDOW_TRACK::getImg(cv::Mat aFrame,cv::Mat& aSlideFrame)
{
	// 调整滑动窗口 //
	changeBox(aFrame);
	// 坐标转换 //
	int x1 = static_cast<int>(MainBox.x - MainBox.w / 2);
	int y1 = static_cast<int>(MainBox.y - MainBox.h / 2);
	int x2 = static_cast<int>(MainBox.x + MainBox.w / 2);
	int y2 = static_cast<int>(MainBox.y + MainBox.h / 2);
	// 截取滑动窗口 //
	cv::Rect roi(x1, y1, x2 - x1, y2 - y1);
	aSlideFrame = aFrame(roi);
}*/
void RK_ClIDEWINDOW_TRACK::getImg(cv::Mat aFrame,cv::Mat& aSlideFrame)
{
	int x1_, y1_, x2_, y2_, top, left, bottom, right;
	/* 调整滑动窗口 */
	changeBox(aFrame,x1_, y1_, x2_, y2_,top, left, bottom, right);
	/* 截取滑动窗口 */
	aSlideFrame = cv::Mat::ones(MainBox.h, MainBox.w, CV_8UC3) * 128;
	cv::Mat roi = aSlideFrame(cv::Range(top, bottom), cv::Range(left, right)); 
	cv::Mat crop = aFrame(cv::Range(y1_, y2_), cv::Range(x1_, x2_)); 
	crop.copyTo(roi);
}

/* 更新滑动窗口的信息 */
void RK_ClIDEWINDOW_TRACK::nextImg(cv::Mat aFrame, std::vector<float> vPoints, cv::Mat& aNewFrame)
{
	std::vector<float> vNewPoints;
	/* 匹配算法 */
	//get_id(vPoints,vNewPoints);
	get_feature(aNewFrame,vPoints, vNewPoints);
	/* 更新x,y */
	get_newPoint(aFrame, vNewPoints);
	for (int i = 0; i < vNewPoints.size()/4; i++)
	{
	    cv::rectangle(aNewFrame, cv::Point(vNewPoints[0], vNewPoints[1]), cv::Point(vNewPoints[2], vNewPoints[3]),  cv::Scalar(0, 0, 255), 3);
	}
}
