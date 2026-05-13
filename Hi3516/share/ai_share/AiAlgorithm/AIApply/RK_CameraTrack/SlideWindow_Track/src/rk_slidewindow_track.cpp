#include "rk_slidewindow_track.h"

/* 构造函数 */
RK_ClIDEWINDOW_TRACK::RK_ClIDEWINDOW_TRACK()
{
}
RK_ClIDEWINDOW_TRACK::RK_ClIDEWINDOW_TRACK(char *cFacenetPath) : feature_demo(cFacenetPath)
{
}
/* 析构函数 */
RK_ClIDEWINDOW_TRACK::~RK_ClIDEWINDOW_TRACK()
{
}

/* 余弦相识度 */
float RK_ClIDEWINDOW_TRACK::cosine_similarity(const float *vec1, const float *vec2, int size)
{
	float dot_product = std::inner_product(vec1, vec1 + size, vec2, 0.0f);

	float norm1 = std::sqrt(std::inner_product(vec1, vec1 + size, vec1, 0.0f));
	float norm2 = std::sqrt(std::inner_product(vec2, vec2 + size, vec2, 0.0f));

	float similarity = dot_product / (norm1 * norm2);
	return similarity;
}

/* 从人头坐标，扩充到上半身特征 */
void RK_ClIDEWINDOW_TRACK::get_head2body_feature(cv::Mat aImg, float x1, float y1, float x2, float y2, float *pFeatures)
{
	float ww, hh;
	std::vector<float> vAiPoints;
	ww = x2 - x1;
	hh = y2 - y1;
	x1 -= ww / 1.5;
	x2 += ww / 1.5;
	y2 += hh * 1.5;
	if (x1 < 0)
		x1 = 0;
	if (x2 > aImg.cols)
		x2 = aImg.cols;
	if (y2 > aImg.rows)
		y2 = aImg.rows;
	vAiPoints.insert(vAiPoints.end(), {x1, y1, x2, y2});
	feature_demo.GetFeatures(aImg, vAiPoints, pFeatures);
}

void resizePoint(cv::Mat aImg,float& x1,float& y1,float& x2,float& y2)
{
	float ww, hh;
	ww = x2 - x1;
	hh = y2 - y1;
	x1 -= ww / 1.5;
	x2 += ww / 1.5;
	y2 += hh * 3;
	if (x1 < 0)
		x1 = 0;
	if (x2 > aImg.cols)
		x2 = aImg.cols;
	if (y2 > aImg.rows)
		y2 = aImg.rows;
}

static float CalculateOverlap(cv::Mat aImg, float xmin0, float ymin0, float xmax0, float ymax0, float xmin1, float ymin1, float xmax1,
                              float ymax1)
{
  resizePoint(aImg,xmin0,ymin0,xmax0,ymax0);
  resizePoint(aImg,xmin1,ymin1,xmax1,ymax1);
  float w = fmax(0.f, fmin(xmax0, xmax1) - fmax(xmin0, xmin1) + 1.0);
  float h = fmax(0.f, fmin(ymax0, ymax1) - fmax(ymin0, ymin1) + 1.0);
  float i = w * h;
  float u = (xmax0 - xmin0 + 1.0) * (ymax0 - ymin0 + 1.0) + (xmax1 - xmin1 + 1.0) * (ymax1 - ymin1 + 1.0) - i;
  return u <= 0.f ? 0.f : (i / u);
}


/* 通过特征，获取最相似的一个框 */
bool RK_ClIDEWINDOW_TRACK::get_feature(cv::Mat aImg, std::vector<float> vPoints, std::vector<float> &vNewPoints)
{
	/* 缓存多少帧特征 */
	// int m_nNumFeatures=2;

	float fFeatureThes = 0.85;
	/* 扩充特征提取区域 */
	float x1, y1, x2, y2;
	/* 获取特征 */
    float pFeatures[512] = {0};
	/*缓存特征*/
    float pSaveFeatures[512] = {0};
	// static float *pFeatures = nullptr; 
	// static float *pSaveFeatures = nullptr; 

    // if(pFeatures == nullptr)
    // {
    //     pFeatures = new float[512];
    // }
    // // 将 pFeatures 中的数据置零
    // memset(pFeatures, 0, sizeof(float) * 512);

    // if(pSaveFeatures == nullptr)
    // {
    //     pSaveFeatures = new float[512];
    // }
    // // 将 pSaveFeatures 中的数据置零
    // memset(pSaveFeatures, 0, sizeof(float) * 512);

	/*计算余弦相似度，拿最大的框*/ 
	int max_idex = -1;
	float similarity = 0;
	float cs = 0;
	for (int i = 0; i < vPoints.size() / 6; i++)
	{
		x1 = vPoints[6 * i];
		y1 = vPoints[6 * i + 1];
		x2 = vPoints[6 * i + 2];
		y2 = vPoints[6 * i + 3];

		get_head2body_feature(aImg, x1, y1, x2, y2, pFeatures);
		if (_flag && (x1 > 0 && y1 > 0 && x2 < aImg.cols && y2 < aImg.rows))
		{
			// 复制vFeatures[0]的内容到pFeatures
			for (int ii = 0; ii < 512; ii++)
				pMyFeatures[ii] = pFeatures[ii];
			printf("特征获取成功\n");
			_flag = false;
		}

		cs = cosine_similarity(pFeatures, pMyFeatures, 512);
		
		if (cs > similarity)
		{
			max_idex = i;
			similarity = cs;
			/* 临时保存最大的特征-缓存 */
			for (int ii = 0; ii < 512; ii++)
				pSaveFeatures[ii] = pFeatures[ii];
		}
	}

	/* 如果特征队列ls存在数据，并且当前帧存在特征大于阈值 */
	float fFeatureScores = 0;
	/*需要更新坐标的位置，即和特征列表余弦相识度最大的下面*/ 
	int nMaxFeature = -1;
	float nNCs = 0; cs =0;
	
	if(max_idex != -1 && similarity > fSimilarityThreshold)
	{
		/* 如果特征列表没有保存满，则直接插入，否则更新特征最像的那个 */
		if(vFeatures.size() < m_nNumFeatures)
		{
			float* aNF = new float[512];
			for (int ii = 0; ii < 512; ii++)
			{
				/*赋值给最新的特征*/ 
				aNF[ii] = pSaveFeatures[ii];
			}
			vFeatures.push_back(aNF);
			// cv::Rect aRc = cv::Rect(int(vPoints[max_idex*6 +0]), int(vPoints[max_idex*6 +1]), int(vPoints[max_idex*6 +2] - vPoints[max_idex*6 +0]), int(vPoints[max_idex*6 +3] - vPoints[max_idex*6 +1]));
			// cv::Mat aResultImg2 = aImg.clone()(aRc);
			// cv::imwrite( "./vResultPoints3.jpg",aResultImg2);
		}
		for(int jj = 0;jj < vFeatures.size();jj++)
		{
			nNCs = cosine_similarity(vFeatures[jj], pSaveFeatures, 512);
			// printf("余弦相识度：%f\n",nNCs);
			fFeatureScores += nNCs;
			/*获取更大的相似度下标*/ 
			if(nNCs > cs )
			{
				cs = nNCs;
				nMaxFeature = jj;
			}
		}
		if(vFeatures.size() >= m_nNumFeatures && nMaxFeature !=-1)
		{
			/*替换特征*/ 
			for (int ii = 0; ii < 512; ii++)
			{
				/*赋值给最新的特征*/ 
				vFeatures[nMaxFeature][ii] = pSaveFeatures[ii];
			}
			// cv::Rect aRc = cv::Rect(int(vPoints[max_idex*6 +0]), int(vPoints[max_idex*6 +1]), int(vPoints[max_idex*6 +2] - vPoints[max_idex*6 +0]), int(vPoints[max_idex*6 +3] - vPoints[max_idex*6 +1]));
			// cv::Mat aResultImg3 = aImg.clone()(aRc);
			// cv::imwrite( "./vResultPoints4.jpg",aResultImg3);
		}
		/* 判断分数是否符合要求 */
		fFeatureScores = fFeatureScores*1.0/vFeatures.size();
		// printf("综合得分为：%f,%ld\n",fFeatureScores,vFeatures.size());
		if(fFeatureScores > fFeatureThes)
		{
			vNewPoints.insert(vNewPoints.end(), {vPoints[6 * max_idex], vPoints[6 * max_idex + 1], vPoints[6 * max_idex + 2], vPoints[6 * max_idex + 3]});
			/*复制vFeatures[0]的内容到pFeatures*/ 
			for (int ii = 0; ii < 512; ii++)
				pMyFeatures[ii] = pSaveFeatures[ii];
		}
	}
	return true;
}

/* 更新滑动窗口的信息 */
void RK_ClIDEWINDOW_TRACK::nextImg(cv::Mat aFrame, std::vector<float> vPoints, std::vector<float> &vNewPoints)
{
	/* 匹配算法 */
	get_feature(aFrame, vPoints, vNewPoints);
}

/* 清空保存的特征点 */
void RK_ClIDEWINDOW_TRACK::clear_features()
{
    for(auto item : vFeatures)
    {
        delete []item;
    }
    vFeatures.clear();
}
