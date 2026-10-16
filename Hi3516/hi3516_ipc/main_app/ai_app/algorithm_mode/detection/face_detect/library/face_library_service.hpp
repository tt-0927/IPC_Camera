/**
 * @FilePath     : face_library_service.hpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-21
 * @Description  : 人脸名单库（目标库）服务——上传图入库、库比对与人员详情查询的统一出口。
 *
 * 实现状态（比对迁移阶段二已落地）：
 * - enrollFromImage：入库编排照搬 Legacy addFaceLibGroup 语义，执行器换成自建的
 *   临时轻量 Worker（CFaceDetectWorker，start(true) 低内存互斥模式：YOLO 与
 *   ArcFace 按任务切换驻留、内存峰值受控），不依赖常驻 Worker——入库期间常驻
 *   算法已被调用方的独占框架（beginFaceLibExclusive/endFaceLibExclusive）暂停；
 *   入库毕即销毁临时 Worker，先于调用方恢复常驻算法。
 * - 调用方 algo_stream_deal::add_Facelib_Groups 保留独占框架（暂停常驻算法、
 *   期望配置暂存恢复、入库互斥锁），仅独占体内实现换到本服务；检测层未启用
 *   （CAP_UNIFIED_DETECTION=0）的 Legacy 画像仍走 Legacy 临时实例路径。
 * - compareFeature/queryById：转发 FaceManage::AIFaceManage 单例，行为不变，
 *   收敛统一出口；名单增删查（algo_control_deal 直连 AIFaceManage）不经本服务。
 * - 入库图片来源仅上传图片（平台/CGI 提供 NV21 文件，control 侧下载落盘）。
 * - enrollFromLiveTrack：实时流取优入库（以跟踪 trackId 为会话键订阅
 *   CBestShotHub 的 onNewBest），仍预留未实现，触发入口产品化另定。
 *
 * 返回码：FaceLibAddErrorCode（本文件定义，数值与 Legacy
 * face_feature_processor.hpp 的全局枚举完全一致，平台按数值解读不变）；Legacy
 * 存量"大量失败分支 return false == FACE_LIB_ADD_OK 被上游当成功"的问题在本
 * 实现中修复，每个失败分支返回真实错误码。
 *
 * 线程契约：入库由调用方串行化（algo_stream_deal 的入库互斥锁 + AI 资源独占
 * 框架），本服务自身不做重入保护；比对/查询转发 AIFaceManage（内部自持锁），
 * 可跨线程调用。
 */

#pragma once

#include <string>
#include <vector>

#include "face_manage_ext.hpp"

namespace FaceChain_NS
{
#if CAP_AI_FACE_COMPARE
/*
 * 名单库入库返回码。数值与 Legacy face_feature_processor.hpp 的全局同名枚举
 * 逐一对应（平台/CGI 按数值解读，两侧并存互不影响：本枚举在 FaceChain_NS
 * 命名空间内，不会与 Legacy 全局枚举构成重定义）。
 */
enum FaceLibAddErrorCode
{
    FACE_LIB_ADD_OK = 0,
    ERR_DETECT_NO_FACES = -4,                // 未检测到人脸
    ERR_DETECT_MULTIPLE_FACES = -2,          // 检测到多张人脸
    ERR_ADD_DUPLICATE_FACE = -3,             // 重复人脸
    ERR_FACE_FEATURE_INIT_FAILED = -5,       // 特征处理器初始化失败
    ERR_FACE_SOURCE_FRAME_ALLOC_FAILED = -6, // 原图视频帧分配失败（通常为内存不足）
    ERR_FACE_NV21_OPEN_FAILED = -7,          // NV21 文件打开失败
    ERR_FACE_NV21_SIZE_MISMATCH = -8,        // NV21 文件大小与宽高不匹配
    ERR_FACE_RESIZE_FRAME_ALLOC_FAILED = -9, // 640x640 检测帧分配失败
    ERR_FACE_RESIZE_FAILED = -10,            // 图片缩放失败
    ERR_FACE_DETECT_TASK_FAILED = -11,       // YOLO 检测任务执行失败
    ERR_FACE_DETECT_TIMEOUT = -12,           // YOLO 检测任务超时
    ERR_FACE_FEATURE_EXTRACT_FAILED = -13,   // ArcFace 特征提取失败
    ERR_FACE_DATABASE_WRITE_FAILED = -14,    // 名单库数据库写入失败
    ERR_FACE_MODEL_START_FAILED = -15,       // YOLO/ArcFace 工作线程或模型启动失败
    ERR_FACE_ALGO_CREATE_FAILED = -16,       // 人脸算法实例不可用
    ERR_AI_SUSPEND_FAILED = -17,             // 暂停其他算法失败
    ERR_AI_RESTORE_FAILED = -18,             // 人脸入库后恢复原算法失败
    /* 预留接口 enrollFromLiveTrack 未实现时的返回（Legacy 无对应值，无上游调用方） */
    ERR_FACE_LIVE_ENROLL_UNSUPPORTED = -19
};
#endif

/* 人脸名单库服务（比对/入库统一出口） */
class IFaceLibraryService
{
public:
    virtual ~IFaceLibraryService() = default;

    /*
     * 上传图入库：入参预填 BinPath（NV21 文件路径）/PicWidth/PicHeight 与
     * 人员元数据，内部完成 检测→单人脸校验→特征提取→查重→落库，
     * 返回 FaceLibAddErrorCode（本文件定义，数值与 Legacy 一致）。
     */
    virtual int enrollFromImage(FaceDataDB_NS::FaceLibsInfo_S &stFaceLibData) = 0;

    /*
     * 实时流取优入库【预留】：以跟踪 trackId 为会话键，从该目标的 best 帧
     * 提取特征入库（bestshot_hub 订阅者）；会话结束（轨迹回收/超时/达到
     * 帧数上限）自动提交或取消。触发入口产品化另定，本期仅定义契约。
     */
    virtual int enrollFromLiveTrack(int nTrackId, const std::string &strFaceLibName, const std::string &strName) = 0;

    /* 特征比对名单库：输出最佳匹配人员 id 与相似度（阈值判定由调用方） */
    virtual bool compareFeature(const std::vector<float> &vfFeature, int &nFaceId, float &fSimilarity) = 0;

    /* 按 id 查人员详情 */
    virtual int queryById(int nFaceId, FaceDataDB_NS::FaceLibsInfo_S &stFaceInfo) = 0;
};

#if CAP_AI_FACE_COMPARE
/*
 * 名单库服务实现（无成员状态，可按栈实例使用）：
 * - enrollFromImage：自建临时轻量 Worker（低内存互斥模式）编排单图建模；
 * - enrollFromLiveTrack：预留未实现，恒返回 ERR_FACE_LIVE_ENROLL_UNSUPPORTED；
 * - compareFeature/queryById：转发 AIFaceManage 单例。
 */
class CFaceLibraryService : public IFaceLibraryService
{
public:
    int enrollFromImage(FaceDataDB_NS::FaceLibsInfo_S &stFaceLibData) override;

    int enrollFromLiveTrack(int nTrackId, const std::string &strFaceLibName, const std::string &strName) override;

    bool compareFeature(const std::vector<float> &vfFeature, int &nFaceId, float &fSimilarity) override;

    int queryById(int nFaceId, FaceDataDB_NS::FaceLibsInfo_S &stFaceInfo) override;
};
#endif
} // namespace FaceChain_NS
