/**
 * @FilePath     : video_config_compat.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-01-13
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-02 16:21:10
 * @Description  : 视频配置版本兼容性实现示例
 */

#pragma once

#include "config_version.h"
#include "config_validator.h"
#include "config_migrator.h"
#include "video_define.h"
#include "dlog.h"

/* 视频配置码流ID最小值 */
#define VIDEO_CONFIG_ID_MIN         0
/* 视频配置码流ID最大值 */
#define VIDEO_CONFIG_ID_MAX         2
/* 视频ROI/区域裁剪 码流ID最小值 */
#define VIDEO_ID_MIN         0
/* 视频ROI/区域裁剪 码流ID最大值 */
#define VIDEO_ID_MAX         1

#if CAP_VIDEO_MAX_4K // 视频最大分辨率到 4K
    #define VIDEO_RESOLUTION_WIDTH_MIN  PIXEL_WIDTH_352
    #define VIDEO_RESOLUTION_WIDTH_MAX  PIXEL_WIDTH_4K
    #define VIDEO_RESOLUTION_HEIGHT_MIN PIXEL_HEIGHT_288
    #define VIDEO_RESOLUTION_HEIGHT_MAX PIXEL_HEIGHT_4K
    #define VIDEO_RESOLUTION_WIDTH_DEFAULT  PIXEL_WIDTH_1920
    #define VIDEO_RESOLUTION_HEIGHT_DEFAULT PIXEL_HEIGHT_1080
#elif CAP_VIDEO_MAX_2_5K //视频最大分辨率到 2.5K
    #define VIDEO_RESOLUTION_WIDTH_MIN      PIXEL_WIDTH_352
    #define VIDEO_RESOLUTION_WIDTH_MAX      PIXEL_WIDTH_2_5K
    #define VIDEO_RESOLUTION_HEIGHT_MIN     PIXEL_HEIGHT_288
    #define VIDEO_RESOLUTION_HEIGHT_MAX     PIXEL_HEIGHT_2_5K
    #define VIDEO_RESOLUTION_WIDTH_DEFAULT  PIXEL_WIDTH_1920
    #define VIDEO_RESOLUTION_HEIGHT_DEFAULT PIXEL_HEIGHT_1080
#else
    /* 视频分辨率宽最小值 */
    #define VIDEO_RESOLUTION_WIDTH_MIN  PIXEL_WIDTH_352
    /* 视频分辨率宽最大值 */
    #define VIDEO_RESOLUTION_WIDTH_MAX  PIXEL_WIDTH_1920
    /* 视频分辨率高最小值 */
    #define VIDEO_RESOLUTION_HEIGHT_MIN PIXEL_HEIGHT_288
    /* 视频分辨率高最大值 */
    #define VIDEO_RESOLUTION_HEIGHT_MAX PIXEL_HEIGHT_1080
    /* 视频分辨率宽默认值 */
    #define VIDEO_RESOLUTION_WIDTH_DEFAULT  PIXEL_WIDTH_1920
    /* 视频分辨率高默认值 */
    #define VIDEO_RESOLUTION_HEIGHT_DEFAULT PIXEL_HEIGHT_1080
#endif

namespace ConfigCompat_NS
{
    //*===========================================================================*/
    //*                     VideoConfig_S 版本迁移器特化                          */
    //*===========================================================================*/

    /**
     * @brief   : VideoConfig_S的迁移器特化
     * @note    : 定义版本号、迁移步骤等
     */
    template <>
    class CConfigMigrator<Video_NS::VideoConfig_S>
    {
    public:
        /**
         * @brief   : 获取当前代码支持的VideoConfig版本
         * @note    : 每次结构体变更时更新此版本号(eg. 1.1.0: 新增 xxx 字段)
         *            - 1.0.0: 初始版本
         * @return   {ConfigVersion_S} 当前版本
         */
        static ConfigVersion_S getCurrentVersion()
        {
            return ConfigVersion_S(1, 0, 0);
        }

        /**
         * @brief   : 获取配置类型标识符
         * @return   {std::string} 类型标识符
         */
        static std::string getConfigType()
        {
            return "VideoConfig";
        }

        /**
         * @brief   : 获取迁移步骤列表
         * @return   {std::vector<MigrateStep_S>} 迁移步骤
         */
        static std::vector<MigrateStep_S> getMigrateSteps()
        {
            std::vector<MigrateStep_S> steps;

            /* 0.0.0 -> 1.0.0: 无版本到初始版本 */
            steps.emplace_back(
                ConfigVersion_S(0, 0, 0),
                ConfigVersion_S(1, 0, 0),
                [](Json::Object* pRootJson) -> bool
                {
                    /* 旧配置无版本号，直接标记为1.0.0 */
                    return true;
                },
                "初始化版本信息");

            // note eg. 1.0.0 -> 1.1.0: 新增码流平滑字段
            // steps.emplace_back(
            //     ConfigVersion_S(1, 0, 0),
            //     ConfigVersion_S(1, 1, 0),
            //     [](Json::Object* pRootJson) -> bool
            //     {
            //         /* 添加 BitrateSmoothing 字段，默认值50 */
            //         addFieldIfMissing(pRootJson, "BitrateSmoothing", 50);
            //         return true;
            //     },
            //     "Add BitrateSmoothing field");

            return steps;
        }

        /**
         * @brief   : 执行迁移
         * @note    : 使用 CMigrateExecutor 提供的标准迁移流程
         */
        static MigrateResult_E migrate(Json::Object* pRootJson,
                                       const ConfigVersion_S& fromVersion,
                                       const ConfigVersion_S& toVersion)
        {
            return CMigrateExecutor::execute(pRootJson, fromVersion, toVersion, getConfigType(), getMigrateSteps());
        }
    };

    //*===========================================================================*/
    //*                     VideoConfig_S 验证器特化                              */
    //*===========================================================================*/

    /**
     * @brief   : VideoConfig_S的验证器特化
     * @note    : 定义各字段的有效范围和默认值
     */
    template <>
    class CConfigValidator<Video_NS::VideoConfig_S>
    {
    public:
        /**
         * @brief   : 验证并修复VideoConfig
         * @param    {VideoConfig_S} &config：待验证的配置
         * @return   {ConfigValidateResult_S} 验证结果
         */
        static ConfigValidateResult_S validate(Video_NS::VideoConfig_S& config)
        {
            using namespace Video_NS;
            ConfigValidateResult_S result;

            /* 验证码流ID范围 [VIDEO_CONFIG_ID_MIN, VIDEO_CONFIG_ID_MAX] */
            result.addFieldResult(
                CRangeValidator<int>::validate(config.nId, VIDEO_CONFIG_ID_MIN, VIDEO_CONFIG_ID_MAX, 0, "nId"));

            /* 验证视频类型枚举有效性 */
            result.addFieldResult(CRangeValidator<VideoType_E>::validateEnumRange(config.enVideoType,
                                                                                  VideoType_E::COMPOSITE_STREAM,
                                                                                  VideoType_E::VIDEO_STREAM,
                                                                                  VideoType_E::COMPOSITE_STREAM,
                                                                                  "enVideoType"));

            /* 验证分辨率宽度 [VIDEO_RESOLUTION_WIDTH_MIN, VIDEO_RESOLUTION_WIDTH_MAX] */
            result.addFieldResult(
                CRangeValidator<int>::validate(config.stVideoResolution.nWidth, VIDEO_RESOLUTION_WIDTH_MIN, VIDEO_RESOLUTION_WIDTH_MAX, VIDEO_RESOLUTION_WIDTH_DEFAULT, "nWidth"));

            /* 验证分辨率高度 [VIDEO_RESOLUTION_HEIGHT_MIN, VIDEO_RESOLUTION_HEIGHT_MAX] */
            result.addFieldResult(
                CRangeValidator<int>::validate(config.stVideoResolution.nHeight, VIDEO_RESOLUTION_HEIGHT_MIN, VIDEO_RESOLUTION_HEIGHT_MAX, VIDEO_RESOLUTION_HEIGHT_DEFAULT, "nHeight"));

            /* 验证码率类型枚举有效性 */
            result.addFieldResult(CRangeValidator<BitrateType_E>::validateEnumRange(config.enBitrateType,
                                                                                    BitrateType_E::CBR,
                                                                                    BitrateType_E::QPMAP,
                                                                                    BitrateType_E::CBR,
                                                                                    "enVideoType"));

            /* 验证图像质量枚举有效性 */
            result.addFieldResult(
                CRangeValidator<Video_NS::ImageQuality_E>::validateEnum(config.enImageQuality,
                                                                        { Video_NS::ImageQuality_E::LOWEST,
                                                                          Video_NS::ImageQuality_E::LOWER,
                                                                          Video_NS::ImageQuality_E::LOW,
                                                                          Video_NS::ImageQuality_E::MEDIUM,
                                                                          Video_NS::ImageQuality_E::HIGHER,
                                                                          Video_NS::ImageQuality_E::HIGHEST },
                                                                        Video_NS::ImageQuality_E::MEDIUM,
                                                                        "enImageQuality"));

            /* 验证帧率枚举有效性 */
            result.addFieldResult(CRangeValidator<FrameRate_E>::validateEnumRange(
                config.enFrameRate,
                static_cast<FrameRate_E>(Video_NS::FRAME_RATE_ALL + 1),
                static_cast<FrameRate_E>(Video_NS::FRAME_RATE_TOTAL - 1),
                Video_NS::FRAME_RATE_30,
                "enFrameRate"));

            /* 验证码率上限 [256, 16384] */
            result.addFieldResult(
                CRangeValidator<int>::validate(config.nBitrateUpperLimit, 256, 16384, 4096, "nBitrateUpperLimit"));

            /* 验证平均码率 [32, 16384] */
            result.addFieldResult(
                CRangeValidator<int>::validate(config.nAverageBitrate, 32, 16384, 2048, "nAverageBitrate"));

            /* 验证视频编码枚举有效性 */
            result.addFieldResult(CRangeValidator<VideoCodec_E>::validateEnumRange(config.enVideoCodec,
                                                                                   VideoCodec_E::H264,
                                                                                   VideoCodec_E::MPEG4,
                                                                                   VideoCodec_E::H265,
                                                                                   "enVideoCodec"));

            /* 验证编码复杂度枚举有效性 */
            result.addFieldResult(CRangeValidator<int>::validateEnumRange((int&) config.enEncodingComplexity,
                                                                 (int) EncodingComplexity_E::Baseline,
                                                                 (int) EncodingComplexity_E::High,
                                                                 (int) EncodingComplexity_E::Main,
                                                                 "enEncodingComplexity"));

            /* 验证I帧间隔 [1, 400] */
            result.addFieldResult(
                CRangeValidator<int>::validate(config.nIFrameInterval, 1, 400, 25, "nIFrameInterval"));

            /* 验证SVC模式枚举有效性 */
            result.addFieldResult(CRangeValidator<SvcMode_E>::validateEnumRange(config.enSvcEnable,
                                                                                SvcMode_E::SVC_MODE_DISABLE,
                                                                                SvcMode_E::SVC_MODE_AUTO,
                                                                                SvcMode_E::SVC_MODE_DISABLE,
                                                                                "enSvcEnable"));

            /* 验证码流平滑 [1, 100] */
            result.addFieldResult(
                CRangeValidator<int>::validate(config.nBitrateSmoothing, 1, 100, 50, "nBitrateSmoothing"));

            return result;
        }
    };

    //*===========================================================================*/
    //*                     AreaCrop_S 版本迁移器特化                             */
    //*===========================================================================*/

    template <>
    class CConfigMigrator<Video_NS::AreaCrop_S>
    {
    public:
        /**
         * @brief   : 获取当前代码支持的AreaCrop版本
         * @note    : 每次结构体变更时更新此版本号(eg. 1.1.0: 新增 xxx 字段)
         *            - 1.0.0: 初始版本
         * @return   {ConfigVersion_S} 当前版本
         */
        static ConfigVersion_S getCurrentVersion()
        {
            return ConfigVersion_S(1, 0, 0);
        }

        static std::string getConfigType()
        {
            return "AreaCrop";
        }

        static std::vector<MigrateStep_S> getMigrateSteps()
        {
            std::vector<MigrateStep_S> steps;

            steps.emplace_back(
                ConfigVersion_S(0, 0, 0),
                ConfigVersion_S(1, 0, 0),
                [](Json::Object* pRootJson) -> bool
                {
                    return true;
                },
                "初始化版本信息");

            return steps;
        }

        /**
         * @brief   : 执行迁移
         * @note    : 使用 CMigrateExecutor 提供的标准迁移流程
         */
        static MigrateResult_E migrate(Json::Object* pRootJson,
                                       const ConfigVersion_S& fromVersion,
                                       const ConfigVersion_S& toVersion)
        {
            return CMigrateExecutor::execute(pRootJson, fromVersion, toVersion, getConfigType(), getMigrateSteps());
        }
    };

    //*===========================================================================*/
    //*                     AreaCrop_S 验证器特化                                 */
    //*===========================================================================*/

    template <>
    class CConfigValidator<Video_NS::AreaCrop_S>
    {
    public:
        static ConfigValidateResult_S validate(Video_NS::AreaCrop_S& config)
        {
            ConfigValidateResult_S result;

            /* 验证码流ID范围 [VIDEO_ID_MIN, VIDEO_ID_MAX] */
            result.addFieldResult(CRangeValidator<int>::validate(config.nId, VIDEO_ID_MIN, VIDEO_ID_MAX, 0, "nId"));

            /* 验证分辨率宽度 [VIDEO_RESOLUTION_WIDTH_MIN, VIDEO_RESOLUTION_WIDTH_MAX] */
            result.addFieldResult(CRangeValidator<int>::validate(config.stResolution.nWidth,
                                                                 VIDEO_RESOLUTION_WIDTH_MIN,
                                                                 VIDEO_RESOLUTION_WIDTH_MAX,
                                                                 VIDEO_RESOLUTION_WIDTH_DEFAULT,
                                                                 "nWidth"));

            /* 验证分辨率高度 [VIDEO_RESOLUTION_HEIGHT_MIN, VIDEO_RESOLUTION_HEIGHT_MAX] */
            result.addFieldResult(CRangeValidator<int>::validate(config.stResolution.nHeight,
                                                                 VIDEO_RESOLUTION_HEIGHT_MIN,
                                                                 VIDEO_RESOLUTION_HEIGHT_MAX,
                                                                 VIDEO_RESOLUTION_HEIGHT_DEFAULT,
                                                                 "nHeight"));

            /* 验证裁剪区域坐标 */
            result.addFieldResult(
                CRangeValidator<int>::validate(config.stRect.nX, 0, VIDEO_RESOLUTION_WIDTH_MAX, 0, "stRect.nX"));
            result.addFieldResult(
                CRangeValidator<int>::validate(config.stRect.nY, 0, VIDEO_RESOLUTION_HEIGHT_MAX, 0, "stRect.nY"));
            result.addFieldResult(CRangeValidator<int>::validate(config.stRect.nWidth,
                                                                 0,
                                                                 VIDEO_RESOLUTION_WIDTH_MAX,
                                                                 VIDEO_RESOLUTION_WIDTH_DEFAULT,
                                                                 "stRect.nWidth"));
            result.addFieldResult(CRangeValidator<int>::validate(config.stRect.nHeight,
                                                                 0,
                                                                 VIDEO_RESOLUTION_HEIGHT_MAX,
                                                                 VIDEO_RESOLUTION_HEIGHT_DEFAULT,
                                                                 "stRect.nHeight"));

            return result;
        }
    };

    //*===========================================================================*/
    //*                     VideoRoiConfig_S 版本迁移器特化                       */
    //*===========================================================================*/

    template <>
    class CConfigMigrator<Video_NS::VideoRoiConfig_S>
    {
    public:
        static ConfigVersion_S getCurrentVersion()
        {
            return ConfigVersion_S(1, 0, 0);
        }

        static std::string getConfigType()
        {
            return "VideoRoiConfig";
        }

        static std::vector<MigrateStep_S> getMigrateSteps()
        {
            std::vector<MigrateStep_S> steps;

            steps.emplace_back(
                ConfigVersion_S(0, 0, 0),
                ConfigVersion_S(1, 0, 0),
                [](Json::Object* pRootJson) -> bool
                {
                    return true;
                },
                "初始化版本信息");

            return steps;
        }

        /**
         * @brief   : 执行迁移
         * @note    : 使用 CMigrateExecutor 提供的标准迁移流程
         */
        static MigrateResult_E migrate(Json::Object* pRootJson,
                                       const ConfigVersion_S& fromVersion,
                                       const ConfigVersion_S& toVersion)
        {
            return CMigrateExecutor::execute(pRootJson, fromVersion, toVersion, getConfigType(), getMigrateSteps());
        }
    };

    //*===========================================================================*/
    //*                     VideoRoiConfig_S 验证器特化                           */
    //*===========================================================================*/

    template <>
    class CConfigValidator<Video_NS::VideoRoiConfig_S>
    {
    public:
        static ConfigValidateResult_S validate(Video_NS::VideoRoiConfig_S& config)
        {
            ConfigValidateResult_S result;

            /* 验证码流ID [VIDEO_ID_MIN, VIDEO_ID_MAX] */
            result.addFieldResult(CRangeValidator<int>::validate(config.nId, VIDEO_ID_MIN, VIDEO_ID_MAX, 0, "nId"));

            /* 验证每个ROI区域 */
            for (size_t i = 0; i < config.vstVideoRoi.size(); ++i)
            {
                auto& roi = config.vstVideoRoi[i];
                std::string prefix = "vstVideoRoi[" + std::to_string(i) + "].";

                /* 验证区域编号 [0, VIDEO_ROI_CONFIG_NUMBER - 1] */
                result.addFieldResult(CRangeValidator<uint32_t>::validate(roi.u32Idx,
                                                                          0,
                                                                          VIDEO_ROI_CONFIG_NUMBER - 1,
                                                                          0,
                                                                          prefix + "u32Idx"));

                /* 验证提示等级 [1, 6] */
                result.addFieldResult(CRangeValidator<uint32_t>::validate(roi.u32Level, 1, 6, 3, prefix + "u32Level"));

                /* 验证区域名称 最长为12字符 */
                result.addFieldResult(
                    CStringValidator::validateLength(roi.strRegionName, 12, prefix + "strRegionName"));

                /* 验证区域坐标 以插件1920*1080为限 */
                result.addFieldResult(CRangeValidator<int>::validate(roi.stRect.nX,
                                                                     0,
                                                                     VIDEO_RESOLUTION_WIDTH_DEFAULT,
                                                                     0,
                                                                     prefix + "stRect.nX"));
                result.addFieldResult(CRangeValidator<int>::validate(roi.stRect.nY,
                                                                     0,
                                                                     VIDEO_RESOLUTION_HEIGHT_DEFAULT,
                                                                     0,
                                                                     prefix + "stRect.nY"));
                result.addFieldResult(CRangeValidator<int>::validate(roi.stRect.nWidth,
                                                                     0,
                                                                     VIDEO_RESOLUTION_WIDTH_DEFAULT,
                                                                     0,
                                                                     prefix + "stRect.nWidth"));
                result.addFieldResult(CRangeValidator<int>::validate(roi.stRect.nHeight,
                                                                     0,
                                                                     VIDEO_RESOLUTION_HEIGHT_DEFAULT,
                                                                     0,
                                                                     prefix + "stRect.nHeight"));
            }

            return result;
        }
    };

} /* namespace ConfigCompat_NS */
