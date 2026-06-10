/**
 * @FilePath     : face_capture_processor.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-04-28 15:07:08
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-28 16:18:40
 * @Description  : 人脸抓拍处理器
 */

 #pragma once

 #include <string>
 #include <vector>

 #include "algorithm.hpp"
 #include "face_capture_types.hpp"
 #include "face_detect_context.hpp"
 #include "face_capture_sdk_event_publisher.hpp"

 namespace FaceDetectInternal
 {
 class CFaceCaptureProcessor
 {
 public:
     /**
      * @brief   : 设置人脸抓拍使能状态
      * @param    {bool} bEnable：true：使能 false：关闭
      * @return   {void}
      */
     void setEnabled(bool bEnable);

     /**
      * @brief   : 设置人脸抓拍参数
      * @param    {Alarm::FaceCapture_S} &stAlgoCfg：人脸抓拍配置
      * @param    {int} nWidth：算法分辨率宽度
      * @param    {int} nHeight：算法分辨率高度
      * @return   {void}
      */
     void setAlgoParamCfg(const Alarm::FaceCapture_S &stAlgoCfg, int nWidth, int nHeight);

     /**
      * @brief   : 处理单帧人脸抓拍结果
      * @param    {SFaceProcessContext} &stContext：单帧处理上下文
      * @param    {std::vector<std::string>} &vecImageFile：邮件附件图片路径列表
      * @return   {void}
      */
     void process(SFaceProcessContext &stContext, std::vector<std::string> &vecImageFile);

     /**
      * @brief   : 收集满足人脸抓拍规则的目标
      * @param    {std::vector<Inference_NS::PointData_S>} &vPointDatas：人脸检测结果
      * @param    {std::vector<Common::RectInfo_S>} &vstRectInfo：输出 OSD 目标框列表
      * @param    {std::vector<FaceCaptureTarget_S>} *pvecTargets：输出抓拍目标列表
      * @return   {bool} true：存在满足规则的人脸 false：不存在满足规则的人脸
      */
     bool collectTargets(std::vector<Inference_NS::PointData_S> &vPointDatas,
                         std::vector<Common::RectInfo_S> &vstRectInfo,
                         std::vector<FaceCaptureTarget_S> *pvecTargets = nullptr);

     /**
      * @brief   : 保存人脸目标小图
      * @param    {std::vector<Common::RectInfo_S>} vstRectInfo：人脸目标框数组
      * @param    {ot_video_frame_info} *pSrcFrameInfo：当前检测帧
      * @param    {int} nChnId：通道号
      * @param    {std::vector<std::string>} &vecImageFile：输出图片路径列表
      * @return   {int} 0：成功，非0：失败
      */
     int saveFaceImage(std::vector<Common::RectInfo_S> vstRectInfo,
                       ot_video_frame_info *pSrcFrameInfo,
                       int nChnId,
                       std::vector<std::string> &vecImageFile,
                       long long llTimestamp = 0,
                       bool bSaveDatabase = true);

     /**
      * @brief   : 获取当前是否使能
      * @return   {bool} true：使能 false：关闭
      */
     bool isEnabled() const;
     int encodeFaceTargetImageToFile(const Common::RectInfo_S &stRectInfo,
        ot_video_frame_info *pSrcFrameInfo,
        const std::string &strFilename);
 private:
     /**
      * @brief   : 构造人脸抓拍联动配置快照
      * @return   {FaceCaptureLinkageOptions_S} 人脸抓拍联动配置
      */
     FaceCaptureLinkageOptions_S buildLinkageOptions() const;

     /**
      * @brief   : 处理人脸抓拍事件联动
      * @param    {bool} bAlarm：当前帧是否触发人脸抓拍报警
      * @param    {std::vector<FaceCaptureTarget_S>} &vecTargets：人脸抓拍目标列表
      * @param    {std::vector<Common::RectInfo_S>} &vstRectInfo：人脸目标框列表
      * @param    {ot_video_frame_info} *pFrameInfo：当前检测帧
      * @param    {int} nChnId：通道号
      * @param    {std::vector<std::string>} &vecImageFile：邮件附件图片路径列表
      * @return   {void}
      */
     void handleLinkage(bool bAlarm,
                        const std::vector<FaceCaptureTarget_S> &vecTargets,
                        const std::vector<Common::RectInfo_S> &vstRectInfo,
                        ot_video_frame_info *pFrameInfo,
                        int nChnId,
                        long long llTimestamp,
                        std::vector<std::string> &vecImageFile);

     /**
      * @brief   : 构建人脸抓拍 SDK 全景图
      * @param    {ot_video_frame_info} *pFrameInfo：当前检测帧
      * @param    {std::vector<unsigned char>} &vecJpeg：输出 JPEG 二进制图片
      * @return   {bool} true：构建成功 false：构建失败
      */
     bool buildSdkPanoramaImage(ot_video_frame_info *pFrameInfo,
                                std::vector<unsigned char> &vecJpeg);

     /**
      * @brief   : 构建人脸抓拍 SDK 单目标小图
      * @param    {Common::RectInfo_S} &stRectInfo：人脸目标框
      * @param    {ot_video_frame_info} *pFrameInfo：当前检测帧
      * @param    {size_t} nIndex：批次内目标序号
      * @param    {std::vector<unsigned char>} &vecJpeg：输出 JPEG 二进制图片
      * @return   {bool} true：构建成功 false：构建失败
      */
     bool buildSdkTargetImage(const Common::RectInfo_S &stRectInfo,
                              ot_video_frame_info *pFrameInfo,
                              size_t nIndex,
                              std::vector<unsigned char> &vecJpeg);

     /**
      * @brief   : 编码单张人脸目标小图到指定文件
      * @param    {Common::RectInfo_S} &stRectInfo：人脸目标框
      * @param    {ot_video_frame_info} *pSrcFrameInfo：当前检测帧
      * @param    {std::string} &strFilename：输出 JPEG 文件路径
      * @return   {int} 0：成功，非0：失败
      */


     /**
      * @brief   : 读取 JPEG 文件为内存数据
      * @param    {std::string} &strFilename：JPEG 文件路径
      * @param    {std::vector<unsigned char>} &vecJpeg：输出 JPEG 二进制图片
      * @return   {bool} true：读取成功 false：读取失败
      */
     bool loadJpegFile(const std::string &strFilename, std::vector<unsigned char> &vecJpeg) const;

     /**
      * @brief   : 构造人脸抓拍 SDK 临时图片路径
      * @param    {std::string} &strImageType：图片类型标识
      * @param    {size_t} nIndex：批次内图片序号
      * @return   {std::string} 临时图片路径
      */
     std::string buildTempFilePath(const std::string &strImageType, size_t nIndex) const;

     /**
      * @brief   : 保存信息至抓图图片数据库
      * @param    {std::string} &strFilename：图片文件路径
      * @param    {std::string} &strCurrentDate：图片日期
      * @param    {std::string} &strCurrentTime：图片时间
      * @param    {int} nChnId：通道号
      * @return   {int} 0：成功，非0：失败
      */
     int saveToDatabase(const std::string &strFilename,
                        const std::string &strCurrentDate,
                        const std::string &strCurrentTime,
                        int nChnId);

     /* 人脸抓拍配置 */
     Alarm::FaceCapture_S m_stAlgoCfg;
     /* 算法分辨率宽度 */
     int m_nWidth = PIXEL_WIDTH_640;
     /* 算法分辨率高度 */
     int m_nHeight = PIXEL_HEIGHT_384;
     /* 算法默认最小瞳距 */
     int m_nMinIpd = 20;
     /* 算法最小瞳距下限 */
     const int MIN_IPD = 10;
     /* 算法最小瞳距上限 */
     const int MAX_IPD = 75;
     /* 是否联动保存人脸全景图片 */
     bool m_bFacePanoramicImage = false;
     /* 是否联动保存人脸图片 */
     bool m_bFaceImage = false;
     /* 是否联动邮件 */
     bool m_bEmail = false;
     /* 是否联动上传 SD 卡 */
     bool m_bUploadSdCard = false;
     /* 人脸抓拍报警状态机 */
     CAlarmStateMachine m_alarmStateMachine;
     /* 人脸抓拍 SDK 图片事件推送器 */
     CFaceCaptureSdkEventPublisher m_sdkEventPublisher;
 };
 } // namespace FaceDetectInternal
