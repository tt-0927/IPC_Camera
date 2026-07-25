/*
 * @Author       : EasonLu
 * @Date         : 2025-02-12 16:46:35
 * @LastEditors  : EasonLu
 * @LastEditTime : 2025-04-30 17:27:21
 * @FilePath     : SipModule.cpp
 * @Description  : SIP模块管理
 */
#include "SipModule.h"
#include "DeviceManage.h"
#include "MethodRequest.h"
#include "RequestPool.h"
#include "SSRC_Config.h"
#include "RtpServer.h"
#include "dlog.h"
#include "gm.h"

using namespace SIP;

SipModule *SipModule::m_pInstance = nullptr;
std::mutex SipModule::m_mtx;

int SIP::SipModule::Init(CbInfo_S stInfo)
{
    if (m_bInit)
    {
        return 0;
    }
    m_bInit = true;
    // DeviceManage::instance()->Init();
    // DeviceManage::instance()->Start();
    m_stCbInfo = stInfo;

    CRtpServer::instance()->rtpServer_init();
    return 0;
}

int SIP::SipModule::Deinit()
{
    CRtpServer::instance()->rtpServer_deinit();
    // DeviceManage::instance()->Stop();
    m_bInit = false;
    return 0;
}

void SIP::SipModule::StartServer(SipServerInfo_S stInfo)
{
    /* TCP和UDP服务器都启动 */
    stInfo.bTcp = false;
    if (m_stUdpSipServer.IsRunning())
    {
        m_stUdpSipServer.Stop();
    }
    m_stUdpSipServer.Init(stInfo);
    m_stUdpSipServer.Start();
    m_stUdpSipServer.UpdateCbInfo(m_stCbInfo);

    stInfo.bTcp = true;
    if (m_stTcpSipServer.IsRunning())
    {
        m_stTcpSipServer.Stop();
    }
    m_stTcpSipServer.Init(stInfo);
    m_stTcpSipServer.Start();
    m_stTcpSipServer.UpdateCbInfo(m_stCbInfo);

    if (stInfo.strID.size() > 8)
    {
        /* 用ID的第4~8位更新SSRC的第2~6位 */
        SSRCConfig::instance()->SetServerPrefix(stInfo.strID.substr(3, 5));
    }
}

void SIP::SipModule::StopServer()
{
    if (m_stTcpSipServer.IsRunning())
    {
        m_stTcpSipServer.Stop();
    }

    if (m_stUdpSipServer.IsRunning())
    {
        m_stUdpSipServer.Stop();
    }
}

void SipModule::StartClient(SipClientInfo_S stInfo, bool bEnableGm)
{
    if (m_stClient.IsRunning())
    {
        m_stClient.Stop();
    }
    /* 设置国标35114使能状态 */
    CGm::instance()->setGmEnable(bEnableGm);
    m_stClient.Init(stInfo);
    m_stClient.Start();
    m_stClient.UpdateCbInfo(m_stCbInfo);
}

void SipModule::StopClient()
{
    if (m_stClient.IsRunning())
    {
        m_stClient.Stop();
    }
}

void SipModule::RebootClient()
{

    if (m_bIsStarting.exchange(true)) 
    {

        printf("SIP客户端正在启动中，请稍后...\n");
        return;
    }
   
    m_startThread = std::thread([this]() 
    {
        m_bIsStarting  = true;
        if (m_stClient.IsRunning())
        {
            m_stClient.Stop();
        }
        m_stClient.Start();

        m_bIsStarting = false; 
    });
    
    m_startThread.detach();
}

const SipServerInfo_S &SIP::SipModule::GetSipServerInfo(eXosip_t *pCtx)
{
    if (nullptr == pCtx)
    {
        return m_stEmptyInfo;
    }

    if (pCtx == m_stTcpSipServer.GetSipContext())
    {
        return m_stTcpSipServer.GetSipServerInfo();
    }
    else if (pCtx == m_stUdpSipServer.GetSipContext())
    {
        return m_stUdpSipServer.GetSipServerInfo();
    }
    else if (pCtx == m_stClient.GetSipContext())
    {
        return m_stClient.GetSipServerInfo();
    }
    return m_stEmptyInfo;
}

const std::string SIP::SipModule::GetSipUriByContext(eXosip_t *pCtx)
{
    auto &stInfo = GetSipServerInfo(pCtx);
    return stInfo.GetUri();
}

int SIP::SipModule::GetDevInfo(std::vector<SipDeviceInfo_S> &vecDevInfo)
{
    return DeviceManage::instance()->GetDeviceList(vecDevInfo);
}

int SIP::SipModule::GetLocalInfo(SipLocalInfo_S &stInfo)
{
    /* 默认给空数据 */
    stInfo = SipLocalInfo_S();
    if (nullptr != m_stCbInfo.fnGetLocalInfo)
    {
        m_stCbInfo.fnGetLocalInfo(stInfo);
    }
    return 0;
}

int SIP::SipModule::PtzCtrl(const SipDeviceInfo_S &device, SipPtzType_E enCmd, int nSpeed)
{
    auto pDevice = DeviceManage::instance()->GetDevice(device.strID);
    if (nullptr == pDevice)
    {
        dlog_warn("设备[%s]不存在，无法控制", device.strID.c_str());
        return -1;
    }

    /* 调用指定通道ID的Ptz控制 */
    return pDevice->PtzCtrl(device.strChnID, enCmd, nSpeed);
}

int SIP::SipModule::PresetCtrl(const SipDeviceInfo_S &device, SipPresetType_E enCmd, unsigned int nPresetID)
{
    auto pDevice = DeviceManage::instance()->GetDevice(device.strID);
    if (nullptr == pDevice)
    {
        dlog_warn("设备[%s]不存在，无法控制", device.strID.c_str());
        return -1;
    }
    return pDevice->PresetCtrl(device.strChnID, enCmd, nPresetID);
}

int SIP::SipModule::AlarmSubscribe(const SipDeviceInfo_S &device, const GB28181::AlarmSubscribe_S &stInfo)
{
    auto pDevice = DeviceManage::instance()->GetDevice(device.strID);
    if (nullptr == pDevice)
    {
        dlog_warn("设备[%s]不存在，无法订阅报警信息", device.strID.c_str());
        return -1;
    }
    return pDevice->AlarmSubscribe(stInfo);
}

int SIP::SipModule::Guard(const SipDeviceInfo_S &device, bool bSetGuard)
{
    auto pDevice = DeviceManage::instance()->GetDevice(device.strID);
    if (nullptr == pDevice)
    {
        dlog_warn("设备[%s]不存在，无法布防", device.strID.c_str());
        return -1;
    }
    return pDevice->Guard(bSetGuard);
}

int SIP::SipModule::Play(const SipDeviceInfo_S &device, bool bStart)
{
    auto pDevice = DeviceManage::instance()->GetDevice(device.strID);
    if (nullptr == pDevice)
    {
        dlog_warn("设备[%s]不存在，无法播放", device.strID.c_str());
        return -1;
    }
    auto pChannel = pDevice->GetChannelByType(SipChannelType_E::NETWORK_CAMERA_IPC_ENCODE);
    if (nullptr == pChannel)
    {
        dlog_warn("设备[%s]视频编码通道不存在，无法播放", device.strID.c_str());
        return -2;
    }
    dlog_info("播放设备[%s]视频通道 状态[%d]",
              device.strID.c_str(), bStart);
    return pDevice->PlayVideo(bStart);
}

int SIP::SipModule::SendPlayMedia(
    SipDeviceInfo_S &device,
    char *pBuf,
    int nLen,
    bool bIsAudio)
{
    return m_stClient.SendMedia(device, pBuf, nLen, bIsAudio);
}

int SIP::SipModule::SendReadFileMedia(
    const std::string &strCallID,
    char *pBuf,
    int nLen,
    bool bIsAudio)
{
    return m_stClient.SendMedia(strCallID, pBuf, nLen, bIsAudio);
}

int SIP::SipModule::SetClientCodec(SipDeviceInfo_S &device)
{
    return m_stClient.SetCodec(device);
}

int SIP::SipModule::SendAlarmInfo(GB28181::AlarmInfo_S &stInfo)
{
    return m_stClient.SendAlarmInfo(stInfo);
}

int SIP::SipModule::UpdateChnInfo(std::vector<SipChannelInfo_S> &vecChnInfo)
{
    return m_stClient.UpdateChnInfo(vecChnInfo);
}

int SIP::SipModule::SendUploadSnapShotFiniInfo(GB28181::UploadSnapShotFiniInfo_S &stInfo)
{
    return m_stClient.SendUploadSnapShotFiniInfo(stInfo);
}

bool SIP::SipModule::get_client_status()
{
    return m_stClient.get_client_status();
}
