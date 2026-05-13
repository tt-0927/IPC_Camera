/*
 * @FilePath     : ImageProcessor.hpp
 * @Author       : yanzeh yanzeh@kfb.cn
 * @Date         : 2024-01-12 10:16:49
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-03-14 11:28:49
 * @Description  : 图片处理器
 */
#pragma once

#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>

#include "BlError.h"

class CImageProcessor
{
public:

    /* 数据格式类型 */
    typedef enum _DataFormatType_
    {
        RGB888 = 0,
        BGR888,
        RGBA8888
    } DataFormatType_E;

    /* 文件格式类型 */
    typedef enum _FileFormatType_
    {
        JPEG = 0,
        PNG,
    } FileFormatType_E;

    /* 标签信息 */
    typedef struct _LabelInfo_
    {
        int nX;               /* x坐标 */
        int nY;               /* y坐标 */

        std::string strLabel; /* 标签 */
        /* 使用系统函数时的参数 */
        int         nThickness; /* 标签厚度 */
        int         nFontFace;  /* 字体类型 cv::HersheyFonts */
        double      dFontScale; /* 文字大小的比例因子 */
        /* 使用自定义字库时的参数 */
        int         nFontSize; /* 字体大小 */

        int nLabelR;
        int nLabelG;
        int nLabelB;
        int nLabelA; /* 框标签的颜色 */

        _LabelInfo_()
        {
            clear();
        }

        void clear()
        {
            nX = 0;
            nX = 0;

            strLabel.clear();
            nThickness = 1;
            nFontFace  = 1;
            dFontScale = 1.0;
            nFontSize  = 18;

            nLabelR = 0;
            nLabelG = 0;
            nLabelB = 0;
            nLabelA = 255;
        }
    } LabelInfo_S;

    /* 坐标信息 */
    typedef struct _BoxInfo_
    {
        int nX;              /* x坐标 */
        int nY;              /* y坐标 */
        int nW;              /* 宽 */
        int nH;              /* 高 */

        LabelInfo_S stLabel; /* 框标签信息 */

        int nThickness;      /* 正数：框的厚度 负数：填充满*/

        int nBoxR;
        int nBoxG;
        int nBoxB;
        int nBoxA; /* 框的颜色 */

        _BoxInfo_()
        {
            clear();
        }

        void clear()
        {
            nX = 0;
            nX = 0;
            nW = 0;
            nH = 0;

            stLabel.clear();
            nThickness = 1;

            nBoxR = 0;
            nBoxG = 0;
            nBoxB = 0;
            nBoxA = 255;
        }
    } BoxInfo_S;

    /* 线信息 */
    typedef struct _LineInfo_
    {
        int nX1;        /* x坐标 */
        int nY1;        /* y坐标 */
        int nX2;        /* x坐标 */
        int nY2;        /* y坐标 */

        int nThickness; /* 线的厚度 */

        int nR;
        int nG;
        int nB;
        int nA; /* 线的颜色 */

        _LineInfo_()
        {
            clear();
        }

        void clear()
        {
            nX1 = 0;
            nY1 = 0;
            nX2 = 0;
            nY2 = 0;

            nThickness = 1;

            nR = 0;
            nG = 0;
            nB = 0;
            nA = 255;
        }
    } LineInfo_S;

    /* 圆信息 */
    typedef struct _CircleInfo_
    {
        int nCenterX;   /* 圆心x坐标 */
        int nCenterY;   /* 圆心y坐标 */
        int nRadius;    /* 半径 */

        int nThickness; /* 线的厚度 -1；填充*/

        int nR;
        int nG;
        int nB;
        int nA; /* 线的颜色 */

        _CircleInfo_()
        {
            clear();
        }

        void clear()
        {
            nCenterX = 0;
            nCenterY = 0;
            nRadius  = 0;

            nThickness = 1;

            nR = 0;
            nG = 0;
            nB = 0;
            nA = 255;
        }
    } CircleInfo_S;

    /* 画框参数结构体 */
    typedef struct _DrawParam_
    {
        int              nOutWidth;  /* 输出宽 */
        int              nOutHeight; /* 输出高 */
        DataFormatType_E enOutType;  /* 输出数据格式 */

        int nR;
        int nG;
        int nB;
        int nA; /* 画布的颜色 */

        _DrawParam_()
        {
            clear();
        }

        void clear()
        {
            nOutWidth  = 0;
            nOutHeight = 0;
            enOutType  = RGBA8888;

            nR = 0;
            nG = 0;
            nB = 0;
            nA = 0;
        }
    } DrawParam_S;

public:

    /**
     * @brief 在cv::Mat的基础上划线
     * @param [cv::Mat] editImage: 编辑的对象
     * @param [std::list<LineInfo_S>] listLineInfo: 线信息
     * @return [*]
     * @note
     */
    BlError_E drawLines(cv::Mat&              editImage,
                        std::list<LineInfo_S> listLineInfo);

    /**
     * @brief 写字
     * @param [cv::Mat] editImage: 编辑的对象
     * @param [std::list<LabelInfo_S>] listLabelInfo: Label信息
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note
     */
    BlError_E drawLabel(cv::Mat&               editImage,
                        std::list<LabelInfo_S> listLabelInfo);

    /**
     * @brief 写字
     * @param [cv::Mat] editImage: 编辑的对象
     * @param [std::list<LabelInfo_S>] listLabelInfo: Label信息
     * @param [std::string] strFontPath: 使用自定义字库，字库路径
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note
     */
    BlError_E drawLabel(cv::Mat&               editImage,
                        std::list<LabelInfo_S> listLabelInfo,
                        std::string            strFontPath);

    /**
     * @brief 在cv::Mat的基础上划圆
     * @param [cv::Mat] editImage: 编辑的对象
     * @param [std::list<CircleInfo_S>] listCircleInfo: 圆信息
     * @param [std::string] strFontPath: 使用自定义字库，字库路径
     * @return [*]
     * @note
     */
    BlError_E drawCenter(cv::Mat&                editImage,
                         std::list<CircleInfo_S> listCircleInfo,
                         std::string             strFontPath);

    /**
     * @brief 画框
     * @param [cv::Mat] editImage: 编辑的对象
     * @param [std::list<BoxInfo_S>] listBoxInfo: 画框信息
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note
     */
    BlError_E drawBox(cv::Mat&             editImage,
                      std::list<BoxInfo_S> listBoxInfo);

    /**
     * @brief 画框
     * @param [cv::Mat] editImage: 编辑的对象
     * @param [std::list<BoxInfo_S>] listBoxInfo: 画框信息
     * @param [std::string] strFontPath: 使用自定义字库，字库路径
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note
     */
    BlError_E drawBox(cv::Mat&             editImage,
                      std::list<BoxInfo_S> listBoxInfo,
                      std::string          strFontPath);

    /**
     * @brief 创建一个透明的图片，画画
     * @param [DrawParam_S] stParam: 参数
     * @param [std::list<BoxInfo_S>] listBoxInfo: 画框信息
     * @param [char**] pchOutData: 输出的数据指针
     * @param [int&] nOutDataSize: 输出数据的大小
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note pchOutData :new出来的，需要调用者释放
     */
    BlError_E draw(DrawParam_S          stParam,
                   std::list<BoxInfo_S> listBoxInfo,
                   char**               pchOutData,
                   int&                 nOutDataSize);

    /**
     * @brief 创建一个透明的图片，画画
     * @param [DrawParam_S] stParam: 参数
     * @param [std::list<BoxInfo_S>] listBoxInfo: 画框信息
     * @param [char**] pchOutData: 输出的数据指针
     * @param [int&] nOutDataSize: 输出数据的大小
     * @param [std::string] strFontPath: 使用自定义字库，字库路径
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note pchOutData :new出来的，需要调用者释放
     */
    BlError_E draw(DrawParam_S          stParam,
                   std::list<BoxInfo_S> listBoxInfo,
                   char**               pchOutData,
                   int&                 nOutDataSize,
                   std::string          strFontPath);




    /**
     * @brief 创建一个透明的图片，画画
     * @param [DrawParam_S] stParam: 参数
     * @param [std::list<BoxInfo_S>] listBoxInfo: 画框信息
     * @param [std::list<LabelInfo_S>] listLabelInfo: 标签信息信息
     * @param [char**] pchOutData: 输出的数据指针
     * @param [int&] nOutDataSize: 输出数据的大小
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note pchOutData :new出来的，需要调用者释放
     */
    BlError_E draw(DrawParam_S            stParam,
                   std::list<BoxInfo_S>   listBoxInfo,
                   std::list<LabelInfo_S> listLabelInfo,
                   char**                 pchOutData,
                   int&                   nOutDataSize);

    /**
     * @brief 创建一个透明的图片，画画
     * @param [DrawParam_S] stParam: 参数
     * @param [std::list<BoxInfo_S>] listBoxInfo: 画框信息
     * @param [std::list<LabelInfo_S>] listLabelInfo: 标签信息信息
     * @param [char**] pchOutData: 输出的数据指针
     * @param [int&] nOutDataSize: 输出数据的大小
     * @param [std::string] strFontPath: 使用自定义字库，字库路径
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note pchOutData :new出来的，需要调用者释放
     */
    BlError_E draw(DrawParam_S            stParam,
                   std::list<BoxInfo_S>   listBoxInfo,
                   std::list<LabelInfo_S> listLabelInfo,
                   char**                 pchOutData,
                   int&                   nOutDataSize,
                   std::string            strFontPath);

    /**
     * @brief 创建一个透明的图片，画画
     * @param [DrawParam_S] stParam: 参数
     * @param [std::list<BoxInfo_S>] listBoxInfo: 画框信息
     * @param [std::list<LabelInfo_S>] listLabelInfo: 标签信息信息
     * @param [char**] pchOutData: 输出的数据指针
     * @param [int&] nOutDataSize: 输出数据的大小
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note pchOutData :new出来的，需要调用者释放
     */
    BlError_E draw(DrawParam_S            stParam,
                   std::list<BoxInfo_S>   listBoxInfo,
                   std::list<LabelInfo_S> listLabelInfo,
                   std::list<LineInfo_S>  listLineInfo,
                   char**                 pchOutData,
                   int&                   nOutDataSize);

    /**
     * @brief 创建一个透明的图片，画画
     * @param [DrawParam_S] stParam: 参数
     * @param [std::list<BoxInfo_S>] listBoxInfo: 画框信息
     * @param [std::list<LabelInfo_S>] listLabelInfo: 标签信息信息
     * @param [char**] pchOutData: 输出的数据指针
     * @param [int&] nOutDataSize: 输出数据的大小
     * @param [std::string] strFontPath: 使用自定义字库，字库路径
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note pchOutData :new出来的，需要调用者释放
     */
    BlError_E draw(DrawParam_S            stParam,
                   std::list<BoxInfo_S>   listBoxInfo,
                   std::list<LabelInfo_S> listLabelInfo,
                   std::list<LineInfo_S>  listLineInfo,
                   char**                 pchOutData,
                   int&                   nOutDataSize,
                   std::string            strFontPath);

    /**
     * @brief 创建一个透明的图片，画画
     * @param [DrawParam_S] stParam: 参数
     * @param [std::list<BoxInfo_S>] listBoxInfo: 画框信息
     * @param [std::list<LabelInfo_S>] listLabelInfo: 标签信息信息
     * @param [std::list<LineInfo_S>] listLineInfo: 线信息信息
     * @param [std::list<CircleInfo_S>] listCircleInfo: 圆信息信息
     * @param [char**] pchOutData: 输出的数据指针
     * @param [int&] nOutDataSize: 输出数据的大小
     * @param [std::string] strFontPath: 使用自定义字库，字库路径
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note pchOutData :new出来的，需要调用者释放
     */
    BlError_E draw(DrawParam_S             stParam,
                   std::list<BoxInfo_S>    listBoxInfo,
                   std::list<LabelInfo_S>  listLabelInfo,
                   std::list<LineInfo_S>   listLineInfo,
                   std::list<CircleInfo_S> listCircleInfo,
                   char**                  pchOutData,
                   int&                    nOutDataSize,
                   std::string             strFontPath);


    /**
     * @brief 将二进值数据保存成图片
     * @param [DataFormatType_E] enDataType: 二进值数据类型
     * @param [char*] pchData: 二进值数据
     * @param [int] nWidth: 图片数据宽度
     * @param [int] nHeigh: 图片数据高度
     * @param [string&] strFilePath: 图片文件路径
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note
     */
    BlError_E saveImage(DataFormatType_E   enDataType,
                        const char*        pchData,
                        int                nWidth,
                        int                nHeigh,
                        const std::string& strFilePath);
    /**
     * @brief 将二进值数据保存成图片
     * @param [DataFormatType_E] enDataType: 二进值数据类型
     * @param [char*] pchData: 二进值数据
     * @param [int] nWidth: 图片数据宽度
     * @param [int] nHeigh: 图片数据高度
     * @param [string&] strFilePath: 图片文件路径
     * @param [FileFormatType_E] enFileType: 图片文件类型
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note
     */
    BlError_E saveImage(DataFormatType_E   enDataType,
                        const char*        pchData,
                        int                nWidth,
                        int                nHeigh,
                        const std::string& strFilePath,
                        FileFormatType_E   enFileType);


    /**
     * @brief 将二进值数据保存成文件
     * @param [char*] pchData: 二进值数据
     * @param [size_t] nSize: 二进值数据大小
     * @param [char*] pchFilePath: 文件路径
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note
     */
    BlError_E saveBinaryDataToFile(const char* pchData, size_t nSize, const char* pchFilePath);

    /**
     * @brief 转换图片数据
     * @param [DataFormatType_E] enInType: 需要转换的数据类型
     * @param [int] nInWidth: 需要转换的宽
     * @param [int] nInHeight: 需要转换的高
     * @param [char*] pchInData: 需要转换的图片数据
     * @param [int] nInDataSize: 需要转换的图片数据大小
     * @param [DataFormatType_E] enOutType: 转换后的数据类型
     * @param [int] nOutWidth: 转换后的宽
     * @param [int] nOutHeight: 转换后的高
     * @param [char**] pchOutData: 转换后的图片数据，需要调用者释放空间
     * @param [int&] nOutDataSize: 转换后的图片数据大小
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note pchOutData :new出来的，需要调用者释放
     */
    BlError_E transition(DataFormatType_E enInType,
                         int              nInWidth,
                         int              nInHeight,
                         const char*      pchInData,
                         int              nInDataSize,
                         DataFormatType_E enOutType,
                         int              nOutWidth,
                         int              nOutHeight,
                         char**           pchOutData,
                         int&             nOutDataSize);

    /**
     * @brief 转换图片数据
     * @param [DataFormatType_E] enInType: 需要转换的数据类型
     * @param [int] nInWidth: 需要转换的宽
     * @param [int] nInHeight: 需要转换的高
     * @param [char*] pchInData: 需要转换的图片数据
     * @param [int] nInDataSize: 需要转换的图片数据大小
     * @param [DataFormatType_E] enOutType: 转换后的数据类型
     * @param [int] nOutWidth: 转换后的宽
     * @param [int] nOutHeight: 转换后的高
     * @param [cv::Mat&] outImage: 转换后的图片
     * @param [int&] nOutDataSize: 转换后的图片数据大小
     * @return [*] 成功 >= BlError_E::OK   其他失败
     * @note
     */
    BlError_E transition(DataFormatType_E enInType,
                         int              nInWidth,
                         int              nInHeight,
                         const char*      pchInData,
                         int              nInDataSize,
                         DataFormatType_E enOutType,
                         int              nOutWidth,
                         int              nOutHeight,
                         cv::Mat&         outImage);
};
