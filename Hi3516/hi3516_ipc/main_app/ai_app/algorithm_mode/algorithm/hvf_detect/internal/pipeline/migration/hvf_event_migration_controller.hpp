/**
 * @FilePath     : hvf_event_migration_controller.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-08-28
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-08-28
 * @Description  : HVF 事件族统一迁移控制器，Composition Root 编排 Legacy/New 两态链路
 */

#pragma once

#if CAP_UNIFIED_EVENT_PIPELINE

#include <cstdint>
#include <memory>
#include <mutex>

#include "alarm_define.h"
#include "detection_types.hpp"
#include "dwell_config_adapter.hpp"
#include "dwell_processor.hpp"
#include "face_config_adapter.hpp"
#include "face_processor.hpp"
#include "hisi_frame_image_provider.hpp"
#include "hvf_event_output_executor.hpp"
#include "hvf_result_converter.hpp"
#include "region_config_adapter.hpp"
#include "region_processor.hpp"
#include "result_dispatcher.hpp"
#include "svp_ai_detect.h"
#include "trip_line_config_adapter.hpp"
#include "trip_line_processor.hpp"

/* 旧处理器，Legacy 分支内部持有 */
#include "internal/processors/region/hvf_intrusion_processor.hpp"
#include "internal/processors/region/hvf_enter_exit_processor.hpp"
#include "internal/processors/boundary/hvf_boundary_processor.hpp"
#include "internal/processors/region/hvf_loitering_processor.hpp"
#include "internal/processors/region/hvf_parking_processor.hpp"
#include "internal/processors/face/hvf_face_processor.hpp"

namespace HVFDetectInternal
{
/**
 * @brief   : HVF 事件族统一迁移控制器
 * @note    : 注册 Region Processor（侵入/进入/离开）与 TripLine Processor（越界），
 *            后续阶段追加 Dwell/Face Processor。
 *            lock: 同一把锁串行化配置/清零/启停与逐帧处理，保证配置原子性。
 */
class CHVFEventMigrationController
{
public:
    /**
     * @brief   : 构造事件迁移控制器
     */
    CHVFEventMigrationController();

    /**
     * @brief   : 析构事件迁移控制器
     */
    ~CHVFEventMigrationController() = default;

    /**
     * @brief   : 设置算法分辨率
     * @param    {int} nWidth：算法分辨率宽度
     * @param    {int} nHeight：算法分辨率高度
     * @return   {void}
     * @note    : Legacy 链路按算法分辨率消费配置坐标
     */
    void set_algo_resolution(int nWidth, int nHeight);

    /**
     * @brief   : 设置模型输入尺寸，创建新链路 Converter
     * @param    {const AiPipeline_NS::FrameSize_S &} stModelInputSize：模型输入分辨率
     * @return   {void}
     * @note    : 尺寸非法时新链路保持不可用，回退 Legacy
     */
    void set_model_input_size(const AiPipeline_NS::FrameSize_S &stModelInputSize);

    /**
     * @brief   : 原子应用区域入侵配置
     * @param    {const Alarm::FieldDetection_S &} stConfig：区域入侵配置
     * @param    {bool} bAlgoEnabled：算法总开关
     * @return   {void}
     * @note    : lock: 同锁先 Legacy 后 New
     */
    void apply_intrusion_config(const Alarm::FieldDetection_S &stConfig, bool bAlgoEnabled);

    /**
     * @brief   : 原子应用进入区域配置
     * @param    {const Alarm::EntranceDetection_S &} stConfig：进入区域配置
     * @param    {bool} bAlgoEnabled：算法总开关
     * @return   {void}
     */
    void apply_entrance_config(const Alarm::EntranceDetection_S &stConfig, bool bAlgoEnabled);

    /**
     * @brief   : 原子应用离开区域配置
     * @param    {const Alarm::ExitingDetection_S &} stConfig：离开区域配置
     * @param    {bool} bAlgoEnabled：算法总开关
     * @return   {void}
     */
    void apply_exit_config(const Alarm::ExitingDetection_S &stConfig, bool bAlgoEnabled);

    /**
     * @brief   : 单事件参数热更新：区域入侵
     * @param    {const Alarm::FieldDetection_S &} stConfig：区域入侵配置
     * @return   {void}
     */
    void update_intrusion_parameters(const Alarm::FieldDetection_S &stConfig);

    /**
     * @brief   : 单事件参数热更新：进入区域
     * @param    {const Alarm::EntranceDetection_S &} stConfig：进入区域配置
     * @return   {void}
     */
    void update_entrance_parameters(const Alarm::EntranceDetection_S &stConfig);

    /**
     * @brief   : 单事件参数热更新：离开区域
     * @param    {const Alarm::ExitingDetection_S &} stConfig：离开区域配置
     * @return   {void}
     */
    void update_exit_parameters(const Alarm::ExitingDetection_S &stConfig);

    /**
     * @brief   : 原子应用越界配置
     * @param    {const Alarm::BoundaryDetection_S &} stConfig：越界配置
     * @param    {bool} bAlgoEnabled：算法总开关
     * @return   {void}
     * @note    : lock: 同锁先 Legacy 后 New
     */
    void apply_boundary_config(const Alarm::BoundaryDetection_S &stConfig, bool bAlgoEnabled);

    /**
     * @brief   : 单事件参数热更新：越界
     * @param    {const Alarm::BoundaryDetection_S &} stConfig：越界配置
     * @return   {void}
     */
    void update_boundary_parameters(const Alarm::BoundaryDetection_S &stConfig);

    /**
     * @brief   : 原子应用徘徊侦测配置
     * @param    {const Alarm::LoiteringDetection_S &} stConfig：徘徊侦测配置
     * @param    {bool} bAlgoEnabled：算法总开关
     * @return   {void}
     * @note    : lock: 同锁先 Legacy 后 New
     */
    void apply_loitering_config(const Alarm::LoiteringDetection_S &stConfig, bool bAlgoEnabled);

    /**
     * @brief   : 原子应用停车侦测配置
     * @param    {const Alarm::ParkingDetection_S &} stConfig：停车侦测配置
     * @param    {bool} bAlgoEnabled：算法总开关
     * @return   {void}
     * @note    : lock: 同锁先 Legacy 后 New
     */
    void apply_parking_config(const Alarm::ParkingDetection_S &stConfig, bool bAlgoEnabled);

    /**
     * @brief   : 单事件参数热更新：徘徊侦测
     * @param    {const Alarm::LoiteringDetection_S &} stConfig：徘徊侦测配置
     * @return   {void}
     */
    void update_loitering_parameters(const Alarm::LoiteringDetection_S &stConfig);

    /**
     * @brief   : 单事件参数热更新：停车侦测
     * @param    {const Alarm::ParkingDetection_S &} stConfig：停车侦测配置
     * @return   {void}
     */
    void update_parking_parameters(const Alarm::ParkingDetection_S &stConfig);

    /**
     * @brief   : 原子应用人脸侦测配置
     * @param    {const Alarm::FaceDetection_S &} stConfig：人脸侦测配置
     * @param    {bool} bAlgoEnabled：算法总开关
     * @return   {void}
     * @note    : lock: 同锁先 Legacy 后 New
     */
    void apply_face_config(const Alarm::FaceDetection_S &stConfig, bool bAlgoEnabled);

    /**
     * @brief   : 单事件参数热更新：人脸侦测
     * @param    {const Alarm::FaceDetection_S &} stConfig：人脸侦测配置
     * @return   {void}
     */
    void update_face_parameters(const Alarm::FaceDetection_S &stConfig);

    /**
     * @brief   : 获取当前是否启用（任一事件使能即 true）
     * @return   {bool} true：启用
     */
    bool is_enabled() const;

    /**
     * @brief   : 处理单帧 HVF 结果
     * @param    {ot_aidetect_result_array &} stResult：HVF 原生结果
     * @param    {int} nChnId：通道号
     * @param    {const std::shared_ptr<ot_video_frame_info> &} pFrameInfo：当前帧共享引用
     * @param    {const AiPipeline_NS::FrameSize_S &} stSourceFrameSize：实际源帧分辨率
     * @param    {std::vector<Common::RectInfo_S> &} vstRectInfo：本帧 OSD 汇总框数组
     * @return   {int} OK：成功
     * @note    : lock: 与配置共用同一把锁串行化
     */
    int process(ot_aidetect_result_array &stResult,
                int nChnId,
                const std::shared_ptr<ot_video_frame_info> &pFrameInfo,
                const AiPipeline_NS::FrameSize_S &stSourceFrameSize,
                std::vector<Common::RectInfo_S> &vstRectInfo);

    /**
     * @brief   : 完全重置控制器
     * @return   {void}
     */
    void reset();

private:
    /**
     * @brief   : 把三类旧配置合并为平台无关原始配置并交给共享层适配器归一化
     * @return   {void}
     * @note    : 内部方法，调用方已持锁
     */
    void rebuild_and_apply_region_config();

    /**
     * @brief   : 把 Alarm::BoundaryDetection_S 转为平台无关原始配置并交给共享层适配器归一化
     * @return   {void}
     * @note    : 内部方法，调用方已持锁
     */
    void rebuild_and_apply_trip_line_config();

    /**
     * @brief   : 把 Alarm::FieldDetection_S 转为原始区域规则
     * @param    {const Alarm::FieldDetection_S &} stSrc：区域入侵配置
     * @param    {std::vector<AiPipeline_NS::Region_NS::RawRegionRule_S> &} vecOut：输出规则列表
     * @return   {void}
     */
    void convert_intrusion_rules(const Alarm::FieldDetection_S &stSrc,
                                 std::vector<AiPipeline_NS::Region_NS::RawRegionRule_S> &vecOut) const;

    /**
     * @brief   : 把 Alarm::EntranceDetection_S 转为原始区域规则
     * @param    {const Alarm::EntranceDetection_S &} stSrc：进入区域配置
     * @param    {std::vector<AiPipeline_NS::Region_NS::RawRegionRule_S> &} vecOut：输出规则列表
     * @return   {void}
     */
    void convert_entrance_rules(const Alarm::EntranceDetection_S &stSrc,
                                std::vector<AiPipeline_NS::Region_NS::RawRegionRule_S> &vecOut) const;

    /**
     * @brief   : 把 Alarm::ExitingDetection_S 转为原始区域规则
     * @param    {const Alarm::ExitingDetection_S &} stSrc：离开区域配置
     * @param    {std::vector<AiPipeline_NS::Region_NS::RawRegionRule_S> &} vecOut：输出规则列表
     * @return   {void}
     */
    void convert_exit_rules(const Alarm::ExitingDetection_S &stSrc,
                            std::vector<AiPipeline_NS::Region_NS::RawRegionRule_S> &vecOut) const;

    /**
     * @brief   : 把 Alarm::Region_S 转为归一化坐标点集
     * @param    {const Alarm::Region_S &} stRegion：区域配置
     * @param    {std::vector<Geometry_NS::Point_S> &} vecOut：输出点集
     * @return   {bool} true：有效
     */
    bool convert_region_polygon(const Alarm::Region_S &stRegion,
                                std::vector<AiPipeline_NS::Geometry_NS::Point_S> &vecOut) const;

    /**
     * @brief   : 判断单个区域多边形是否全为 (0,0) 占位
     * @param    {const Alarm::Region_S &} stRegion：区域配置
     * @return   {bool} true：全零占位（视为"未配置"）
     */
    bool is_region_placeholder(const Alarm::Region_S &stRegion) const;

    /**
     * @brief   : 判断区域规则源数组是否全部为占位规则（顶点全为 (0,0)）
     * @param    {const Alarm::FieldDetection_S &} stConfig：入侵配置源
     * @return   {bool} true：全部为占位规则（即用户未配置任何有效区域）
     */
    bool is_all_placeholder(const Alarm::FieldDetection_S &stConfig) const;
    bool is_all_placeholder(const Alarm::EntranceDetection_S &stConfig) const;
    bool is_all_placeholder(const Alarm::ExitingDetection_S &stConfig) const;
    bool is_all_placeholder(const Alarm::BoundaryDetection_S &stConfig) const;
    bool is_all_placeholder(const Alarm::LoiteringDetection_S &stConfig) const;
    bool is_all_placeholder(const Alarm::ParkingDetection_S &stConfig) const;

    /**
     * @brief   : 把 DetectionTarget_E 向量转为位掩码
     * @param    {const std::vector<int> &} vecTargets：检测目标列表
     * @return   {uint32_t} 位掩码
     */
    uint32_t convert_detection_targets(const std::vector<int> &vecTargets) const;

    /**
     * @brief   : 把 Alarm::BoundaryDetection_S 转为原始越界规则列表
     * @param    {const Alarm::BoundaryDetection_S &} stSrc：越界配置
     * @param    {std::vector<AiPipeline_NS::TripLine_NS::RawTripLineRule_S> &} vecOut：输出规则列表
     * @return   {void}
     */
    void convert_boundary_rules(const Alarm::BoundaryDetection_S &stSrc,
                                std::vector<AiPipeline_NS::TripLine_NS::RawTripLineRule_S> &vecOut) const;

    /**
     * @brief   : 把徘徊/停车配置合并为平台无关原始配置并交给共享层适配器归一化
     * @return   {void}
     * @note    : 内部方法，调用方已持锁
     */
    void rebuild_and_apply_dwell_config();

    /**
     * @brief   : 把 Alarm::LoiteringDetection_S 转为原始驻留规则列表
     * @param    {const Alarm::LoiteringDetection_S &} stSrc：徘徊侦测配置
     * @param    {std::vector<AiPipeline_NS::Dwell_NS::RawDwellRule_S> &} vecOut：输出规则列表
     * @return   {void}
     */
    void convert_loitering_rules(const Alarm::LoiteringDetection_S &stSrc,
                                 std::vector<AiPipeline_NS::Dwell_NS::RawDwellRule_S> &vecOut) const;

    /**
     * @brief   : 把 Alarm::ParkingDetection_S 转为原始驻留规则列表
     * @param    {const Alarm::ParkingDetection_S &} stSrc：停车侦测配置
     * @param    {std::vector<AiPipeline_NS::Dwell_NS::RawDwellRule_S> &} vecOut：输出规则列表
     * @return   {void}
     */
    void convert_parking_rules(const Alarm::ParkingDetection_S &stSrc,
                               std::vector<AiPipeline_NS::Dwell_NS::RawDwellRule_S> &vecOut) const;

    /**
     * @brief   : 把 Alarm::FaceDetection_S 转为平台无关原始配置并交给共享层适配器归一化
     * @return   {void}
     * @note    : 内部方法，调用方已持锁
     */
    void rebuild_and_apply_face_config();

    /* lock: 串行化配置、启停与逐帧处理 */
    mutable std::mutex m_mutex;

    /* 算法分辨率 */
    int m_nAlgoWidth = 0;
    int m_nAlgoHeight = 0;
    /* 当前是否启用（任一区域事件使能即 true） */
    bool m_bEnabled = false;
    /* 单调递增帧标识 */
    uint64_t m_ullFrameId = 0;

    /* 三类区域事件缓存配置 */
    Alarm::FieldDetection_S m_stIntrusionConfig;
    Alarm::EntranceDetection_S m_stEntranceConfig;
    Alarm::ExitingDetection_S m_stExitConfig;
    /* 越界事件缓存配置 */
    Alarm::BoundaryDetection_S m_stBoundaryConfig;
    /* 徘徊/停车事件缓存配置 */
    Alarm::LoiteringDetection_S m_stLoiteringConfig;
    Alarm::ParkingDetection_S m_stParkingConfig;
    /* 人脸侦测事件缓存配置 */
    Alarm::FaceDetection_S m_stFaceConfig;
    /* 各事件的算法总开关 */
    bool m_bIntrusionAlgoEnabled = false;
    bool m_bEntranceAlgoEnabled = false;
    bool m_bExitAlgoEnabled = false;
    bool m_bBoundaryAlgoEnabled = false;
    bool m_bLoiteringAlgoEnabled = false;
    bool m_bParkingAlgoEnabled = false;
    bool m_bFaceAlgoEnabled = false;

    /* 各事件上一次生效的使能态，用于日志降噪（仅使能翻转时打 info） */
    bool m_bPrevIntrusionEnabled = false;
    bool m_bPrevEntranceEnabled = false;
    bool m_bPrevExitEnabled = false;
    bool m_bPrevBoundaryEnabled = false;
    bool m_bPrevLoiteringEnabled = false;
    bool m_bPrevParkingEnabled = false;
    bool m_bPrevFaceEnabled = false;

    /* Legacy 处理器（控制器内部持有，与人流控制器模式一致） */
    CHVFIntrusionProcessor m_legacyIntrusion;
    CHVFEnterExitProcessor m_legacyEnterExit;
    CHVFBoundaryProcessor m_legacyBoundary;
    CHVFLoiteringProcessor m_legacyLoitering;
    CHVFParkingProcessor m_legacyParking;
    CHVFFaceProcessor m_legacyFace;

#if CAP_UNIFIED_EVENT_PIPELINE
    /* 新链路结果转换器 */
    std::unique_ptr<CHisiHvfResultConverter> m_pConverter;
    /* 新链路结果分发器 */
    AiPipeline_NS::CResultDispatcher m_dispatcher;
    /* 新链路区域处理器，所有权归属 Dispatcher */
    AiPipeline_NS::Region_NS::CRegionProcessor *m_pRegionProcessor = nullptr;
    /* 新链路越界处理器，所有权归属 Dispatcher */
    AiPipeline_NS::TripLine_NS::CTripLineProcessor *m_pTripLineProcessor = nullptr;
    /* 新链路驻留处理器（徘徊/停车），所有权归属 Dispatcher */
    AiPipeline_NS::Dwell_NS::CDwellProcessor *m_pDwellProcessor = nullptr;
    /* 新链路人脸侦测处理器，所有权归属 Dispatcher */
    AiPipeline_NS::Face_NS::CFaceProcessor *m_pFaceProcessor = nullptr;
    /* 新链路输出执行器 */
    std::unique_ptr<CHVFEventOutputExecutor> m_pOutputExecutor;
    /* 新链路帧图片 Provider */
    CHisiFrameImageProvider m_imageProvider;
    /* 区域配置适配器 */
    AiPipeline_NS::Region_NS::CRegionConfigAdapter m_regionConfigAdapter;
    /* 越界配置适配器 */
    AiPipeline_NS::TripLine_NS::CTripLineConfigAdapter m_tripLineConfigAdapter;
    /* 驻留配置适配器（徘徊/停车） */
    AiPipeline_NS::Dwell_NS::CDwellConfigAdapter m_dwellConfigAdapter;
    /* 人脸侦测配置适配器 */
    AiPipeline_NS::Face_NS::CFaceConfigAdapter m_faceConfigAdapter;
    /* 原生结果坐标来源 */
    NativeResultCoordinateSource_E m_enCoordinateSource = NativeResultCoordinateSource_E::MODEL_INPUT;
    /* 模型输入尺寸 */
    AiPipeline_NS::FrameSize_S m_stModelInputSize;
    /* 新链路是否就绪 */
    bool m_bNewPipelineReady = false;
#endif
};
} // namespace HVFDetectInternal

#endif // CAP_UNIFIED_EVENT_PIPELINE
