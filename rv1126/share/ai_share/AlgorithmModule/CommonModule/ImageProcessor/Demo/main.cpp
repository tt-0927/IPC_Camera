/*
 * @FilePath     : main.cpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-12 14:50:33
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-01-25 15:51:22
 * @Description  :
 */
#include "dlog.h"
#include "ImageProcessor.hpp"

void saveBinaryDataToFile(const char* data, size_t size, const char* filePath)
{
    // 打开文件以写入二进制数据
    FILE* file = fopen(filePath, "wb");

    if (file == NULL)
    {
        perror("Error opening file");
        return;
    }

    // 写入二进制数据到文件
    size_t written = fwrite(data, 1, size, file);

    if (written != size)
    {
        perror("Error writing to file");
    }
    else
    {
        printf("Binary data successfully saved to %s\n", filePath);
    }

    // 关闭文件
    fclose(file);
}

int main(int argc, char const* argv[])
{
    CImageProcessor Image;
    char*           pchData   = nullptr;
    int             nDataSize = 0;

    /* 画布参数 */
    CImageProcessor::DrawParam_S stInfo;
    stInfo.clear();
    stInfo.nOutHeight = 1080;
    stInfo.nOutWidth  = 1920;
    stInfo.enOutType  = CImageProcessor::RGBA8888;
    stInfo.nR         = 255;
    stInfo.nG         = 255;
    stInfo.nB         = 255;
    stInfo.nA         = 0;


    /* 框参数 */
    CImageProcessor::BoxInfo_S stBoxInfo;
    stBoxInfo.clear();
    std::list<CImageProcessor::BoxInfo_S> listBoxInfo;
    stBoxInfo.nX = 100;
    stBoxInfo.nY = 100;
    stBoxInfo.nW = 500;
    stBoxInfo.nH = 500;
    listBoxInfo.push_back(stBoxInfo);

    stBoxInfo.nX = 10;
    stBoxInfo.nY = 10;
    stBoxInfo.nW = 500;
    stBoxInfo.nH = 500;
    listBoxInfo.push_back(stBoxInfo);

    stBoxInfo.nX = 500;
    stBoxInfo.nY = 500;
    stBoxInfo.nW = 500;
    stBoxInfo.nH = 500;
    listBoxInfo.push_back(stBoxInfo);

    /* 字参数 */
    CImageProcessor::LabelInfo_S stLabelInfo;
    stLabelInfo.clear();
    std::list<CImageProcessor::LabelInfo_S> listLabelInfo;

    stLabelInfo.nLabelR  = 255;
    stLabelInfo.nLabelG  = 0;
    stLabelInfo.nLabelB  = 0;
    stLabelInfo.nLabelA  = 255;
    stLabelInfo.nX       = 500;
    stLabelInfo.nY       = 500;
    stLabelInfo.strLabel = "你好1111111111111111111";
    listLabelInfo.push_back(stLabelInfo);

    /* 绘画 */
    Image.drawBoxAndLabel(stInfo, listBoxInfo, listLabelInfo, &pchData, nDataSize, "/opt/bl/.config/design_data/simfang.ttf");

    /* 保存图片 */
    Image.saveImage(stInfo.enOutType, pchData, stInfo.nOutWidth, stInfo.nOutHeight, "./test.jpeg");
    saveBinaryDataToFile(pchData, nDataSize, "./test.rgba");

    if (pchData)
    {
        delete pchData;
        pchData = nullptr;
    }


    return 0;
}
