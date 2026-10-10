/**
 * @FilePath     : osd_configure.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-09-24 09:30:00
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-09-24 10:20:00
 * @Description  : OSD/Cover配置转换
 */

#pragma once

#include <atomic>
#include <cstddef>
#include <mutex>
#include <string>
#include <vector>
#include "IpcRet.h"
#include "Singleton.h"
#include "osd_define.h"
#include "system_define.h"

/* OSD名称长度限制 */
#define OSD_NAME_LENGTH_LIMIT 100

/**
 * @brief   : OSD配置应用接口
 * @note    : 由业务仓库实现，ipc_share 只依赖该抽象接口，
 *            避免直接依赖业务仓库的OSD渲染模块（反向依赖）；
 *            overplay布局映射、颜色格式等平台差异在该接口内消化
 */
class IOsdConfigApplier
{
public:
    virtual ~IOsdConfigApplier()
    {
    }

    /**
     * @brief   : 获取当前Overplay布局（作为OSD配置转换的模板）
     * @param    {std::vector<Osd::OverplayInfo_S>} &vecInfo：Overplay布局
     * @return   {IpcRet_E} OK：成功，非OK：失败
     */
    virtual IpcRet_E get_overplay_info(std::vector<Osd::OverplayInfo_S> &vecInfo) = 0;

    /**
     * @brief   : 应用Overplay布局（更新渲染）
     * @param    {std::vector<Osd::OverplayInfo_S>} &vecInfo：Overplay布局
     * @return   {IpcRet_E} OK：成功，非OK：失败
     */
    virtual IpcRet_E apply_overplay_info(const std::vector<Osd::OverplayInfo_S> &vecInfo) = 0;

    /**
     * @brief   : 获取当前Cover区域布局（作为Cover配置转换的模板）
     * @param    {std::vector<Osd::CoverInfo_S>} &vecInfo：Cover区域布局
     * @return   {IpcRet_E} OK：成功，非OK：失败
     */
    virtual IpcRet_E get_cover_info(std::vector<Osd::CoverInfo_S> &vecInfo) = 0;

    /**
     * @brief   : 应用Cover区域布局（更新渲染）
     * @param    {std::vector<Osd::CoverInfo_S>} &vecInfo：Cover区域布局
     * @return   {IpcRet_E} OK：成功，非OK：失败
     */
    virtual IpcRet_E apply_cover_info(const std::vector<Osd::CoverInfo_S> &vecInfo) = 0;

    /**
     * @brief   : 获取当前平台实际支持的隐私遮盖区域数量
     * @return   {std::size_t} 区域数量
     * @note    : 网页、ONVIF 和 TVSDK 应以该数量收敛配置，避免返回底层无法渲染的区域
     */
    virtual std::size_t get_cover_max_area_count() const = 0;

    /**
     * @brief   : OSD状态信息转换为Overplay渲染参数（平台差异钩子）
     * @param    {Osd::ElementType_E} enType：元素种类（宽度自适应等按类型处理的平台逻辑使用）
     * @param    {std::string} &strText：元素文本（字符叠加/通道名称内容）
     * @param    {Osd::OsdAttribute_S} &stOsdAttr：OSD状态信息（平台可就地修正，如nW自适应后随配置持久化）
     * @param    {Osd::Overplay_S} &stOverplay：Overplay渲染参数
     * @return   {IpcRet_E} OK：成功，非OK：失败
     * @note    : 位置余量/宽高透传、颜色串格式（RGB/ARGB）、"黑白自动"等平台差异在此实现
     */
    virtual IpcRet_E adapt_osd_attr(Osd::ElementType_E enType,
                                    const std::string &strText,
                                    Osd::OsdAttribute_S &stOsdAttr,
                                    Osd::Overplay_S &stOverplay) = 0;

    /**
     * @brief   : Cover配置单区域映射为Cover渲染信息（平台差异钩子）
     * @param    {Osd::CoverAttribute_S} &stAttr：Cover区域配置
     * @param    {Osd::CoverInfo_S} &stInfo：Cover渲染信息（bEnable/strName已由公共流程填充）
     * @return   {IpcRet_E} OK：成功，非OK：失败
     * @note    : 颜色串格式（RGB/ARGB）、负坐标区域禁用等平台策略在此实现
     */
    virtual IpcRet_E cover_attr_to_info(const Osd::CoverAttribute_S &stAttr, Osd::CoverInfo_S &stInfo) = 0;

    /**
     * @brief   : 通知OSD共用信息更新，触发Overplay渲染刷新
     * @return   {void}
     */
    virtual void refresh_overplay() = 0;
};

class COsdConfigure : public CSingleton<COsdConfigure>
{
    COsdConfigure();

public:
    ~COsdConfigure();
    /* 允许 Singleton 访问私有构造函数 */
    friend class CSingleton<COsdConfigure>;

    /**
     * @brief   : 设置OSD配置应用接口
     * @param    {IOsdConfigApplier *} pApplier：业务仓库OSD配置应用接口
     * @return   {IpcRet_E} OK：成功，非OK：失败
     */
    IpcRet_E setOsdConfigApplier(IOsdConfigApplier *pApplier);

    /**
     * @brief   : 清理OSD配置应用接口
     * @param    {IOsdConfigApplier *} pApplier：需要清理的业务仓库OSD配置应用接口
     * @return   {IpcRet_E} OK：成功，非OK：失败
     */
    IpcRet_E clearOsdConfigApplier(IOsdConfigApplier *pApplier);

    /***
     * @description : 初始化（加载OSD/Cover配置文件）
     * @return       {IpcRet_E} OK：成功，非OK：失败
     * @note         : 需先注册 IOsdConfigApplier（区域数量收敛依赖平台能力）
     */
    IpcRet_E init();

    /**
     * @brief   : OSD配置模块是否已初始化完成
     * @return   {bool} true：已初始化，false：未初始化
     */
    bool is_initialized() const;

    /***
     * @description : 获取OSD配置
     * @param        {Osd::OsdConfig_S} &stInfo：OSD配置
     * @return       {IpcRet_E} OK：成功，非OK：失败
     */
    IpcRet_E get_osd_config(Osd::OsdConfig_S &stInfo);

    /***
     * @description : 设置OSD配置（转换为Overplay布局后经应用接口下发渲染）
     * @param        {Osd::OsdConfig_S} stInfo：OSD配置
     * @return       {IpcRet_E} OK：成功，非OK：失败
     */
    IpcRet_E set_osd_config(Osd::OsdConfig_S stInfo);

    /***
     * @description : 获取Cover配置
     * @param        {Osd::CoverConfig_S} &stInfo：Cover配置
     * @return       {IpcRet_E} OK：成功，非OK：失败
     */
    IpcRet_E get_cover_config(Osd::CoverConfig_S &stInfo);

    /***
     * @description : 设置Cover配置（转换为Cover区域布局后经应用接口下发渲染）
     * @param        {Osd::CoverConfig_S} stInfo：Cover配置
     * @return       {IpcRet_E} OK：成功，非OK：失败
     */
    IpcRet_E set_cover_config(Osd::CoverConfig_S stInfo);

    /**
     * @brief   : 获取当前平台实际支持的隐私遮盖区域数量
     * @return   {std::size_t} 区域数量
     */
    std::size_t get_cover_max_area_count() const;

    /***
     * @description : 获取当前Overplay布局
     * @param        {std::vector<Osd::OverplayInfo_S>} &vecInfo：Overplay布局
     * @return       {IpcRet_E} OK：成功，非OK：失败
     */
    IpcRet_E get_overplay_info(std::vector<Osd::OverplayInfo_S> &vecInfo);

    /***
     * @description : 设置当前Overplay布局（经应用接口更新渲染）
     * @param        {std::vector<Osd::OverplayInfo_S>} &vecInfo：Overplay布局
     * @return       {IpcRet_E} OK：成功，非OK：失败
     */
    IpcRet_E set_overplay_info(const std::vector<Osd::OverplayInfo_S> &vecInfo);

    /***
     * @description : 获取OSD共用信息（时间/IP/提示语等，按OSD配置的时间格式组织）
     * @param        {Osd::ShareInfo_S} &stuShareInfo：OSD共用信息
     * @return       {IpcRet_E} OK：成功，非OK：失败
     */
    IpcRet_E get_osd_share_info(Osd::ShareInfo_S &stuShareInfo);

    /***
     * @description : 设置OSD共用信息（设备配置变化后刷新时间/IP/提示语并触发渲染）
     * @param        {System::DeviceConfig_S} stDeviceConfig：设备配置
     * @return       {IpcRet_E} OK：成功，非OK：失败
     */
    IpcRet_E set_osd_share_info(System::DeviceConfig_S stDeviceConfig);

private:
    /**
     * @brief   : Cover配置区域数量收敛到平台能力
     * @param    {Osd::CoverConfig_S} &stConfig：Cover配置
     * @return   {bool} true：配置被修改，false：配置未变化
     */
    bool normalize_cover_config(Osd::CoverConfig_S &stConfig) const;

    /* OSD配置应用接口 */
    IOsdConfigApplier *m_pApplier;
    std::string m_strOsdConfigFile;     /* osd配置文件 */
    std::string m_strCoverConfigFile;   /* cover配置文件 */
    Osd::OsdConfig_S m_stOsdConfig;     /* osd配置信息 */
    Osd::CoverConfig_S m_stCoverConfig; /* cover配置信息 */
    Osd::ShareInfo_S m_stuShareInfo;    /* osd共用元素信息 */
    std::atomic<bool> m_bInit;          /* 初始化标志 */
    std::mutex m_mutex;                 /* 配置缓存锁 */
};
