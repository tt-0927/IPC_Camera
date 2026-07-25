/*
 * @FilePath     : ScenarioBase.hpp
 * @Author       : 严泽辉 yanzeh@kfb.cn
 * @Date         : 2024-05-17 16:47:47
 * @LastEditors: 李辉 lihui@kfb.cn
 * @LastEditTime: 2024-10-31 16:39:43
 * @Description  :
 */
#pragma once

#include <ctime>
#include <iomanip>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>

#include "CAExtern.hpp"
#include "CVExtern.hpp"

namespace Scenario_NS
{
    class CScenarioBase
    {
    public:

        CScenarioBase(AiScenario_NS::InParam_S stInParam)
            : m_stInParam(stInParam)
        {
        }

        virtual ~CScenarioBase()
        {
        }

        /* 使用RGB颜色 */
        static cv::Scalar GetUniqueColor(int nId)
        {
            static std::unordered_map<int, cv::Scalar> colorMap;
            if (colorMap.size() <= 0)
            {
                /* BGR */
                colorMap[-1] = cv::Scalar(200, 200, 200);
                colorMap[0]  = cv::Scalar(0, 0, 255);
                colorMap[1]  = cv::Scalar(0, 255, 0);
                colorMap[2]  = cv::Scalar(255, 0, 0);
                colorMap[3]  = cv::Scalar(0, 255, 255);
                colorMap[4]  = cv::Scalar(255, 0, 255);
                colorMap[5]  = cv::Scalar(255, 255, 0);
                colorMap[6]  = cv::Scalar(0, 0, 0);
            }
            if (colorMap.find(nId) == colorMap.end())
            {
                /* 生成一个亮色并存储在映射中 */
                int brightness = 128; /* 设置最小亮度 */
                colorMap[nId]  = cv::Scalar(
                    brightness + rand() % (255 - brightness),
                    brightness + rand() % (255 - brightness),
                    brightness + rand() % (255 - brightness));
            }
            return colorMap[nId];
        }

        bool saveImage(const cv::Mat& image, const std::string& strOutputPath)
        {
            struct stat info;
            /* 目录不存在 */
            if (stat(strOutputPath.c_str(), &info) != 0)
            {
                /* 使用命令 mkdir -p 来递归创建目录 */
                std::string strCmd = "mkdir -p \"" + strOutputPath + "\"";

                int nRet = system(strCmd.c_str());
                if (nRet != 0)
                {
                    return false;
                }
            }

            /* 获取当前时间（精确到微秒） */
            auto now          = std::chrono::system_clock::now();
            auto time_t_now   = std::chrono::system_clock::to_time_t(now);
            auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000000;

            /* 将 time_t 转换为本地时间 */
            struct tm timeinfo;
#ifdef _WIN32
            localtime_s(&timeinfo, &time_t_now);
#else
            localtime_r(&time_t_now, &timeinfo);
#endif

            /* 格式化时间戳，精确到微秒 */
            std::ostringstream timestamp;
            timestamp << std::put_time(&timeinfo, "%Y%m%d_%H%M%S")
                      << "_" << std::setw(6) << std::setfill('0') << microseconds.count();

            /* 构造完整的文件名（包含路径） */
            std::ostringstream filename;
            filename << strOutputPath << "/image_" << timestamp.str() << ".jpg";

            /* 使用 OpenCV 的 imwrite 函数保存图像 */
            bool bSaved = cv::imwrite(filename.str(), image);

            return bSaved; /* 返回保存结果 */
        }

        /**
         * @brief 设置参数
         * @param [InParam_S] stInParam: 参数
         * @return [*]
         * @note 重新初始化生效
         */
        bool setParam(AiScenario_NS::InParam_S stInParam)
        {
            m_stInParam = stInParam;
            return true;
        }

        /**
         * @brief 初始化
         * @return [*]
         * @note
         */
        virtual bool init() = 0;

        /**
         * @brief 反初始化
         * @return [*]
         * @note
         */
        virtual bool unInit() = 0;

        /**
         * @brief 处理数据
         * @param [CVData_S] stInData: 传入的视频数据
         * @param [char*&] pchOutData: 输出的处理结果
         * @return [*]
         * @note
         */
        virtual bool process(AiScenario_NS::CVData_S stInData, char*& pchOutData, int& nDataSize) = 0;

        /**
         * @brief 处理数据
         * @param [CAData_S] stInData: 传入的音频数据
         * @param [char*&] pchOutData: 输出的处理结果
         * @return [*]
         * @note
         */
        virtual bool process(AiScenario_NS::CAData_S stInData, char*& pchOutData, int& nDataSize) = 0;

        /**
         * @brief 释放处理结果
         * @param [char*&] pchOutData: 处理结果指针
         * @return [*]
         * @note
         */
        virtual bool releaseData(char*& pchOutData) = 0;

    protected:

        AiScenario_NS::InParam_S m_stInParam;
    };

}    // namespace Scenario_NS
