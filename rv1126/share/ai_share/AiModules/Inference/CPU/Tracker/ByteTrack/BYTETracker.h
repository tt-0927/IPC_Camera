
/*
 * @FilePath     : BYTETracker.h
 * @Author       : 吴才朋 wucp@kfb.cn
 * @Date         : 2024-09-23 19:48:15
 * @LastEditors  : 严泽辉 yanzeh@kfb.cn
 * @LastEditTime : 2024-09-27 09:40:05
 * @Description  : Bytetrack跟踪算法接口
 */

#pragma once

#include "STrack.h"

/* 跟踪结构体 */
typedef struct _DetectResult_
{
    int              nClassId;    /* 类别ID */
    float            fConfidence; /* 置信度 */
    cv::Rect_<float> vfBox;       /* 目标框（cx,cy,w,h）*/
}DetectResult_S;

namespace Inference_NS
{
    class cBYTETracker
    {
    public:

        /**
         * @brief ByteTrack 跟踪算法
         * @param [char*] pchTime: 时间字符串，格式 2023-11-30 19:35:30
         * @param [tm*] pstTime: 转化后的结构体
         * @return [*] BlError_E::OK 成功  其他失败
         * @note
         */
        cBYTETracker(int nFrameRate = 30, int nTrackBuffer = 30);
        ~cBYTETracker();

        /**
         * @brief 更新坐标，重新获取ID
         * @param [const std::vector<DetectResult_S>&] pstTime: 转化后的结构体
         * @return [std::vector<cSTrack>] 返回包含跟踪框相关的信息[.tlwh容器，包含目标框（cx,cy,w,h）]
         * @note
         */
        std::vector<cSTrack> update(const std::vector<DetectResult_S>& vObjects);

        /**
         * @brief 随机获取绘制的颜色色
         * @param [int] nIdx: 跟踪得到的ID
         * @return [cv::Scalar] open绘制的bgr颜色值
         * @note
         */
        cv::Scalar getColor(int nIdx);

        /**
         * @brief 设置跟踪相关的信息
         * @param [float] fTrackThresh[0-1,0.8]: 追踪阈值，这个值用于设置初始目标检测的置信度阈值。
         * @param [float] fHighThresh[0-1]: 高置信度阈值，用于确定哪些检测结果非常可靠。
         * @param [float] fMatchThresh[0-1]: 匹配阈值，在目标跟踪过程中，这个值用于决定两帧之间跟踪目标是否匹配。
         * @param [int] nFrameId: 起始的ID
         * @param [int] nMaxTimeLost[>0]: 最大丢失时间，这个变量决定跟踪对象在连续几帧未能匹配到检测结果时，会被认为丢失。
         * @return
         * @note
         */
        void setValue(float fTrackThresh = 0.8, float fHighThresh = 0.6, float fMatchThresh = 0.8, int nFrameId = 0, int nMaxTimeLost = 30)
        {
            m_fTrackThresh = fTrackThresh;
            m_fHighThresh  = fHighThresh;
            m_fMatchThresh = fMatchThresh;
            m_nFrameId     = nFrameId;
            m_nMaxTimeLost = nMaxTimeLost;
        };

    private:

        std::vector<cSTrack*> joint_stracks(std::vector<cSTrack*>& tlista, std::vector<cSTrack>& tlistb);
        std::vector<cSTrack>  joint_stracks(std::vector<cSTrack>& tlista, std::vector<cSTrack>& tlistb);

        std::vector<cSTrack> sub_stracks(std::vector<cSTrack>& tlista, std::vector<cSTrack>& tlistb);
        void                 remove_duplicate_stracks(std::vector<cSTrack>& resa, std::vector<cSTrack>& resb, std::vector<cSTrack>& stracksa, std::vector<cSTrack>& stracksb);

        void                              linear_assignment(std::vector< std::vector<float> >& cost_matrix, int cost_matrix_size, int cost_matrix_size_size, float thresh, std::vector< std::vector<int> >& matches, std::vector<int>& unmatched_a, std::vector<int>& unmatched_b);
        std::vector< std::vector<float> > iou_distance(std::vector<cSTrack*>& atracks, std::vector<cSTrack>& btracks, int& dist_size, int& dist_size_size);
        std::vector< std::vector<float> > iou_distance(std::vector<cSTrack>& atracks, std::vector<cSTrack>& btracks);
        std::vector< std::vector<float> > ious(std::vector< std::vector<float> >& atlbrs, std::vector< std::vector<float> >& btlbrs);

        double lapjv(const std::vector< std::vector<float> >& cost, std::vector<int>& rowsol, std::vector<int>& colsol, bool extend_cost = false, float cost_limit = LONG_MAX, bool return_cost = true);

    private:

        /* 追踪阈值，这个值用于设置初始目标检测的置信度阈值。当检测器返回的检测框置信度大于 fTrackThresh 时，会被认为是潜在的跟踪对象。 */
        float m_fTrackThresh;
        /* 高置信度阈值，用于确定哪些检测结果非常可靠。通常，用于在后续步骤中减少误匹配或过滤掉低质量的检测。 */
        float m_fHighThresh;
        /* 匹配阈值，在目标跟踪过程中，这个值用于决定两帧之间跟踪目标是否匹配。具体来说，当计算两个目标之间的相似性或距离时，只有相似性高于 fMatchThresh 的才会认为是同一个目标。 */
        float m_fMatchThresh;
        /* 起始的ID */
        int   m_nFrameId;
        /* 最大丢失时间，这个变量决定跟踪对象在连续几帧未能匹配到检测结果时，会被认为丢失。 */
        int   m_nMaxTimeLost;

        std::vector<cSTrack>          tracked_stracks;
        std::vector<cSTrack>          lost_stracks;
        std::vector<cSTrack>          removed_stracks;
        byte_kalman::ByteKalmanFilter kalman_filter;
    };

}    // namespace Inference_NS