/**
 * @FilePath     : classroom_gateway.h
 * @Author       : leiyy
 * @Date         : 2026-10-19 17:04:19
 * @LastEditors  : leiyy
 * @LastEditTime : 2026-10-19 17:10:12
 * @Description  : 课堂分析对外网关
 */

#pragma once

#include <memory>
#include <mutex>
#include <string>

#include "algorithm.hpp"
#include "classroom_engine.h"

namespace Classroom
{
/**
 * @brief   : 课堂分析网关（由 CAlgoStreamDeal 唯一持有一个实例）
 */
class CClassroomGateway : public CAlgorithm
{
public:
    /**
     * @brief   : 构造即装配（装模型、起节拍线程）
     */
    CClassroomGateway();

    /* 析构即停止：先收引擎（含 join 节拍线程）再放管线，顺序理由见 m_spPerceive 的声明处 */
    ~CClassroomGateway() override;

    /**
     * @brief   : 收一帧媒体数据（框架的算法入口，取流线程同步调用）
     * @param    {MediaData_S} stMediaData：CStreamHandler 已经为该帧拷过一份私有内存，
     *            其中 pData 是 shared_ptr，函数返回后即失效——所以本函数只做"塞进总线"这一件事
     */
    void recvMediaData(MediaData_S stMediaData) override;

    bool isReady() const;

    /**
     * @brief   : 送一帧 NV12（非阻塞）
     * @param    {const void*} pucNv12：像素首地址，函数返回后即可被调用方回收
     * @param    {int64_t} llLen：可用字节数
     * @param    {int} nWidth / nHeight：该帧实际宽高
     * @param    {uint64_t} u64PtsUs：推流 PTS
     * @return   {bool} true 已入帧总线；false 本帧被丢（未就绪 / 尺寸不符 / 无空闲槽）
     */
    bool onFrame(const void *pucNv12, int64_t llLen, int nWidth, int nHeight, uint64_t u64PtsUs);

    /**
     * @brief   : 文本下发配置（平台命令通道落地后走这里）
     * @param    {const std::string&} strJson：完整配置 JSON
     * @param    {std::string&} rstrResp：OUT=回显文本（成功带新版本号，失败带第一条硬错误）
     * @return   {bool} false 表示校验不过，旧配置继续生效
     */
    bool onConfigText(const std::string &strJson, std::string &rstrResp);

    /**
     * @brief   : 指定文件路径重载配置（与引擎的 mtime 看护同一条 reload）
     */
    bool onConfigFile(const std::string &strPath, std::string &rstrResp);

    /**
     * @brief   : 当前状态（JSON 文本）
     */
    std::string statusJson() const;

    /* 把 statusJson 落到日志：装配完成/失败、配置换入时各打一次，事后查现场有据可依 */
    void dumpStatus(const char *pReason);

    /* 给同层（egress 等）取引擎出口用；未装配成功返回 nullptr，调用方必须判空 */
    CClassroomEngine *engine() const;

#if defined(CAP_TV3881TJY_CLASSROOM_CONFIG) && CAP_TV3881TJY_CLASSROOM_CONFIG
    /**
     * @brief   : 进程级唯一实例（配置下发接口用）
     * @return   {CClassroomGateway*}  nullptr 表示课堂网关还没建起来（装配失败/未启动）
     */
    static CClassroomGateway *instance();

    /**
     * @brief   : 取某一功能页的当前生效配置（配置下发用）
     * @param    {const char*} pszFeature：功能页名（见 classroom_config_adapter.h 的表）
     * @param    {std::string&} rstrResp：OUT=JSON（只含本页那几个键的当前生效值）
     * @return   {bool} false 表示功能页名不认识或课堂未就绪
     */
    bool readConfigPage(const char *pszFeature, std::string &rstrResp);

    /**
     * @brief   : 按功能页合入配置（配置下发用）
     * @param    {const char*} pszFeature：功能页名
     * @param    {const std::string&} strPartial：网页送来的部分 JSON
     * @param    {std::string&} rstrResp：OUT=回显（成功带新版本号，失败带原因）
     * @return   {bool} false 表示合并/校验失败，**旧配置继续生效**
     */
    bool configPageSet(const char *pszFeature, const std::string &strPartial, std::string &rstrResp);

#endif

private:
    /**
     * @brief   : 装配并启动课堂引擎（构造函数调用）
     * @return   {bool} false 表示课堂功能未起来（已打 ERROR），整机其余算法能力不受影响
     */
    bool init();

    /* 停止并释放（析构函数调用），与 init() 严格配对，可重复调用 */
    void deinit();

    /**
     * @brief   : 换入一份已合并的配置 + 落盘 + 生成回执（单页 Set 与区域表 Set 共用）
     */
    bool applyMergedText(const std::string &strMerged, const char *pszTitle, std::string &rstrResp);

    mutable std::mutex m_mtx;
    /* 统计出口 */
    std::unique_ptr<IStatsSink> m_spStatsSink;
    /* 感知管线（P1 起）：引擎的节拍线程每拍调它一次，因此它必须比引擎活得久。
       析构顺序是声明的逆序，故这一行放在 m_spEngine **之前**；deinit() 里的显式释放顺序同理。
       两条路径都按这个次序走，就不会出现「引擎还在跑、管线的宿主已经没了」 */
    std::unique_ptr<CPerceivePipeline> m_spPerceive;
    std::unique_ptr<CClassroomEngine> m_spEngine;
};
}
