/**
 * @file RecordInfoConvert.cpp
 * @author ITC
 * @date 2026-08-20
 * @LastEditors  : ITC
 * @LastEditTime : 2026-08-20
 *
 * @brief 录播部门（BU_SJLB）专用结构体 JSON 转换实现
 * 功能说明：
 * 1. 实现注册信息的结构体与 JSON 双向转换
 * 2. 各种功能的struct与json转换，返回
 */

#include "RecordInfoConvert.h"
#include "SDKConvert.h"

#include <cstdio>
#include <cstdlib>

void SDKConvert::deal(Json::Object* pRootJson, NET_RegisterInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "MachinSn", stInfo.strMachinSn);
    convert.field(pRootJson, "RegisterEg", stInfo.strRegisterEg);
    convert.field(pRootJson, "StartTime", stInfo.strStartTime);
    convert.field(pRootJson, "UsableTime", stInfo.nUsableTimer);
    convert.field(pRootJson, "ActionTime", stInfo.enActionTime);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordControlInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Status", stInfo.nStatus);
    convert.field(pRootJson, "RecordMode", stInfo.nRecordMode);
    convert.field(pRootJson, "Name", stInfo.szName);
    convert.field(pRootJson, "FileName", stInfo.szFileName);
    convert.field(pRootJson, "MainTeacher", stInfo.szMainTeacher);
    convert.field(pRootJson, "RoomName", stInfo.szRoomName);
    convert.field(pRootJson, "Notes", stInfo.szNotes);
    convert.field(pRootJson, "RecordTime", stInfo.nRecordTime);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_LiveStatusInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);

    /* SET 时前端传 EnableStream；GET 时返回 Status。两者共用一个 nStatus 字段。 */
    if (bOutStruct)
    {
        Json::get(pRootJson, "EnableStream", stInfo.nStatus);
    }
    else
    {
        Json::add(pRootJson, "Status", stInfo.nStatus);
    }

    convert.field(pRootJson, "RtmpName", stInfo.szRtmpName);
    convert.field(pRootJson, "RtmpTime", stInfo.nRtmpTime);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordFileItem_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "FileName", stInfo.szFileName);
    convert.field(pRootJson, "GroupID", stInfo.nGroupId);
    convert.field(pRootJson, "GroupName", stInfo.szGroupName);
    convert.field(pRootJson, "StartTime", stInfo.szStartTime);
    convert.field(pRootJson, "DurationTime", stInfo.szDurationTime);
    convert.field(pRootJson, "FileSize", stInfo.nFileSize);
    convert.field(pRootJson, "FileFormat", stInfo.nFileFormat);
    convert.field(pRootJson, "Damaged", stInfo.nDamaged);
    convert.field(pRootJson, "CourseName", stInfo.szCourseName);
    convert.field(pRootJson, "KeySpeaker", stInfo.szKeySpeaker);
    convert.field(pRootJson, "Location", stInfo.szLocation);
    convert.field(pRootJson, "Notes", stInfo.szNotes);
    convert.field(pRootJson, "FtpUpload", stInfo.nFtpUpload);
    convert.field(pRootJson, "PlatformUpload", stInfo.nPlatformUpload);
    convert.field(pRootJson, "DownloadCnt", stInfo.nDownloadCnt);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_RecordFileInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "CurPage", stInfo.nCurPage);
    convert.field(pRootJson, "CurPageSize", stInfo.nCurPageSize);
    convert.field(pRootJson, "PageSize", stInfo.nPageSize);
    convert.field(pRootJson, "Total", stInfo.nTotal);
    convert.field(pRootJson, "CoverPath", stInfo.szCoverPath);

    if (bOutStruct)
    {
        /* JSON -> 结构体 */
        Json::Object* pArray = Json::get(pRootJson, "FileInfo");
        int nSize = pArray ? Json::Array::size(pArray) : 0;
        int nCount = nSize;
        if (nCount > NET_RECORD_FILE_ITEM_MAX)
        {
            nCount = NET_RECORD_FILE_ITEM_MAX;
        }
        for (int i = 0; pArray && i < nCount; ++i)
        {
            Json::Object* pItem = Json::Array::get(pArray, i);
            if (pItem)
            {
                deal(pItem, stInfo.astFileItems[i], bOutStruct);
            }
        }
        stInfo.nFileCount = nCount;
    }
    else
    {
        /* 结构体 -> JSON */
        Json::Object* pArray = Json::Array::init();
        if (pArray)
        {
            int nCount = stInfo.nFileCount;
            if (nCount > NET_RECORD_FILE_ITEM_MAX)
            {
                nCount = NET_RECORD_FILE_ITEM_MAX;
            }
            for (int i = 0; i < nCount; ++i)
            {
                Json::Object* pItem = Json::init();
                deal(pItem, stInfo.astFileItems[i], bOutStruct);
                Json::Array::add(pArray, pItem);
            }
            Json::add(pRootJson, "FileInfo", pArray);
        }
    }
}

void SDKConvert::deal(Json::Object* pRootJson, NET_DirectorModeInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Mode", stInfo.nMode);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_CameraControlInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "ID",    stInfo.nId);
    convert.field(pRootJson, "Type",  stInfo.nType);
    convert.field(pRootJson, "Speed", stInfo.nSpeed);
    convert.field(pRootJson, "Num",   stInfo.nNum);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_PresetBitItem_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "CameraId",  stInfo.nCameraId);
    convert.field(pRootJson, "PresetNum", stInfo.nPresetNum);
    convert.field(pRootJson, "Name",      stInfo.szName);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_PresetBitInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Total", stInfo.nTotal);

    if (bOutStruct)
    {
        /* JSON→Struct */
        Json::Object* pArray = Json::get(pRootJson, "Arraylist");
        if (pArray)
        {
            int nCount = Json::Array::size(pArray);
            if (nCount > NET_PRESET_BIT_MAX) nCount = NET_PRESET_BIT_MAX;
            for (int i = 0; i < nCount; ++i)
            {
                 Json::Object* pItem = Json::Array::get(pArray, i);
                deal(pItem, stInfo.astItems[i], true);
            }
            stInfo.nTotal = nCount;
        }
    }
    else
    {
        /* Struct→JSON */
        Json::Object* pArray = Json::Array::init();
        int nCount = stInfo.nTotal;
        if (nCount > NET_PRESET_BIT_MAX) nCount = NET_PRESET_BIT_MAX;
        for (int i = 0; i < nCount; ++i)
        {
            Json::Object* pItem = Json::init();
            deal(pItem, stInfo.astItems[i], false);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "Arraylist", pArray);
    }
}

void SDKConvert::deal(Json::Object* pRootJson, NET_PresetBitCtrl_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Type",      stInfo.nOptType);
    convert.field(pRootJson, "CameraId",  stInfo.nCameraId);
    convert.field(pRootJson, "PresetNum", stInfo.nPresetNum);
    convert.field(pRootJson, "Name",      stInfo.szName);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_ExternalControlInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Cmdcode", stInfo.nCmdCode);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_LayoutRect_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "X",       stInfo.nX);
    convert.field(pRootJson, "Y",       stInfo.nY);
    convert.field(pRootJson, "W",       stInfo.nW);
    convert.field(pRootJson, "H",       stInfo.nH);
    convert.field(pRootJson, "Channel", stInfo.nChannel);
    convert.field(pRootJson, "UserID",  stInfo.nUserID);
    convert.field(pRootJson, "Myself",  stInfo.nMyself);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_LayoutSelfInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "LayoutNum",  stInfo.nNum);
    convert.field(pRootJson, "MovieMode",  stInfo.nMovieMode);
    convert.field(pRootJson, "MPlayout",   stInfo.nMplayout);
    convert.field(pRootJson, "ChannelPip", stInfo.nChannelPip);
    convert.field(pRootJson, "CtrlType",   stInfo.enCtrlType);

    if (bOutStruct)
    {
        /* JSON→Struct */
        Json::Object* pArray = Json::get(pRootJson, "Arraylist");
        if (pArray)
        {
            int nCount = Json::Array::size(pArray);
            if (nCount > NET_LAYOUT_RECT_MAX) nCount = NET_LAYOUT_RECT_MAX;
            for (int i = 0; i < nCount; ++i)
            {
                Json::Object* pItem = Json::Array::get(pArray, i);
                deal(pItem, stInfo.astRect[i], true);
            }
            stInfo.nNum = nCount;
        }
    }
    else
    {
        /* Struct→JSON */
        Json::Object* pArray = Json::Array::init();
        int nCount = stInfo.nNum;
        if (nCount > NET_LAYOUT_RECT_MAX) nCount = NET_LAYOUT_RECT_MAX;
        for (int i = 0; i < nCount; ++i)
        {
            Json::Object* pItem = Json::init();
            deal(pItem, stInfo.astRect[i], false);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "Arraylist", pArray);
    }
}

void SDKConvert::deal(Json::Object* pRootJson, NET_PVW2PGMInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Mode", stInfo.nMode);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_AppointmentItem_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "ID",          stInfo.nID);
    convert.field(pRootJson, "WeekDay",     stInfo.nWeekDay);
    convert.field(pRootJson, "Name",        stInfo.szName);
    convert.field(pRootJson, "StartTime",   stInfo.szStartTime);
    convert.field(pRootJson, "StopTime",    stInfo.szStopTime);
    convert.field(pRootJson, "IsCir",       stInfo.nIsCir);
    convert.field(pRootJson, "TeacherName", stInfo.szTeacherName);
    convert.field(pRootJson, "RoomName",    stInfo.szRoomName);
    convert.field(pRootJson, "Notes",       stInfo.szNotes);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_AppointmentInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Total", stInfo.nTotal);

    if (bOutStruct)
    {
        /* JSON→Struct */
        Json::Object* pArray = Json::get(pRootJson, "Arraylist");
        if (pArray)
        {
            int nCount = Json::Array::size(pArray);
            if (nCount > NET_APPOINTMENT_MAX) nCount = NET_APPOINTMENT_MAX;
            for (int i = 0; i < nCount; ++i)
            {
                Json::Object* pItem = Json::Array::get(pArray, i);
                deal(pItem, stInfo.astItems[i], true);
            }
            stInfo.nTotal = nCount;
        }
    }
    else
    {
        /* Struct→JSON */
        Json::Object* pArray = Json::Array::init();
        int nCount = stInfo.nTotal;
        if (nCount > NET_APPOINTMENT_MAX) nCount = NET_APPOINTMENT_MAX;
        for (int i = 0; i < nCount; ++i)
        {
            Json::Object* pItem = Json::init();
            deal(pItem, stInfo.astItems[i], false);
            Json::Array::add(pArray, pItem);
        }
        Json::add(pRootJson, "Arraylist", pArray);
    }
}

void SDKConvert::deal(Json::Object* pRootJson, NET_RebootInfo_S& stInfo, bool bOutStruct)
{
    (void)pRootJson;
    (void)stInfo;
    (void)bOutStruct;
    /* NET_CONTROL_REBOOT (535) 无需序列化字段 */
}

void SDKConvert::deal(Json::Object* pRootJson, NET_OutVolume_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Volume", stInfo.nVolume);
}

void SDKConvert::deal(Json::Object* pRootJson, NET_SshSafeInfo_S& stInfo, bool bOutStruct)
{
    if (!pRootJson) return;
    SDKConvert::CSDKConvert convert(bOutStruct);
    convert.field(pRootJson, "Status",        stInfo.nStatus);
    convert.field(pRootJson, "StartTime",     stInfo.nStartTime);
    convert.field(pRootJson, "TimeRemaining", stInfo.nTimeRemain);
}