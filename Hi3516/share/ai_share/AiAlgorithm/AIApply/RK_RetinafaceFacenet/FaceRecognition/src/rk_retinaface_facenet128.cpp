
#include "rk_retinaface_facenet128.h"

#include <chrono>

/* 构造函数 -- 初始化变量 */
RETINAFACE_FACENET::RETINAFACE_FACENET()
{
    aSrcImg.create(nHeight, nWidth, CV_8UC3);    // 创建一个1920x1080的彩色图像
}

RETINAFACE_FACENET::RETINAFACE_FACENET(int nInitImgWidth, int nInitImgHeight)
{
    nHeight = nInitImgHeight;
    nWidth  = nInitImgWidth;
    aSrcImg.create(nHeight, nWidth, CV_8UC3);    // 创建一个1920x1080的彩色图像
}

/* 析构函数 */
RETINAFACE_FACENET::~RETINAFACE_FACENET()
{
}

/* 人脸矫正 */
void Face_Alignment(cv::Mat& aFace, float fX, float fY)
{

    // Calculate the angle of the eye line with respect to the horizontal line
    float fAngle;
    if (fX == 0)
    {
        fAngle = 0;
    }
    else
    {
        fAngle = std::atan(fY / fX) * 180 / M_PI;
    }

    cv::Point2f aCenter(aFace.cols / 2.0f, aFace.rows / 2.0f);
    cv::Mat     aRotationMatrix = cv::getRotationMatrix2D(aCenter, fAngle, 1.0);

    // Apply affine transformation to the image
    cv::warpAffine(aFace, aFace, aRotationMatrix, aFace.size());
}

/* 余弦相识度 */
/******************************************************************************************************************************/
float RETINAFACE_FACENET::CosineSimilarity(const float* vec1, const float* vec2, int size)
{
    float dot_product = std::inner_product(vec1, vec1 + size, vec2, 0.0f);

    float norm1 = std::sqrt(std::inner_product(vec1, vec1 + size, vec1, 0.0f));
    float norm2 = std::sqrt(std::inner_product(vec2, vec2 + size, vec2, 0.0f));

    float similarity = dot_product / (norm1 * norm2);
    return similarity;
}

/******************************************************************************************************************************/
/*视频流的识别 */
int RETINAFACE_FACENET::RetinafaceFacenetBgr(char* pDataBuffer, RK_FACES_DETECT& FaceDetect, RK_FACE_FEATURE& Facenet128, std::vector<PEOPLEFEATURES>& vAllFeature)
{
    if (pDataBuffer)
    {
        float aFacenet128Value[128];
        /* 清除之前存储的人脸特征 */
        vAllFeature.clear();

        memcpy(aSrcImg.data, pDataBuffer, nHeight * nWidth * 3);

        /* 人脸检测 */
        std::vector<int> vBoxPoint;
        FaceDetect.DetectFaceBgr(aSrcImg, vBoxPoint);


        // cv::imwrite("./saveimg.jpg",aSrcImg);
        for (int i = 0; i < (vBoxPoint.size() / 14); i++)
        {

            /* 截取人脸 */
            cv::Point topLeft(vBoxPoint[i * 14 + 0], vBoxPoint[i * 14 + 1]);
            cv::Point bottomRight(vBoxPoint[i * 14 + 2], vBoxPoint[i * 14 + 3]);
            aFace = aSrcImg(cv::Rect(topLeft, bottomRight));
            if (aFace.empty())
            {
                return -1;
            }
            cv::resize(aFace, aFace, cv::Size(160, 160));
            /* 人脸矫正 */
            Face_Alignment(aFace, vBoxPoint[i * 14 + 4] - vBoxPoint[i * 14 + 6], vBoxPoint[i * 14 + 5] - vBoxPoint[i * 14 + 7]);
            /* 人脸特征点提取 */
            Facenet128.DetectFaceRgb(aFace, aFacenet128Value);

            /* 定义一个存放结果的结构体 */
            PEOPLEFEATURES sPFeatures;
            for (int bindex = 0; bindex < 4; bindex++)
            {
                sPFeatures.fBoxs[bindex] = vBoxPoint[i * 14 + bindex];
            }
            for (int nfindex = 0; nfindex < 128; nfindex++)
            {
                sPFeatures.fFeatures[nfindex] = aFacenet128Value[nfindex];
                // printf("===========%f\n",sPFeatures.fFeatures[nfindex]);
            }
            sPFeatures.nResult = 0;
            vAllFeature.push_back(sPFeatures);
        }

        return 1;
    }
    else
    {
        std::cout << "data_buf error!!!" << std::endl;
    }

    return -1;
}

/*视频流的识别 */
int RETINAFACE_FACENET::RetinafaceFacenetBgr(cv::Mat inMat, RK_FACES_DETECT& FaceDetect, RK_FACE_FEATURE& Facenet128, std::vector<PEOPLEFEATURES>& vAllFeature)
{
    if (!inMat.empty())
    {
        float aFacenet128Value[128];
        /* 清除之前存储的人脸特征 */
        vAllFeature.clear();

        /* 人脸检测 */
        std::vector<int> vBoxPoint;
        FaceDetect.DetectFaceBgr(inMat, vBoxPoint);


        // cv::imwrite("./saveimg.jpg",inMat);
        for (int i = 0; i < (vBoxPoint.size() / 14); i++)
        {

            /* 截取人脸 */
            cv::Point topLeft(vBoxPoint[i * 14 + 0], vBoxPoint[i * 14 + 1]);
            cv::Point bottomRight(vBoxPoint[i * 14 + 2], vBoxPoint[i * 14 + 3]);
            aFace = inMat(cv::Rect(topLeft, bottomRight));
            if (aFace.empty())
            {
                return -1;
            }
            cv::resize(aFace, aFace, cv::Size(160, 160));
            /* 人脸矫正 */
            Face_Alignment(aFace, vBoxPoint[i * 14 + 4] - vBoxPoint[i * 14 + 6], vBoxPoint[i * 14 + 5] - vBoxPoint[i * 14 + 7]);
            /* 人脸特征点提取 */
            Facenet128.DetectFaceRgb(aFace, aFacenet128Value);

            /* 定义一个存放结果的结构体 */
            PEOPLEFEATURES sPFeatures;
            for (int bindex = 0; bindex < 4; bindex++)
            {
                sPFeatures.fBoxs[bindex] = vBoxPoint[i * 14 + bindex];
            }
            for (int nfindex = 0; nfindex < 128; nfindex++)
            {
                sPFeatures.fFeatures[nfindex] = aFacenet128Value[nfindex];
                // printf("===========%f\n",sPFeatures.fFeatures[nfindex]);
            }
            sPFeatures.nResult = 0;
            vAllFeature.push_back(sPFeatures);
        }

        return 1;
    }
    else
    {
        std::cout << "data_buf error!!!" << std::endl;
    }

    return -1;
}
