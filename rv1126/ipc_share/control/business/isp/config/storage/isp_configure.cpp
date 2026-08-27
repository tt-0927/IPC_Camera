/**
 * @FilePath     : isp_configure.cpp
 * @Author       : cyc
 * @Date         : 2025-07-17 10:56:42
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-07-15 15:54:11
 * @Description  : 图像配置
 */

#include "isp_configure.h"

#include <cstddef>

#include "IpcRet.h"

CIspConfigure::CIspConfigure()
    : m_picScene(PIC_SCENE_CONFIG_FILE), m_picMirror(PIC_MIRROR_CONFIG_FILE), m_picSchedule(IMAGE_SCHEDULE_CONFIG_FILE),
      m_picSceneParam(PIC_SCENEPARAM_CONFIG_FILE)
{
    ISP::AllSceneParams_S stAllParams;
    if (m_picSceneParam.get(stAllParams) != OK)
    {
        dlog_error("读取场景配置兼容快照失败");
        return;
    }

    const ISP::AllSceneParams_S stDefaults = ISP::AllSceneParams_S::CreateWithDefaultRule();
    bool bNeedMigration = false;
    const std::size_t nWebSceneCount = static_cast<std::size_t>(ISP::SCENE_MAX);
    if (stAllParams.aSceneParams.size() < nWebSceneCount)
    {
        const std::size_t nOldSize = stAllParams.aSceneParams.size();
        stAllParams.aSceneParams.resize(nWebSceneCount);
        for (std::size_t nIndex = nOldSize; nIndex < nWebSceneCount; ++nIndex)
        {
            stAllParams.aSceneParams[nIndex] = stDefaults.aSceneParams[nIndex];
        }
        bNeedMigration = true;
    }
    else if (stAllParams.aSceneParams.size() > nWebSceneCount)
    {
        /* 旧版末尾两个夜间槽位属于内部运行态误建模，迁移时直接丢弃。 */
        stAllParams.aSceneParams.resize(nWebSceneCount);
        bNeedMigration = true;
    }

    for (std::size_t nIndex = 0; nIndex < stAllParams.aSceneParams.size(); ++nIndex)
    {
        const ISP::SceneType_E enExpectedScene = static_cast<ISP::SceneType_E>(nIndex);
        if (stAllParams.aSceneParams[nIndex].enSceneType != enExpectedScene)
        {
            stAllParams.aSceneParams[nIndex].enSceneType = enExpectedScene;
            bNeedMigration = true;
        }
    }

    if (stAllParams.enCurrentScene < ISP::SCENE_NORMAL || stAllParams.enCurrentScene >= ISP::SCENE_MAX)
    {
        dlog_warn("旧配置包含非法网页场景索引:%d, 回退到普通场景",
                  static_cast<int>(stAllParams.enCurrentScene));
        stAllParams.enCurrentScene = ISP::SCENE_NORMAL;
        bNeedMigration = true;
    }

    if (bNeedMigration)
    {
        /* info: 六个网页场景的枚举值和已有参数保持不变，仅清理历史内部夜间槽位。 */
        const int nRet = m_picSceneParam.set(stAllParams);
        if (nRet != OK)
        {
            dlog_error("持久化网页六场景兼容迁移失败: %d", nRet);
        }
    }
}

CIspConfigure::~CIspConfigure()
{
}

int CIspConfigure::set_configure(const ISP::SceneType_E &data)
{
    /* 全量场景参数快照用于只改写当前场景索引，保留其他场景的独立配置。 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_picSceneParam.get(stAllParams);
    if (nRet != OK)
    {
        dlog_error("m_picSceneParam get error");
        return nRet;
    }
    if (stAllParams.enCurrentScene >= ISP::SCENE_NORMAL && stAllParams.enCurrentScene < ISP::SCENE_MAX &&
        stAllParams.enCurrentScene < stAllParams.aSceneParams.size())
    {
        /* 校验当前索引有效后才更新选择，避免损坏的持久化数据导致越界写入。 */
        stAllParams.enCurrentScene = data;
        return m_picSceneParam.set(stAllParams);
    }
    return ERR;
}

int CIspConfigure::get_configure(ISP::SceneType_E &data) const
{
    /* 从全量快照读取当前场景，先校验索引与场景数组的一致性。 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_picSceneParam.get(stAllParams);
    if (nRet != OK)
    {
        dlog_error("m_picSceneParam get error");
        return nRet;
    }

    if (stAllParams.enCurrentScene >= ISP::SCENE_NORMAL && stAllParams.enCurrentScene < ISP::SCENE_MAX &&
        stAllParams.enCurrentScene < stAllParams.aSceneParams.size())
    {
        data = stAllParams.enCurrentScene;
        return OK;
    }

    return ERR;
}

int CIspConfigure::set_configure(const ISP::ImageParam_S &data)
{
    /* 图像参数属于当前场景槽位，必须读改写全量快照以保留其他场景数据。 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_picSceneParam.get(stAllParams);
    if (nRet != OK)
    {
        dlog_error("m_picSceneParam get error");
        return nRet;
    }

    if (stAllParams.enCurrentScene >= ISP::SCENE_NORMAL && stAllParams.enCurrentScene < ISP::SCENE_MAX &&
        stAllParams.enCurrentScene < stAllParams.aSceneParams.size())
    {
        stAllParams.aSceneParams[stAllParams.enCurrentScene].stImageParam = data;
        return m_picSceneParam.set(stAllParams);
    }

    return ERR;
}

int CIspConfigure::get_configure(ISP::ImageParam_S &data) const
{
    /* 读取当前场景的图像参数副本，调用方不会直接持有持久化内部对象。 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_picSceneParam.get(stAllParams);
    if (nRet != OK)
    {
        dlog_error("m_picSceneParam get error");
        return nRet;
    }

    if (stAllParams.enCurrentScene >= ISP::SCENE_NORMAL && stAllParams.enCurrentScene < ISP::SCENE_MAX &&
        stAllParams.enCurrentScene < stAllParams.aSceneParams.size())
    {
        data = stAllParams.aSceneParams[stAllParams.enCurrentScene].stImageParam;
        return OK;
    }

    return ERR;
}

int CIspConfigure::set_configure(const ISP::ExposureAttr_S &data)
{
    /* 曝光配置与当前场景绑定，保存时保留同一快照中的其他参数类别。 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_picSceneParam.get(stAllParams);
    if (nRet != OK)
    {
        dlog_error("m_picSceneParam get error");
        return nRet;
    }

    if (stAllParams.enCurrentScene >= ISP::SCENE_NORMAL && stAllParams.enCurrentScene < ISP::SCENE_MAX &&
        stAllParams.enCurrentScene < stAllParams.aSceneParams.size())
    {
        stAllParams.aSceneParams[stAllParams.enCurrentScene].stExpAttr = data;
        return m_picSceneParam.set(stAllParams);
    }

    return ERR;
}

int CIspConfigure::get_configure(ISP::ExposureAttr_S &data) const
{
    /* 从经过索引校验的当前场景槽位复制曝光配置。 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_picSceneParam.get(stAllParams);
    if (nRet != OK)
    {
        dlog_error("m_picSceneParam get error");
        return nRet;
    }

    if (stAllParams.enCurrentScene >= ISP::SCENE_NORMAL && stAllParams.enCurrentScene < ISP::SCENE_MAX &&
        stAllParams.enCurrentScene < stAllParams.aSceneParams.size())
    {
        data = stAllParams.aSceneParams[stAllParams.enCurrentScene].stExpAttr;
        return OK;
    }

    return ERR;
}

int CIspConfigure::set_configure(const ISP::DayNightAttr_S &data)
{
    /* 日夜配置按当前场景持久化，便于场景切换后恢复各自的补光策略。 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_picSceneParam.get(stAllParams);
    if (nRet != OK)
    {
        dlog_error("m_picSceneParam get error");
        return nRet;
    }

    if (stAllParams.enCurrentScene >= ISP::SCENE_NORMAL && stAllParams.enCurrentScene < ISP::SCENE_MAX &&
        stAllParams.enCurrentScene < stAllParams.aSceneParams.size())
    {
        stAllParams.aSceneParams[stAllParams.enCurrentScene].stDayNightAttr = data;
        return m_picSceneParam.set(stAllParams);
    }

    return ERR;
}

int CIspConfigure::get_configure(ISP::DayNightAttr_S &data) const
{
    /* 日夜配置快照供策略和业务层读取，避免其直接访问存储实现。 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_picSceneParam.get(stAllParams);
    if (nRet != OK)
    {
        dlog_error("m_picSceneParam get error");
        return nRet;
    }

    if (stAllParams.enCurrentScene >= ISP::SCENE_NORMAL && stAllParams.enCurrentScene < ISP::SCENE_MAX &&
        stAllParams.enCurrentScene < stAllParams.aSceneParams.size())
    {
        data = stAllParams.aSceneParams[stAllParams.enCurrentScene].stDayNightAttr;
        return OK;
    }

    dlog_error("读取日夜配置失败, 当前场景索引:%d, 场景数量:%zu",
               static_cast<int>(stAllParams.enCurrentScene),
               stAllParams.aSceneParams.size());
    return ERR;
}

int CIspConfigure::set_configure(const ISP::BackLightArrt_S &data)
{
    /* 背光配置写入当前场景槽位，读改写避免覆盖曝光、白平衡等并列字段。 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_picSceneParam.get(stAllParams);
    if (nRet != OK)
    {
        dlog_error("m_picSceneParam get error");
        return nRet;
    }
    if (stAllParams.enCurrentScene >= ISP::SCENE_NORMAL && stAllParams.enCurrentScene < ISP::SCENE_MAX &&
        stAllParams.enCurrentScene < stAllParams.aSceneParams.size())
    {
        stAllParams.aSceneParams[stAllParams.enCurrentScene].stBackLightAttr = data;
        return m_picSceneParam.set(stAllParams);
    }

    return ERR;
}

int CIspConfigure::get_configure(ISP::BackLightArrt_S &data) const
{
    /* 读取当前场景的背光配置副本，索引无效时拒绝返回不确定数据。 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_picSceneParam.get(stAllParams);
    if (nRet != OK)
    {
        dlog_error("m_picSceneParam get error");
        return nRet;
    }

    if (stAllParams.enCurrentScene >= ISP::SCENE_NORMAL && stAllParams.enCurrentScene < ISP::SCENE_MAX &&
        stAllParams.enCurrentScene < stAllParams.aSceneParams.size())
    {
        data = stAllParams.aSceneParams[stAllParams.enCurrentScene].stBackLightAttr;
        return OK;
    }

    return ERR;
}

int CIspConfigure::set_configure(const ISP::AwbAttr_S &data)
{
    /* 白平衡配置与场景一起保存，保持用户选项在场景切换后可重放。 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_picSceneParam.get(stAllParams);
    if (nRet != OK)
    {
        dlog_error("m_picSceneParam get error");
        return nRet;
    }
    if (stAllParams.enCurrentScene >= ISP::SCENE_NORMAL && stAllParams.enCurrentScene < ISP::SCENE_MAX &&
        stAllParams.enCurrentScene < stAllParams.aSceneParams.size())
    {
        stAllParams.aSceneParams[stAllParams.enCurrentScene].stAwbAttr = data;
        return m_picSceneParam.set(stAllParams);
    }

    return ERR;
}

int CIspConfigure::get_configure(ISP::AwbAttr_S &data) const
{
    /* 使用全量快照读取当前场景白平衡，存储对象不会暴露可写引用。 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_picSceneParam.get(stAllParams);
    if (nRet != OK)
    {
        dlog_error("m_picSceneParam get error");
        return nRet;
    }

    if (stAllParams.enCurrentScene >= ISP::SCENE_NORMAL && stAllParams.enCurrentScene < ISP::SCENE_MAX &&
        stAllParams.enCurrentScene < stAllParams.aSceneParams.size())
    {
        data = stAllParams.aSceneParams[stAllParams.enCurrentScene].stAwbAttr;
        return OK;
    }

    return ERR;
}

int CIspConfigure::set_configure(const ISP::DnrAttr_S &data)
{
    /* 降噪配置同样按场景保存，避免参数重放时使用错误场景的强度。 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_picSceneParam.get(stAllParams);
    if (nRet != OK)
    {
        dlog_error("m_picSceneParam get error");
        return nRet;
    }
    if (stAllParams.enCurrentScene >= ISP::SCENE_NORMAL && stAllParams.enCurrentScene < ISP::SCENE_MAX &&
        stAllParams.enCurrentScene < stAllParams.aSceneParams.size())
    {
        stAllParams.aSceneParams[stAllParams.enCurrentScene].stDnrAttr = data;
        return m_picSceneParam.set(stAllParams);
    }

    return ERR;
}

int CIspConfigure::get_configure(ISP::DnrAttr_S &data) const
{
    /* 从有效的当前场景槽位复制降噪配置。 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_picSceneParam.get(stAllParams);
    if (nRet != OK)
    {
        dlog_error("m_picSceneParam get error");
        return nRet;
    }

    if (stAllParams.enCurrentScene >= ISP::SCENE_NORMAL && stAllParams.enCurrentScene < ISP::SCENE_MAX &&
        stAllParams.enCurrentScene < stAllParams.aSceneParams.size())
    {
        data = stAllParams.aSceneParams[stAllParams.enCurrentScene].stDnrAttr;
        return OK;
    }

    return ERR;
}

int CIspConfigure::set_configure(const ISP::VideoAdjust_S &data)
{
    /* 镜像配置跨场景共享，使用独立存储避免被场景参数重放覆盖。 */
    return m_picMirror.set(data);
}

int CIspConfigure::get_configure(ISP::VideoAdjust_S &data) const
{
    return m_picMirror.get(data);
}

int CIspConfigure::set_configure(const ISP::SceneSchedule_S &data)
{
    /* 场景计划为全局调度配置，不属于单个场景槽位。 */
    return m_picSchedule.set(data);
}

int CIspConfigure::get_configure(ISP::SceneSchedule_S &data) const
{
    return m_picSchedule.get(data);
}

int CIspConfigure::get_configure(ISP::AllSceneParams_S &data) const
{
    /* 返回全量配置副本，供需要一致性快照的调用方一次读取全部场景字段。 */
    return m_picSceneParam.get(data);
}

int CIspConfigure::set_configure(const ISP::AllSceneParams_S &data)
{
    /* 调用方已完成整体校验时才使用全量覆盖入口，普通参数更新应走对应重载。 */
    return m_picSceneParam.set(data);
}

int CIspConfigure::set_configure()
{
    /* 全量快照用于仅恢复当前场景默认值，避免影响其他场景自定义参数。 */
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_picSceneParam.get(stAllParams);
    if (nRet != OK)
    {
        dlog_error("m_picSceneParam get error");
        return nRet;
    }

    if (stAllParams.enCurrentScene >= ISP::SCENE_NORMAL && stAllParams.enCurrentScene < ISP::SCENE_MAX &&
        stAllParams.enCurrentScene < stAllParams.aSceneParams.size())
    {
        /* 先由模型恢复当前槽位默认值，再将完整快照一次性写回持久层。 */
        if (stAllParams.restoreSceneToDefault(stAllParams.enCurrentScene))
        {
            return m_picSceneParam.set(stAllParams);
        }
    }

    return ERR;
}
