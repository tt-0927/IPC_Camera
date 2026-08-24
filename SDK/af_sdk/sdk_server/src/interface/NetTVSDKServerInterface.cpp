

#include<stdio.h>
#include <stdint.h>
#include <cstring>
#include <memory>

#include "NetTVSDKServerInterface.h"
#include "NetTVSDKServerImpl.h"

/* VoiceCom/RecordFrame 的 C 接口实现已迁出至 BG6_ZHSJ/BU_SJCL/ 目录：
 *   NetTVVoiceComInterface.cpp      ← 语音对讲（直接调 CVoiceComServer 单例）
 *   NetTVRecordFrameInterface.cpp   ← 录像帧流（直接调 CRecordFrameServer 单例）
 * 它们是 BU_SJCL 独立业务，不经 PIMPL，对外声明仍在本头文件中。
 */

// 全局Impl单例（使用智能指针）
static std::unique_ptr<CNetTVSDKServerImpl> g_pServerImpl;

namespace
{
UINT32 ClampCaptureImageLength(UINT32 uLength)
{
    return uLength > NET_PIC_DATA_MAX_LEN ? NET_PIC_DATA_MAX_LEN : uLength;
}

void FillCaptureImage(NET_ImageBuffer_S& stImage, BYTE* pData, UINT32 uLength)
{
    stImage.pData = pData;
    stImage.uDataLen = ClampCaptureImageLength(uLength);
}

void CopyCaptureTimestamp(CHAR* strDst, const CHAR* strSrc)
{
    if (!strDst || !strSrc)
    {
        return;
    }
    std::memcpy(strDst, strSrc, NET_CAPTURE_TIMESTAMP_MAX_LEN);
    strDst[NET_CAPTURE_TIMESTAMP_MAX_LEN - 1] = '\0';
}

BOOL PushCompatibleCapture(NET_Alarmer_S* pAlarmer, NET_AlarmCaptureInfo_S& stCapture)
{
    /* NET_serverPushAlarmInfo 在当前调用中完成 JSON 序列化，因此图片指针只需在本函数返回前有效。 */
    return NET_serverPushAlarmInfo(pAlarmer,
                                   static_cast<INT32>(stCapture.uAlarmType),
                                   &stCapture,
                                   static_cast<INT32>(sizeof(stCapture)));
}
} // namespace

#ifdef __cplusplus
extern "C" {
#endif

NET_API BOOL STDCALL NET_serverInit(IN UINT32 udwPort,IN CHAR szUserName[NET_LEN_132],IN CHAR szPassword[NET_LEN_132],IN CHAR szDeviceName[NET_LEN_132])
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoInit(udwPort, szUserName, szPassword, szDeviceName);
}

NET_API BOOL STDCALL NET_serverCleanup(void)
{
	/* Platform workers must stop before callbacks and the main server are released. */
	NET_serverPlatformStop();
	if (g_pServerImpl)
	{
		BOOL ret = g_pServerImpl->DoCleanup();
		g_pServerImpl.reset();
		return ret;
	}
	return TRUE;
}

NET_API BOOL STDCALL NET_serverSetLogToFile(IN INT32 dwLogLevel,IN CHAR  *strLogDir,IN INT32 nLogFileSize,IN INT32 dwLogFileNum)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoSetLogToFile(dwLogLevel, strLogDir, nLogFileSize, dwLogFileNum);
}

NET_API INT32 STDCALL NET_serverGetSdkVersion(void)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoGetSDKVersion();
}

NET_API INT32 STDCALL NET_serverGetClientCount(void)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoGetClientCount();
}

NET_API BOOL STDCALL NET_serverSetUserPassword(IN CHAR szUserName[NET_LEN_132],IN CHAR szPassword[NET_LEN_132])
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoSetUserPasswd(szUserName, szPassword);
}

NET_API BOOL STDCALL NET_serverPushAlarmInfo(IN NET_Alarmer_S *pAlarmer,
                                                    IN INT32 lCommand,
                                                    IN LPVOID pAlarmInfo,
                                                    IN INT32 dwBufLen)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoPushAlarmInfo(pAlarmer, lCommand, pAlarmInfo, dwBufLen);
}

NET_API BOOL STDCALL NET_serverPushFaceCaptureInfo(IN NET_Alarmer_S* pAlarmer,
                                                    IN NET_FaceCapturePushInfo_S* pCaptureInfo)
{
    if (!pAlarmer || !pCaptureInfo)
    {
        return FALSE;
    }

    NET_AlarmCaptureInfo_S stCapture = {};
    stCapture.uAlarmType = NET_ALARM_CAPTURE_FACE;
    stCapture.uCaptureType = NET_CAPTURE_TYPE_FACE;
    FillCaptureImage(stCapture.stPanoramaImg, pCaptureInfo->byPanoramaImg, pCaptureInfo->uPanoramaImgLen);
    if (pCaptureInfo->uFaceImgLen > 0)
    {
        stCapture.uCropCount = 1;
        stCapture.stCropImages[0].uTargetType = 3; /* 与历史 SDK 的人脸目标类型保持一致。 */
        stCapture.stCropImages[0].nTrackID = -1;
        FillCaptureImage(stCapture.stCropImages[0].stImage, pCaptureInfo->byFaceImg, pCaptureInfo->uFaceImgLen);
    }
    stCapture.stExtraInfo.bMale = pCaptureInfo->bMale;
    stCapture.stExtraInfo.nAgeLabel = pCaptureInfo->nAgeLabel;
    stCapture.stExtraInfo.bGlasses = pCaptureInfo->bGlasses;
    stCapture.stExtraInfo.bBeard = pCaptureInfo->bBeard;
    stCapture.stExtraInfo.bMask = pCaptureInfo->bMask;
    stCapture.stExtraInfo.nEmotionLabel = pCaptureInfo->nEmotionLabel;
    stCapture.stExtraInfo.stTargetRegion = pCaptureInfo->stFaceRegion;
    CopyCaptureTimestamp(stCapture.stExtraInfo.strTimestamp, pCaptureInfo->strTimestamp);
    return PushCompatibleCapture(pAlarmer, stCapture);
}

NET_API BOOL STDCALL NET_serverPushPersonCaptureInfo(IN NET_Alarmer_S* pAlarmer,
                                                      IN NET_PersonCapturePushInfo_S* pCaptureInfo)
{
    if (!pAlarmer || !pCaptureInfo)
    {
        return FALSE;
    }

    NET_AlarmCaptureInfo_S stCapture = {};
    stCapture.uAlarmType = NET_ALARM_CAPTURE_PEOPLE;
    stCapture.uCaptureType = NET_CAPTURE_TYPE_PEOPLE;
    FillCaptureImage(stCapture.stPanoramaImg, pCaptureInfo->byPanoramaImg, pCaptureInfo->uPanoramaImgLen);
    if (pCaptureInfo->uPersonImgLen > 0)
    {
        stCapture.uCropCount = 1;
        stCapture.stCropImages[0].uTargetType = NET_CAPTURE_TYPE_PEOPLE;
        stCapture.stCropImages[0].nTrackID = -1;
        FillCaptureImage(stCapture.stCropImages[0].stImage, pCaptureInfo->byPersonImg, pCaptureInfo->uPersonImgLen);
    }
    stCapture.stExtraInfo.bMale = pCaptureInfo->bMale;
    stCapture.stExtraInfo.nAgeLabel = pCaptureInfo->nAgeLabel;
    stCapture.stExtraInfo.bBag = pCaptureInfo->bBag;
    stCapture.stExtraInfo.nTopColorLabel = pCaptureInfo->nTopColorLabel;
    stCapture.stExtraInfo.nBottomColorLabel = pCaptureInfo->nBottomColorLabel;
    CopyCaptureTimestamp(stCapture.stExtraInfo.strTimestamp, pCaptureInfo->strTimestamp);
    return PushCompatibleCapture(pAlarmer, stCapture);
}

NET_API BOOL STDCALL NET_serverPushMotorvehicleCaptureInfo(IN NET_Alarmer_S* pAlarmer,
                                                            IN NET_MotorvehicleCapturePushInfo_S* pCaptureInfo)
{
    if (!pAlarmer || !pCaptureInfo)
    {
        return FALSE;
    }

    NET_AlarmCaptureInfo_S stCapture = {};
    stCapture.uAlarmType = NET_ALARM_CAPTURE_VEHICLE;
    stCapture.uCaptureType = NET_CAPTURE_TYPE_VEHICLE;
    FillCaptureImage(stCapture.stPanoramaImg, pCaptureInfo->byPanoramaImg, pCaptureInfo->uPanoramaImgLen);
    if (pCaptureInfo->uTargetImgLen > 0)
    {
        stCapture.uCropCount = 1;
        stCapture.stCropImages[0].uTargetType = NET_CAPTURE_TYPE_VEHICLE;
        stCapture.stCropImages[0].nTrackID = -1;
        FillCaptureImage(stCapture.stCropImages[0].stImage, pCaptureInfo->byTargetImg, pCaptureInfo->uTargetImgLen);
    }
    stCapture.stExtraInfo.nVehicleType = pCaptureInfo->nVehicleType;
    stCapture.stExtraInfo.nVehicleColor = pCaptureInfo->nVehicleColor;
    std::memcpy(stCapture.stExtraInfo.strVehicleBrand,
                pCaptureInfo->strVehicleBrand,
                sizeof(stCapture.stExtraInfo.strVehicleBrand));
    std::memcpy(stCapture.stExtraInfo.strLicensePlateNumber,
                pCaptureInfo->strLicensePlateNumber,
                sizeof(stCapture.stExtraInfo.strLicensePlateNumber));
    stCapture.stExtraInfo.strVehicleBrand[sizeof(stCapture.stExtraInfo.strVehicleBrand) - 1] = '\0';
    stCapture.stExtraInfo.strLicensePlateNumber[sizeof(stCapture.stExtraInfo.strLicensePlateNumber) - 1] = '\0';
    CopyCaptureTimestamp(stCapture.stExtraInfo.strTimestamp, pCaptureInfo->strTimestamp);
    return PushCompatibleCapture(pAlarmer, stCapture);
}

NET_API BOOL STDCALL NET_serverPushNonMotorvehicleCaptureInfo(IN NET_Alarmer_S* pAlarmer,
                                                               IN NET_NonMotorvehicleCapturePushInfo_S* pCaptureInfo)
{
    if (!pAlarmer || !pCaptureInfo)
    {
        return FALSE;
    }

    NET_AlarmCaptureInfo_S stCapture = {};
    stCapture.uAlarmType = NET_ALARM_CAPTURE_NON_MOTOR;
    stCapture.uCaptureType = NET_CAPTURE_TYPE_NON_MOTOR;
    FillCaptureImage(stCapture.stPanoramaImg, pCaptureInfo->byPanoramaImg, pCaptureInfo->uPanoramaImgLen);
    if (pCaptureInfo->uTargetImgLen > 0)
    {
        stCapture.uCropCount = 1;
        stCapture.stCropImages[0].uTargetType = NET_CAPTURE_TYPE_NON_MOTOR;
        stCapture.stCropImages[0].nTrackID = -1;
        FillCaptureImage(stCapture.stCropImages[0].stImage, pCaptureInfo->byTargetImg, pCaptureInfo->uTargetImgLen);
    }
    stCapture.stExtraInfo.nVehicleType = pCaptureInfo->nVehicleType;
    stCapture.stExtraInfo.nVehicleColor = pCaptureInfo->nVehicleColor;
    CopyCaptureTimestamp(stCapture.stExtraInfo.strTimestamp, pCaptureInfo->strTimestamp);
    return PushCompatibleCapture(pAlarmer, stCapture);
}

NET_API BOOL STDCALL NET_serverPushChannelStatusInfo(IN NET_ChannelInfo_S *pChannelInfo)
{
	if (!g_pServerImpl)
	{
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoPushChannelStatusInfo(pChannelInfo);
}

NET_API BOOL STDCALL
NET_serverRegisterGetDiscoveryDeviceInfoCb(
    IN NET_CB_GetDiscoveryDeviceInfo cbFunc)
{
	if (!g_pServerImpl) {
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoRegisterCb_GetDiscoveryDeviceInfo(cbFunc);
}

NET_API BOOL STDCALL
NET_serverStartDiscovery(IN const CHAR* szInterfaceName)
{
	if (!g_pServerImpl) {
		g_pServerImpl = std::make_unique<CNetTVSDKServerImpl>();
	}
	return g_pServerImpl->DoDiscoveryStart(szInterfaceName);
}

NET_API BOOL STDCALL
NET_serverStopDiscovery(void)
{
	if (!g_pServerImpl) {
		return FALSE;
	}
	return g_pServerImpl->DoDiscoveryStop();
}


#ifdef __cplusplus
}
#endif
