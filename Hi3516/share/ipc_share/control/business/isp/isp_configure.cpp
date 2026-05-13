/**
 * @FilePath     : isp_configure.cpp
 * @Author       : cyc
 * @Date         : 2025-07-17 10:56:42
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-21 16:13:57
 * @Description  : 图像配置
 */

#include "isp_configure.h"
#include "IpcRet.h"

CIspConfigure::CIspConfigure()
    : m_picScene(PIC_SCENE_CONFIG_FILE), m_picMirror(PIC_MIRROR_CONFIG_FILE), m_picSchedule(IMAGE_SCHEDULE_CONFIG_FILE),
      m_picSceneParam(PIC_SCENEPARAM_CONFIG_FILE)
{
}

CIspConfigure::~CIspConfigure()
{
}

int CIspConfigure::set_configure(const ISP::SceneType_E& data)
{
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
        stAllParams.enCurrentScene = data;
        return m_picSceneParam.set(stAllParams);
    }
    return ERR;
}

int CIspConfigure::get_configure(ISP::SceneType_E& data) const
{
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

int CIspConfigure::set_configure(const ISP::ImageParam_S& data)
{
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

int CIspConfigure::get_configure(ISP::ImageParam_S& data) const
{
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

int CIspConfigure::set_configure(const ISP::ExposureAttr_S& data)
{
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

int CIspConfigure::get_configure(ISP::ExposureAttr_S& data) const
{
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

int CIspConfigure::set_configure(const ISP::DayNightAttr_S& data)
{
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

int CIspConfigure::get_configure(ISP::DayNightAttr_S& data) const
{
    ISP::AllSceneParams_S stAllParams;
    int nRet = m_picSceneParam.get(stAllParams);
    if (nRet != OK)
    {
        dlog_error("m_picSceneParam get error");
        return nRet;
    }

    if (stAllParams.enCurrentScene >= ISP::SCENE_NORMAL && stAllParams.enCurrentScene < ISP::SCENE_MAX)
    {
        data = stAllParams.aSceneParams[stAllParams.enCurrentScene].stDayNightAttr;
        return OK;
    }

    return ERR;
}

int CIspConfigure::set_configure(const ISP::BackLightArrt_S& data)
{
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

int CIspConfigure::get_configure(ISP::BackLightArrt_S& data) const
{
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

int CIspConfigure::set_configure(const ISP::AwbAttr_S& data)
{
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

int CIspConfigure::get_configure(ISP::AwbAttr_S& data) const
{
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

int CIspConfigure::set_configure(const ISP::DnrAttr_S& data)
{
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

int CIspConfigure::get_configure(ISP::DnrAttr_S& data) const
{
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

int CIspConfigure::set_configure(const ISP::VideoAdjust_S& data)
{
    return m_picMirror.set(data);
}

int CIspConfigure::get_configure(ISP::VideoAdjust_S& data) const
{
    return m_picMirror.get(data);
}

int CIspConfigure::set_configure(const ISP::SceneSchedule_S& data)
{
    return m_picSchedule.set(data);
}

int CIspConfigure::get_configure(ISP::SceneSchedule_S& data) const
{
    return m_picSchedule.get(data);
}

int CIspConfigure::get_configure(ISP::AllSceneParams_S& data) const
{
    return m_picSceneParam.get(data);
}

int CIspConfigure::set_configure(const ISP::AllSceneParams_S& data)
{
    return m_picSceneParam.set(data);
}

int CIspConfigure::set_configure()
{
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
        if (stAllParams.restoreSceneToDefault(stAllParams.enCurrentScene))
        {
            return m_picSceneParam.set(stAllParams);
        }
    }

    return ERR;
}
