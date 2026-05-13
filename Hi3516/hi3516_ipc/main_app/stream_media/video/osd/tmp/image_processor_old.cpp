// /*
//  * @FilePath     : image_processor.cpp
//  * @Author       : yanzeh yanzeh@kfb.cn
//  * @Date         : 2024-01-12 10:16:42
//  * @LastEditors  : 严泽辉 yanzeh@kfb.cn
//  * @LastEditTime : 2024-09-27 17:57:23
//  * @Description  : 图片处理器
//  */
// #include "image_processor.hpp"

// #include <fstream>

// #include "cvxFont.h"
// #include "dlog.h"

// /* 在cv::Mat的基础上划线 */
// IpcRet_E CImageProcessor::drawLines(cv::Mat& editImage, std::list<LineInfo_S> listLineInfo)
// {
//     if (editImage.empty())
//     {
//         dlog_error("传入的参数异常");
//         return ERR_PARAM;
//     }

//     for (auto item : listLineInfo)
//     {
//         /* 定义线的起点和终点 */
//         cv::Point startPoint(item.nX1, item.nY1);
//         cv::Point endPoint(item.nX2, item.nY2);

//         /* 定义线的颜色 */
//         cv::Scalar lineColor(item.nR, item.nG, item.nB, item.nA);

//         /* 使用cv::line函数在图像上绘制线段 */
//         cv::line(editImage, startPoint, endPoint, lineColor, item.nThickness);
//     }

//     return OK;
// }

// /* 写字 */
// IpcRet_E CImageProcessor::drawLabel(cv::Mat& editImage, std::list<LabelInfo_S> listLabelInfo)
// {
//     if (editImage.empty())
//     {
//         dlog_error("传入的参数异常");
//         return ERR_PARAM;
//     }

//     /* 画字 */
//     for (auto item : listLabelInfo)
//     {
//         cv::putText(editImage,
//                     item.strLabel,
//                     cv::Point(item.nX, editImage.rows - item.nY - 4),
//                     item.nFontFace,
//                     item.dFontScale,
//                     cv::Scalar(item.nLabelR, item.nLabelG, item.nLabelB, item.nLabelA),
//                     item.nThickness);
//     }

//     return OK;
// }

// /* 写字 */
// IpcRet_E CImageProcessor::drawLabel(
//     cv::Mat&               editImage,
//     std::list<LabelInfo_S> listLabelInfo,
//     std::string            strFontPath)
// {
//     if (editImage.empty())
//     {
//         dlog_error("传入的参数异常");
//         return ERR_PARAM;
//     }

//     std::ifstream file(strFontPath);
//     if (!file.good())
//     {
//         dlog_error("字库路径不存在 [%s]", strFontPath.c_str());
//         return ERR_PARAM;
//     }

//     cvx::CvxFont font(strFontPath);

//     /* 画字 */
//     for (auto item : listLabelInfo)
//     {
//         cvx::putText(editImage,
//                      item.strLabel,
//                      cv::Point(item.nX, editImage.rows - item.nY - 4),
//                      font,
//                      item.nFontSize,
//                      cv::Scalar(item.nLabelR, item.nLabelG, item.nLabelB, item.nLabelA));
//     }

//     return OK;
// }

// /* 在cv::Mat的基础上划圆 */
// IpcRet_E CImageProcessor::drawCenter(
//     cv::Mat&                editImage,
//     std::list<CircleInfo_S> listCircleInfo,
//     std::string             strFontPath)
// {
//     if (editImage.empty())
//     {
//         dlog_error("传入的参数异常");
//         return ERR_PARAM;
//     }

//     std::ifstream file(strFontPath);
//     if (!file.good())
//     {
//         dlog_error("字库路径不存在 [%s]", strFontPath.c_str());
//         return ERR_PARAM;
//     }

//     cvx::CvxFont font(strFontPath);

//     /* 画字 */
//     for (auto item : listCircleInfo)
//     {
//         cv::Point center(item.nCenterX, item.nCenterY);

//         cv::Scalar color(item.nR, item.nG, item.nB, item.nA);

//         /* 绘制圆形 */
//         cv::circle(editImage, center, item.nRadius, color, item.nThickness);
//     }

//     return OK;
// }

// /* 画框 */
// IpcRet_E CImageProcessor::drawBox(cv::Mat& editImage, std::list<BoxInfo_S> listBoxInfo)
// {
//     if (editImage.empty())
//     {
//         dlog_error("传入的参数异常");
//         return ERR_PARAM;
//     }

//     /* 画框 */
//     for (auto item : listBoxInfo)
//     {
//         cv::Rect rect(item.nX,
//                       item.nY,
//                       item.nW,
//                       item.nH);
//         /* 在图片上画框 */
//         cv::rectangle(editImage,
//                       rect,
//                       cv::Scalar(item.nBoxR, item.nBoxG, item.nBoxB, item.nBoxA),
//                       item.nThickness);


//         if (!item.stLabel.strLabel.empty())
//         {
//             /* 画框的标签 */
//             cv::putText(editImage,
//                         item.stLabel.strLabel,
//                         cv::Point(item.stLabel.nX, item.stLabel.nY),
//                         item.stLabel.nFontFace,
//                         item.stLabel.dFontScale,
//                         cv::Scalar(item.stLabel.nLabelR, item.stLabel.nLabelG, item.stLabel.nLabelB, item.stLabel.nLabelA),
//                         item.stLabel.nThickness);
//         }
//     }
//     return OK;
// }

// /* 画框 */
// IpcRet_E CImageProcessor::drawBox(
//     cv::Mat&             editImage,
//     std::list<BoxInfo_S> listBoxInfo,
//     std::string          strFontPath)
// {
//     if (editImage.empty())
//     {
//         dlog_error("传入的参数异常");
//         return ERR_PARAM;
//     }

//     std::ifstream file(strFontPath);
//     if (!file.good())
//     {
//         dlog_error("字库路径不存在 [%s]", strFontPath.c_str());
//         return ERR_PARAM;
//     }

//     cvx::CvxFont font(strFontPath);

//     for (auto item : listBoxInfo)
//     {
//         cv::Rect rect(item.nX,
//                       item.nY,
//                       item.nW,
//                       item.nH);
//         /* 在图片上画框 */
//         cv::rectangle(editImage,
//                       rect,
//                       cv::Scalar(item.nBoxR, item.nBoxG, item.nBoxB, item.nBoxA),
//                       item.nThickness);

//         if (!item.stLabel.strLabel.empty())
//         {
//             // printf("==%d,%d===%d==(%d.%d.%d.%d)=%s\n",
//             //        item.stLabel.nX,
//             //        item.stLabel.nY,
//             //        item.stLabel.nFontSize,
//             //        item.stLabel.nLabelR,
//             //        item.stLabel.nLabelG,
//             //        item.stLabel.nLabelB,
//             //        item.stLabel.nLabelA,
//             //        item.stLabel.strLabel.c_str());
//             /* 画框的标签 */
//             cvx::putText(editImage,
//                          item.stLabel.strLabel,
//                          cv::Point(item.stLabel.nX, item.stLabel.nY),
//                          font,
//                          item.stLabel.nFontSize,
//                          cv::Scalar(item.stLabel.nLabelR, item.stLabel.nLabelG, item.stLabel.nLabelB, item.stLabel.nLabelA));
//         }
//     }

//     return OK;
// }

// /* 创建一个透明的图片，画框 */
// IpcRet_E CImageProcessor::draw(
//     DrawParam_S          stParam,
//     std::list<BoxInfo_S> listBoxInfo,
//     char**               pchOutData,
//     int&                 nOutDataSize)
// {
//     if (nullptr == pchOutData)
//     {
//         dlog_error("传入参数为空");
//         return ERR_PARAM;
//     }

//     IpcRet_E enRetCode = OK;

//     cv::Mat img(stParam.nOutHeight, stParam.nOutWidth, CV_8UC4, cv::Scalar(stParam.nB, stParam.nG, stParam.nR, stParam.nA));

//     /* 画框 */
//     drawBox(img, listBoxInfo);

//     /* 使用cv::cvtColor函数将透明图像转换格式 */
//     cv::Mat imgOut;
//     switch (stParam.enOutType)
//     {
//         case RGB888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2RGB);
//             break;
//         }
//         case BGR888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2BGR);
//             break;
//         }
//         case RGBA8888:
//         {
//             imgOut = img;
//             break;
//         }
//         case ARGB4444:  // 新增分支
//         {
//             // 转换到ARGB4444格式
//             nOutDataSize = img.rows * img.cols * 2; // 每个像素2字节
//             *pchOutData = new char[nOutDataSize];
//             convertToARGB4444(img, *pchOutData);
//             break;
//         }
//         default:
//         {
//             dlog_error("输出图片格式设置异常，未定义该类型[%d]", stParam.enOutType);
//             enRetCode = ERR;
//             break;
//         }
//     }

//     if (enRetCode <= OK)
//     {
//         nOutDataSize = imgOut.total() * imgOut.elemSize();
//         *pchOutData  = new char[nOutDataSize];
//         memcpy(*pchOutData, imgOut.data, nOutDataSize);
//     }

//     return enRetCode;
// }

// /* 创建一个透明的图片，画框 */
// IpcRet_E CImageProcessor::draw(
//     DrawParam_S          stParam,
//     std::list<BoxInfo_S> listBoxInfo,
//     char**               pchOutData,
//     int&                 nOutDataSize,
//     std::string          strFontPath)
// {
//     if (nullptr == pchOutData)
//     {
//         dlog_error("传入参数为空");
//         return ERR_PARAM;
//     }

//     IpcRet_E enRetCode = OK;

//     cv::Mat img(stParam.nOutHeight, stParam.nOutWidth, CV_8UC4, cv::Scalar(stParam.nB, stParam.nG, stParam.nR, stParam.nA));

//     /* 画框 */
//     drawBox(img, listBoxInfo, strFontPath);

//     /* 使用cv::cvtColor函数将透明图像转换格式 */
//     cv::Mat imgOut;
//     switch (stParam.enOutType)
//     {
//         case RGB888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2RGB);
//             break;
//         }
//         case BGR888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2BGR);
//             break;
//         }
//         case RGBA8888:
//         {
//             imgOut = img;
//             break;
//         }
//         case ARGB4444:  // 新增分支
//         {
//             // 转换到ARGB4444格式
//             nOutDataSize = img.rows * img.cols * 2; // 每个像素2字节
//             *pchOutData = new char[nOutDataSize];
//             convertToARGB4444(img, *pchOutData);
//             break;
//         }
//         default:
//         {
//             dlog_error("输出图片格式设置异常，未定义该类型[%d]", stParam.enOutType);
//             enRetCode = ERR;
//             break;
//         }
//     }

//     if (enRetCode <= OK)
//     {
//         nOutDataSize = imgOut.total() * imgOut.elemSize();
//         *pchOutData  = new char[nOutDataSize];
//         memcpy(*pchOutData, imgOut.data, nOutDataSize);
//     }

//     return enRetCode;
// }

// /* 创建一个透明的图片，写标签 */
// IpcRet_E CImageProcessor::draw(
//     DrawParam_S            stParam,
//     std::list<LabelInfo_S> listLabelInfo,
//     char**                 pchOutData,
//     int&                   nOutDataSize)
// {
//     if (nullptr == pchOutData)
//     {
//         dlog_error("传入参数为空");
//         return ERR_PARAM;
//     }

//     IpcRet_E enRetCode = OK;

//     cv::Mat img(stParam.nOutHeight, stParam.nOutWidth, CV_8UC4, cv::Scalar(stParam.nB, stParam.nG, stParam.nR, stParam.nA));

//     /* 画字 */
//     if (!listLabelInfo.empty())
//     {
//         drawLabel(img, listLabelInfo);
//     }

//     /* 使用cv::cvtColor函数将透明图像转换格式 */
//     cv::Mat imgOut;
//     switch (stParam.enOutType)
//     {
//         case RGB888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2RGB);
//             break;
//         }
//         case BGR888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2BGR);
//             break;
//         }
//         case RGBA8888:
//         {
//             imgOut = img;
//             break;
//         }
//         case ARGB4444:
//         {
//             nOutDataSize = img.rows * img.cols * 2; /* 每个像素2字节*/
//             *pchOutData = new char[nOutDataSize];
//             convertToARGB4444(img, *pchOutData);
//             return enRetCode;
//         }
//         default:
//         {
//             dlog_error("输出图片格式设置异常，未定义该类型[%d]", stParam.enOutType);
//             enRetCode = ERR;
//             break;
//         }
//     }

//     if (enRetCode <= OK)
//     {
//         nOutDataSize = imgOut.total() * imgOut.elemSize();
//         *pchOutData  = new char[nOutDataSize];
//         memcpy(*pchOutData, imgOut.data, nOutDataSize);
//     }

//     return enRetCode;
// }

// /* 创建一个透明的图片，写标签 */
// IpcRet_E CImageProcessor::draw(
//     DrawParam_S            stParam,
//     std::list<LabelInfo_S> listLabelInfo,
//     char**                 pchOutData,
//     int&                   nOutDataSize,
//     std::string            strFontPath)
// {
//     if (nullptr == pchOutData)
//     {
//         dlog_error("传入参数为空");
//         return ERR_PARAM;
//     }

//     IpcRet_E enRetCode = OK;

//     cv::Mat img(stParam.nOutHeight, stParam.nOutWidth, CV_8UC4, cv::Scalar(stParam.nB, stParam.nG, stParam.nR, stParam.nA));

//     /* 画字 */
//     if (!listLabelInfo.empty())
//     {
//         drawLabel(img, listLabelInfo, strFontPath);
//     }

//     /* 使用cv::cvtColor函数将透明图像转换格式 */
//     cv::Mat imgOut;
//     switch (stParam.enOutType)
//     {
//         case RGB888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2RGB);
//             break;
//         }
//         case BGR888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2BGR);
//             break;
//         }
//         case RGBA8888:
//         {
//             imgOut = img;
//             break;
//         }
//         case ARGB4444:
//         {
//             nOutDataSize = img.rows * img.cols * 2; /* 每个像素2字节*/
//             *pchOutData = new char[nOutDataSize];
//             convertToARGB4444(img, *pchOutData);
//             return enRetCode;
//         }
//         default:
//         {
//             dlog_error("输出图片格式设置异常，未定义该类型[%d]", stParam.enOutType);
//             enRetCode = ERR;
//             break;
//         }
//     }

//     if (enRetCode <= OK)
//     {
//         nOutDataSize = imgOut.total() * imgOut.elemSize();
//         *pchOutData  = new char[nOutDataSize];
//         memcpy(*pchOutData, imgOut.data, nOutDataSize);
//     }

//     return enRetCode;
// }

// /* 创建一个透明的图片，画框和写标签 */
// IpcRet_E CImageProcessor::draw(
//     DrawParam_S            stParam,
//     std::list<BoxInfo_S>   listBoxInfo,
//     std::list<LabelInfo_S> listLabelInfo,
//     char**                 pchOutData,
//     int&                   nOutDataSize)
// {
//     if (nullptr == pchOutData)
//     {
//         dlog_error("传入参数为空");
//         return ERR_PARAM;
//     }

//     IpcRet_E enRetCode = OK;

//     cv::Mat img(stParam.nOutHeight, stParam.nOutWidth, CV_8UC4, cv::Scalar(stParam.nB, stParam.nG, stParam.nR, stParam.nA));

//     /* 画框 */
//     if (!listBoxInfo.empty())
//     {
//         drawBox(img, listBoxInfo);
//     }

//     /* 画字 */
//     if (!listLabelInfo.empty())
//     {
//         drawLabel(img, listLabelInfo);
//     }

//     /* 使用cv::cvtColor函数将透明图像转换格式 */
//     cv::Mat imgOut;
//     switch (stParam.enOutType)
//     {
//         case RGB888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2RGB);
//             break;
//         }
//         case BGR888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2BGR);
//             break;
//         }
//         case RGBA8888:
//         {
//             imgOut = img;
//             break;
//         }
//         case ARGB4444:  // 新增分支
//         {
//             // 转换到ARGB4444格式
//             nOutDataSize = img.rows * img.cols * 2; // 每个像素2字节
//             *pchOutData = new char[nOutDataSize];
//             convertToARGB4444(img, *pchOutData);
//             break;
//         }
//         default:
//         {
//             dlog_error("输出图片格式设置异常，未定义该类型[%d]", stParam.enOutType);
//             enRetCode = ERR;
//             break;
//         }
//     }

//     if (enRetCode <= OK)
//     {
//         nOutDataSize = imgOut.total() * imgOut.elemSize();
//         *pchOutData  = new char[nOutDataSize];
//         memcpy(*pchOutData, imgOut.data, nOutDataSize);
//     }

//     return enRetCode;
// }

// /* 创建一个透明的图片，画框和写标签 */
// IpcRet_E CImageProcessor::draw(
//     DrawParam_S            stParam,
//     std::list<BoxInfo_S>   listBoxInfo,
//     std::list<LabelInfo_S> listLabelInfo,
//     char**                 pchOutData,
//     int&                   nOutDataSize,
//     std::string            strFontPath)
// {
//     if (nullptr == pchOutData)
//     {
//         dlog_error("传入参数为空");
//         return ERR_PARAM;
//     }


//     IpcRet_E enRetCode = OK;

//     cv::Mat img(stParam.nOutHeight, stParam.nOutWidth, CV_8UC4, cv::Scalar(stParam.nB, stParam.nG, stParam.nR, stParam.nA));

//     /* 画框 */
//     if (!listBoxInfo.empty())
//     {
//         drawBox(img, listBoxInfo, strFontPath);
//     }

//     /* 画字 */
//     if (!listLabelInfo.empty())
//     {
//         drawLabel(img, listLabelInfo, strFontPath);
//     }

//     /* 使用cv::cvtColor函数将透明图像转换格式 */
//     cv::Mat imgOut;
//     switch (stParam.enOutType)
//     {
//         case RGB888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2RGB);
//             break;
//         }
//         case BGR888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2BGR);
//             break;
//         }
//         case RGBA8888:
//         {
//             imgOut = img;
//             break;
//         }
//         case ARGB4444:  // 新增分支
//         {
//             // 转换到ARGB4444格式
//             nOutDataSize = img.rows * img.cols * 2; // 每个像素2字节
//             *pchOutData = new char[nOutDataSize];
//             convertToARGB4444(img, *pchOutData);
//             break;
//         }
//         default:
//         {
//             dlog_error("输出图片格式设置异常，未定义该类型[%d]", stParam.enOutType);
//             enRetCode = ERR;
//             break;
//         }
//     }

//     if (enRetCode <= OK)
//     {
//         nOutDataSize = imgOut.total() * imgOut.elemSize();
//         *pchOutData  = new char[nOutDataSize];
//         memcpy(*pchOutData, imgOut.data, nOutDataSize);
//     }

//     return enRetCode;
// }

// /* 创建一个透明的图片，画框、写标签和画直线 */
// IpcRet_E CImageProcessor::draw(
//     DrawParam_S            stParam,
//     std::list<BoxInfo_S>   listBoxInfo,
//     std::list<LabelInfo_S> listLabelInfo,
//     std::list<LineInfo_S>  listLineInfo,
//     char**                 pchOutData,
//     int&                   nOutDataSize)
// {
//     if (nullptr == pchOutData)
//     {
//         dlog_error("传入参数为空");
//         return ERR_PARAM;
//     }

//     IpcRet_E enRetCode = OK;

//     cv::Mat img(stParam.nOutHeight,
//                 stParam.nOutWidth,
//                 CV_8UC4,
//                 cv::Scalar(stParam.nB, stParam.nG, stParam.nR, stParam.nA));

//     /* 画框 */
//     drawBox(img, listBoxInfo);

//     /* 画字 */
//     drawLabel(img, listLabelInfo);

//     /* 划线 */
//     drawLines(img, listLineInfo);

//     /* 使用cv::cvtColor函数将透明图像转换格式 */
//     cv::Mat imgOut;
//     switch (stParam.enOutType)
//     {
//         case RGB888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2RGB);
//             break;
//         }
//         case BGR888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2BGR);
//             break;
//         }
//         case RGBA8888:
//         {
//             imgOut = img;
//             break;
//         }
//         case ARGB4444:  // 新增分支
//         {
//             // 转换到ARGB4444格式
//             nOutDataSize = img.rows * img.cols * 2; // 每个像素2字节
//             *pchOutData = new char[nOutDataSize];
//             convertToARGB4444(img, *pchOutData);
//             break;
//         }
//         default:
//         {
//             dlog_error("输出图片格式设置异常，未定义该类型[%d]", stParam.enOutType);
//             enRetCode = ERR;
//             break;
//         }
//     }

//     if (enRetCode <= OK)
//     {
//         nOutDataSize = imgOut.total() * imgOut.elemSize();
//         *pchOutData  = new char[nOutDataSize];
//         memcpy(*pchOutData, imgOut.data, nOutDataSize);
//     }

//     return enRetCode;
// }

// /* 创建一个透明的图片，画框、写标签和画直线 */
// IpcRet_E CImageProcessor::draw(
//     DrawParam_S            stParam,
//     std::list<BoxInfo_S>   listBoxInfo,
//     std::list<LabelInfo_S> listLabelInfo,
//     std::list<LineInfo_S>  listLineInfo,
//     char**                 pchOutData,
//     int&                   nOutDataSize,
//     std::string            strFontPath)
// {
//     if (nullptr == pchOutData)
//     {
//         dlog_error("传入参数为空");
//         return ERR_PARAM;
//     }

//     IpcRet_E enRetCode = OK;

//     cv::Mat img(stParam.nOutHeight,
//                 stParam.nOutWidth,
//                 CV_8UC4,
//                 cv::Scalar(stParam.nB, stParam.nG, stParam.nR, stParam.nA));

//     /* 画框 */
//     drawBox(img, listBoxInfo, strFontPath);

//     /* 画字 */
//     drawLabel(img, listLabelInfo, strFontPath);

//     /* 划线 */
//     drawLines(img, listLineInfo);

//     /* 使用cv::cvtColor函数将透明图像转换格式 */
//     cv::Mat imgOut;
//     switch (stParam.enOutType)
//     {
//         case RGB888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2RGB);
//             break;
//         }
//         case BGR888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2BGR);
//             break;
//         }
//         case RGBA8888:
//         {
//             imgOut = img;
//             break;
//         }
//         case ARGB4444:  // 新增分支
//         {
//             // 转换到ARGB4444格式
//             nOutDataSize = img.rows * img.cols * 2; // 每个像素2字节
//             *pchOutData = new char[nOutDataSize];
//             convertToARGB4444(img, *pchOutData);
//             break;
//         }
//         default:
//         {
//             dlog_error("输出图片格式设置异常，未定义该类型[%d]", stParam.enOutType);
//             enRetCode = ERR;
//             break;
//         }
//     }

//     if (enRetCode <= OK)
//     {
//         nOutDataSize = imgOut.total() * imgOut.elemSize();
//         *pchOutData  = new char[nOutDataSize];
//         memcpy(*pchOutData, imgOut.data, nOutDataSize);

//         // cv::imwrite("111.jpg", imgOut);
//     }

//     return enRetCode;
// }

// /* 创建一个透明的图片，画画 */
// IpcRet_E CImageProcessor::draw(
//     DrawParam_S             stParam,
//     std::list<BoxInfo_S>    listBoxInfo,
//     std::list<LabelInfo_S>  listLabelInfo,
//     std::list<LineInfo_S>   listLineInfo,
//     std::list<CircleInfo_S> listCircleInfo,
//     char**                  pchOutData,
//     int&                    nOutDataSize,
//     std::string             strFontPath)
// {
//     if (nullptr == pchOutData)
//     {
//         dlog_error("传入参数为空");
//         return ERR_PARAM;
//     }

//     IpcRet_E enRetCode = OK;

//     cv::Mat img(stParam.nOutHeight,
//                 stParam.nOutWidth,
//                 CV_8UC4,
//                 cv::Scalar(stParam.nB, stParam.nG, stParam.nR, stParam.nA));

//     /* 画框 */
//     drawBox(img, listBoxInfo, strFontPath);

//     /* 画字 */
//     drawLabel(img, listLabelInfo, strFontPath);

//     /* 画线 */
//     drawLines(img, listLineInfo);

//     /* 画圆 */
//     drawCenter(img, listCircleInfo, strFontPath);

//     /* 使用cv::cvtColor函数将透明图像转换格式 */
//     cv::Mat imgOut;
//     switch (stParam.enOutType)
//     {
//         case RGB888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2RGB);
//             break;
//         }
//         case BGR888:
//         {
//             cv::cvtColor(img, imgOut, cv::COLOR_RGBA2BGR);
//             break;
//         }
//         case RGBA8888:
//         {
//             imgOut = img;
//             break;
//         }
//         case ARGB4444:  // 新增分支
//         {
//             // 转换到ARGB4444格式
//             nOutDataSize = img.rows * img.cols * 2; // 每个像素2字节
//             *pchOutData = new char[nOutDataSize];
//             convertToARGB4444(img, *pchOutData);
//             break;
//         }
//         default:
//         {
//             dlog_error("输出图片格式设置异常，未定义该类型[%d]", stParam.enOutType);
//             enRetCode = ERR;
//             break;
//         }
//     }

//     if (enRetCode <= OK)
//     {
//         nOutDataSize = imgOut.total() * imgOut.elemSize();
//         *pchOutData  = new char[nOutDataSize];
//         memcpy(*pchOutData, imgOut.data, nOutDataSize);
//     }

//     return enRetCode;
// }

// /* 将二进值数据保存成图片 */
// IpcRet_E CImageProcessor::saveImage(
//     DataFormatType_E   enDataType,
//     const char*        pchData,
//     int                nWidth,
//     int                nHeigh,
//     const std::string& strFilePath)
// {
//     if (nullptr == pchData || 0 >= nWidth || 0 >= nHeigh)
//     {
//         dlog_error("传入参数为空");
//         return ERR_PARAM;
//     }

//     /* 检查文件路径是否有效 */
//     if (strFilePath.empty())
//     {
//         dlog_error("文件路径为空");
//         return ERR_PARAM;
//     }

//     /* 将二进制数据转换为cv::Mat */
//     cv::Mat imageMat;
//     switch (enDataType)
//     {
//         case RGB888:
//         case BGR888:
//         {
//             imageMat = cv::Mat(nHeigh, nWidth, CV_8UC3, (void*)pchData);
//             break;
//         }
//         case RGBA8888:
//         {
//             imageMat = cv::Mat(nHeigh, nWidth, CV_8UC4, (void*)pchData);
//             break;
//         }
//         // 现有格式...
//         case ARGB4444:
//         {
//             // 转换到RGBA8888以便保存
//             cv::Mat rgba(nHeigh, nWidth, CV_8UC4);
//             convertARGB4444ToRGBA8888(pchData, rgba.data, nWidth, nHeigh);
//             imageMat = rgba;
//             break;
//         }
//         default:
//         {
//             dlog_error("输入数据格式设置异常，未定义该类型[%d]", enDataType);
//             return ERR;
//         }
//     }

//     /* 检查图像是否成功加载 */
//     if (imageMat.empty())
//     {
//         dlog_error("保存图片失败-图像加载失败");
//         return ERR;
//     }

//     /* 保存图像到文件 */
//     try
//     {
//         if (cv::imwrite(strFilePath, imageMat))
//         {
//             dlog_trace("保存图片成功 [%s]", strFilePath.c_str());
//             return OK;
//         }
//         else
//         {
//             dlog_error("保存图片失败 [%s]", strFilePath.c_str());
//             return ERR;
//         }
//     }
//     catch (cv::Exception& e)
//     {
//         /* 获取错误信息 */
//         dlog_error("保存图片失败 [%s]", e.what());
//         return ERR;
//     }

//     return ERR;
// }

// /* 将二进值数据保存成图片 */
// IpcRet_E CImageProcessor::saveImage(
//     DataFormatType_E   enDataType,
//     const char*        pchData,
//     int                nWidth,
//     int                nHeigh,
//     const std::string& strFilePath,
//     FileFormatType_E   enFileType)
// {
//     if (nullptr == pchData || 0 >= nWidth || 0 >= nHeigh)
//     {
//         dlog_error("传入参数为空");
//         return ERR_PARAM;
//     }

//     /* 检查文件路径是否有效 */
//     if (strFilePath.empty())
//     {
//         dlog_error("文件路径为空");
//         return ERR_PARAM;
//     }

//     /* 将二进制数据转换为cv::Mat */
//     cv::Mat imageMat;
//     switch (enDataType)
//     {
//         case RGB888:
//         case BGR888:
//         {
//             imageMat = cv::Mat(nHeigh, nWidth, CV_8UC3, (void*)pchData);
//             break;
//         }
//         case RGBA8888:
//         {
//             imageMat = cv::Mat(nHeigh, nWidth, CV_8UC4, (void*)pchData);
//             break;
//         }
//         // 现有格式...
//         case ARGB4444:
//         {
//             // 转换到RGBA8888以便保存
//             cv::Mat rgba(nHeigh, nWidth, CV_8UC4);
//             convertARGB4444ToRGBA8888(pchData, rgba.data, nWidth, nHeigh);
//             imageMat = rgba;
//             break;
//         }
//         default:
//         {
//             dlog_error("输入数据格式设置异常，未定义该类型[%d]", enDataType);
//             return ERR;
//         }
//     }

//     /* 检查图像是否成功加载 */
//     if (imageMat.empty())
//     {
//         dlog_error("保存图片失败-图像加载失败");
//         return ERR;
//     }

//     std::vector<int> vnParams;
//     switch (enFileType)
//     {
//         case JPEG:
//         {
//             vnParams.push_back(cv::IMWRITE_JPEG_QUALITY);
//             /* 设置JPEG质量，范围0-100 */
//             vnParams.push_back(95);
//             break;
//         }
//         case PNG:
//         {
//             vnParams.push_back(cv::IMWRITE_PNG_COMPRESSION);
//             /* 设置PNG压缩级别，范围0-9 */
//             vnParams.push_back(3);
//             break;
//         }
//         default:
//         {
//             dlog_error("输出文件格式设置异常，未定义该类型[%d]", enDataType);
//             return ERR;
//         }
//     }

//     /* 保存图像到文件 */
//     try
//     {
//         if (cv::imwrite(strFilePath, imageMat, vnParams))
//         {
//             dlog_trace("保存图片成功 [%s]", strFilePath.c_str());
//             return OK;
//         }
//         else
//         {
//             dlog_error("保存图片失败 [%s]", strFilePath.c_str());
//             return ERR;
//         }
//     }
//     catch (cv::Exception& e)
//     {
//         /* 获取错误信息 */
//         dlog_error("保存图片失败 [%s]", e.what());
//         return ERR;
//     }

//     return ERR;
// }

// /* 将二进值数据保存成文件 */
// IpcRet_E CImageProcessor::saveBinaryDataToFile(
//     const char* pchData,
//     size_t      nSize,
//     const char* pchFilePath)
// {
//     /* 打开文件以写入二进制数据 */
//     FILE* pFile = fopen(pchFilePath, "wb");

//     if (pFile == NULL)
//     {
//         dlog_error("打开文件失败 [%s]", pchFilePath);
//         return ERR;
//     }

//     /* 写入二进制数据到文件 */
//     size_t nWritten = fwrite(pchData, 1, nSize, pFile);

//     /* 关闭文件 */
//     fclose(pFile);

//     if (nWritten != nSize)
//     {
//         dlog_error("写入文件失败 [%s]", pchFilePath);
//         return ERR;
//     }

//     return OK;
// }

// /* 转换图片数据 */
// IpcRet_E CImageProcessor::transition(
//     DataFormatType_E enInType,
//     int              nInWidth,
//     int              nInHeight,
//     const char*      pchInData,
//     int              nInDataSize,
//     DataFormatType_E enOutType,
//     int              nOutWidth,
//     int              nOutHeight,
//     char**           pchOutData,
//     int&             nOutDataSize)
// {
//     if (nullptr == pchInData || nullptr == pchOutData || nInDataSize <= 0)
//     {
//         dlog_error("传入参数为空");
//         return ERR_PARAM;
//     }

//     cv::Mat inImage;
//     cv::Mat outImage;

//     switch (enInType)
//     {
//         case RGB888:
//         case BGR888:
//         {
//             inImage = cv::Mat(nInHeight, nInWidth, CV_8UC3, (void*)pchInData);
//             break;
//         }
//         case RGBA8888:
//         {
//             inImage = cv::Mat(nInHeight, nInWidth, CV_8UC4, (void*)pchInData);
//             break;
//         }
//         default:
//         {
//             dlog_error("输入数据格式设置异常，未定义该类型[%d]", enInType);
//             return ERR;
//         }
//     }


//     switch (enInType)
//     {
//         case RGB888:
//         {
//             switch (enOutType)
//             {
//                 case RGB888:
//                 {
//                     outImage = inImage;
//                     break;
//                 }
//                 case BGR888:
//                 {
//                     cv::cvtColor(inImage, outImage, cv::COLOR_RGB2BGR);
//                     break;
//                 }
//                 case RGBA8888:
//                 {
//                     cv::cvtColor(inImage, outImage, cv::COLOR_RGB2RGBA);
//                     break;
//                 }
//                 // 现有格式...
//                 case ARGB4444:
//                 {
//                     // 确保输入是RGBA8888
//                     if (inImage.type() != CV_8UC4) {
//                         cv::Mat tmp;
//                         cv::cvtColor(inImage, tmp, cv::COLOR_BGR2RGBA);
//                         inImage = tmp;
//                     }
                    
//                     // 调整尺寸
//                     if (nInWidth != nOutWidth || nInHeight != nOutHeight) {
//                         cv::resize(inImage, outImage, cv::Size(nOutWidth, nOutHeight));
//                     } else {
//                         outImage = inImage;
//                     }
//                     break;
//                 }
//                 default:
//                 {
//                     dlog_error("输入数据格式设置异常，未定义该类型[%d]", enInType);
//                     return ERR;
//                 }
//             }
//             break;
//         }
//         case BGR888:
//         {
//             switch (enOutType)
//             {
//                 case RGB888:
//                 {
//                     cv::cvtColor(inImage, outImage, cv::COLOR_BGR2RGB);
//                     break;
//                 }
//                 case BGR888:
//                 {
//                     outImage = inImage;
//                     break;
//                 }
//                 case RGBA8888:
//                 {
//                     cv::cvtColor(inImage, outImage, cv::COLOR_BGR2RGBA);
//                     break;
//                 }
//                 // 现有格式...
//                 case ARGB4444:
//                 {
//                     // 确保输入是RGBA8888
//                     if (inImage.type() != CV_8UC4) {
//                         cv::Mat tmp;
//                         cv::cvtColor(inImage, tmp, cv::COLOR_BGR2RGBA);
//                         inImage = tmp;
//                     }
                    
//                     // 调整尺寸
//                     if (nInWidth != nOutWidth || nInHeight != nOutHeight) {
//                         cv::resize(inImage, outImage, cv::Size(nOutWidth, nOutHeight));
//                     } else {
//                         outImage = inImage;
//                     }
//                     break;
//                 }
//                 default:
//                 {
//                     dlog_error("输入数据格式设置异常，未定义该类型[%d]", enInType);
//                     return ERR;
//                 }
//             }
//             break;
//         }
//         case RGBA8888:
//         {
//             switch (enOutType)
//             {
//                 case RGB888:
//                 {
//                     cv::cvtColor(inImage, outImage, cv::COLOR_RGBA2RGB);
//                     break;
//                 }
//                 case BGR888:
//                 {
//                     cv::cvtColor(inImage, outImage, cv::COLOR_RGBA2BGR);
//                     break;
//                 }
//                 case RGBA8888:
//                 {
//                     outImage = inImage;
//                     break;
//                 }
//                 // 现有格式...
//                 case ARGB4444:
//                 {
//                     // 确保输入是RGBA8888
//                     if (inImage.type() != CV_8UC4) {
//                         cv::Mat tmp;
//                         cv::cvtColor(inImage, tmp, cv::COLOR_BGR2RGBA);
//                         inImage = tmp;
//                     }
                    
//                     // 调整尺寸
//                     if (nInWidth != nOutWidth || nInHeight != nOutHeight) {
//                         cv::resize(inImage, outImage, cv::Size(nOutWidth, nOutHeight));
//                     } else {
//                         outImage = inImage;
//                     }
//                     break;
//                 }
//                 default:
//                 {
//                     dlog_error("输入数据格式设置异常，未定义该类型[%d]", enInType);
//                     return ERR;
//                 }
//             }
//             break;
//         }
//         default:
//         {
//             dlog_error("输入数据格式设置异常，未定义该类型[%d]", enInType);
//             return ERR;
//         }
//     }

//     cv::Mat scaledImage;
//     if (nInWidth != nOutWidth || nInHeight != nOutHeight)
//     {
//         cv::resize(outImage, scaledImage, cv::Size(nOutWidth, nOutHeight));
//     }
//     else
//     {
//         scaledImage = outImage;
//     }


//     nOutDataSize = scaledImage.total() * scaledImage.elemSize();
//     *pchOutData  = new char[nOutDataSize];
//     memcpy(*pchOutData, scaledImage.data, nOutDataSize);

//     return OK;
// }

// /* 转换图片数据 */
// IpcRet_E CImageProcessor::transition(
//     DataFormatType_E enInType,
//     int              nInWidth,
//     int              nInHeight,
//     const char*      pchInData,
//     int              nInDataSize,
//     DataFormatType_E enOutType,
//     int              nOutWidth,
//     int              nOutHeight,
//     cv::Mat&         outImage)
// {
//     if (nullptr == pchInData || nInDataSize <= 0)
//     {
//         dlog_error("传入参数为空");
//         return ERR_PARAM;
//     }

//     cv::Mat inImage;
//     cv::Mat outTmpImage;

//     switch (enInType)
//     {
//         case RGB888:
//         case BGR888:
//         {
//             inImage = cv::Mat(nInHeight, nInWidth, CV_8UC3, (void*)pchInData);
//             break;
//         }
//         case RGBA8888:
//         {
//             inImage = cv::Mat(nInHeight, nInWidth, CV_8UC4, (void*)pchInData);
//             break;
//         }
//         default:
//         {
//             dlog_error("输入数据格式设置异常，未定义该类型[%d]", enInType);
//             return ERR;
//         }
//     }


//     switch (enInType)
//     {
//         case RGB888:
//         {
//             switch (enOutType)
//             {
//                 case RGB888:
//                 {
//                     outTmpImage = inImage;
//                     break;
//                 }
//                 case BGR888:
//                 {
//                     cv::cvtColor(inImage, outTmpImage, cv::COLOR_RGB2BGR);
//                     break;
//                 }
//                 case RGBA8888:
//                 {
//                     cv::cvtColor(inImage, outTmpImage, cv::COLOR_RGB2RGBA);
//                     break;
//                 }
//                 // 现有格式...
//                 case ARGB4444:
//                 {
//                     // 确保输入是RGBA8888
//                     if (inImage.type() != CV_8UC4) {
//                         cv::Mat tmp;
//                         cv::cvtColor(inImage, tmp, cv::COLOR_BGR2RGBA);
//                         inImage = tmp;
//                     }
                    
//                     // 调整尺寸
//                     if (nInWidth != nOutWidth || nInHeight != nOutHeight) {
//                         cv::resize(inImage, outImage, cv::Size(nOutWidth, nOutHeight));
//                     } else {
//                         outImage = inImage;
//                     }
//                     break;
//                 }
//                 default:
//                 {
//                     dlog_error("输入数据格式设置异常，未定义该类型[%d]", enInType);
//                     return ERR;
//                 }
//             }
//             break;
//         }
//         case BGR888:
//         {
//             switch (enOutType)
//             {
//                 case RGB888:
//                 {
//                     cv::cvtColor(inImage, outTmpImage, cv::COLOR_BGR2RGB);
//                     break;
//                 }
//                 case BGR888:
//                 {
//                     outTmpImage = inImage;
//                     break;
//                 }
//                 case RGBA8888:
//                 {
//                     cv::cvtColor(inImage, outTmpImage, cv::COLOR_BGR2RGBA);
//                     break;
//                 }
//                 // 现有格式...
//                 case ARGB4444:
//                 {
//                     // 确保输入是RGBA8888
//                     if (inImage.type() != CV_8UC4) {
//                         cv::Mat tmp;
//                         cv::cvtColor(inImage, tmp, cv::COLOR_BGR2RGBA);
//                         inImage = tmp;
//                     }
                    
//                     // 调整尺寸
//                     if (nInWidth != nOutWidth || nInHeight != nOutHeight) {
//                         cv::resize(inImage, outImage, cv::Size(nOutWidth, nOutHeight));
//                     } else {
//                         outImage = inImage;
//                     }
//                     break;
//                 }
//                 default:
//                 {
//                     dlog_error("输入数据格式设置异常，未定义该类型[%d]", enInType);
//                     return ERR;
//                 }
//             }
//             break;
//         }
//         case RGBA8888:
//         {
//             switch (enOutType)
//             {
//                 case RGB888:
//                 {
//                     cv::cvtColor(inImage, outTmpImage, cv::COLOR_RGBA2RGB);
//                     break;
//                 }
//                 case BGR888:
//                 {
//                     cv::cvtColor(inImage, outTmpImage, cv::COLOR_RGBA2BGR);
//                     break;
//                 }
//                 case RGBA8888:
//                 {
//                     outTmpImage = inImage;
//                     break;
//                 }
//                 // 现有格式...
//                 case ARGB4444:
//                 {
//                     // 确保输入是RGBA8888
//                     if (inImage.type() != CV_8UC4) {
//                         cv::Mat tmp;
//                         cv::cvtColor(inImage, tmp, cv::COLOR_BGR2RGBA);
//                         inImage = tmp;
//                     }
                    
//                     // 调整尺寸
//                     if (nInWidth != nOutWidth || nInHeight != nOutHeight) {
//                         cv::resize(inImage, outImage, cv::Size(nOutWidth, nOutHeight));
//                     } else {
//                         outImage = inImage;
//                     }
//                     break;
//                 }
//                 default:
//                 {
//                     dlog_error("输入数据格式设置异常，未定义该类型[%d]", enInType);
//                     return ERR;
//                 }
//             }
//             break;
//         }
//         default:
//         {
//             dlog_error("输入数据格式设置异常，未定义该类型[%d]", enInType);
//             return ERR;
//         }
//     }

//     if (nInWidth != nOutWidth || nInHeight != nOutHeight)
//     {
//         cv::resize(outTmpImage, outImage, cv::Size(nOutWidth, nOutHeight));
//     }
//     else
//     {
//         outImage = outTmpImage;
//     }

//     return OK;
// }

// void CImageProcessor::convertToARGB4444(const cv::Mat& rgbaImg, char* outData)
// {
//     // 确保输入是RGBA8888格式
//     if (rgbaImg.type() != CV_8UC4) {
//         dlog_error("Input must be RGBA8888 format");
//         return;
//     }

//     const int width = rgbaImg.cols;
//     const int height = rgbaImg.rows;
//     const int alignedWidth = (width + 7) & ~7; // 8像素对齐 (16字节对齐)
//     uint16_t* argbData = reinterpret_cast<uint16_t*>(outData);

//     for (int y = 0; y < height; ++y) {
//         for (int x = 0; x < width; ++x) {
//             const cv::Vec4b& pixel = rgbaImg.at<cv::Vec4b>(y, x);
            
//             // 提取并转换通道 (OpenCV顺序: B,G,R,A → ARGB顺序)
//             const uint8_t a = pixel[3] >> 4;  // Alpha 4-bit
//             const uint8_t r = pixel[2] >> 4;  // Red 4-bit
//             const uint8_t g = pixel[1] >> 4;  // Green 4-bit
//             const uint8_t b = pixel[0] >> 4;  // Blue 4-bit
            
//             // 组合为ARGB4444格式 (A<<12 | R<<8 | G<<4 | B)
//             argbData[y * alignedWidth + x] = 
//                 (static_cast<uint16_t>(a) << 12) |
//                 (static_cast<uint16_t>(r) << 8) |
//                 (static_cast<uint16_t>(g) << 4) |
//                 static_cast<uint16_t>(b);
//         }
        
//         // 对齐填充 (每行末尾填充0)
//         if (width < alignedWidth) {
//             memset(argbData + y * alignedWidth + width, 
//                    0, 
//                    (alignedWidth - width) * sizeof(uint16_t));
//         }
//     }
// }

// void CImageProcessor::convertARGB4444ToRGBA8888(const char* argb4444, uchar* rgba, int width, int height)
// {
//     const uint16_t* src = reinterpret_cast<const uint16_t*>(argb4444);
//     const int alignedWidth = (width + 7) & ~7; // 8像素对齐
    
//     for (int y = 0; y < height; ++y) {
//         for (int x = 0; x < width; ++x) {
//             const uint16_t pixel = src[y * alignedWidth + x];
            
//             // 提取ARGB4444通道
//             const uchar a = ((pixel >> 12) & 0xF) * 17; // 0-15 → 0-255
//             const uchar r = ((pixel >> 8)  & 0xF) * 17;
//             const uchar g = ((pixel >> 4)  & 0xF) * 17;
//             const uchar b = (pixel        & 0xF) * 17;
            
//             // 存储为RGBA8888 (OpenCV顺序: B,G,R,A)
//             uchar* dst = &rgba[(y * width + x) * 4];
//             dst[0] = b;
//             dst[1] = g;
//             dst[2] = r;
//             dst[3] = a;
//         }
//     }
// }