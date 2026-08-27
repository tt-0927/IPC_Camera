/*
 * @Author: 梁浩尧 lianghaoyao@kfb.cn
 * @Date: 2025-08-11 10:46:54
 * @LastEditors: 梁浩尧 lianghaoyao@kfb.cn
 * @LastEditTime: 2025-12-25 09:58:02
 * @FilePath: /hisi/share/ipc_share/common/convert/storage_convert.cpp
 * @Description: 存储管理数据的转换
 */
#include "storage_manage_convert.h"
#include "convert.h" /* 这个要放在 storage_convert.h 的后面 */

void Convert::deal(Json::Object* pRootJson, StorageManage_NS::StorageManage_S &StorageManageParam, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);

	convert.field(pRootJson, "IsEnable", StorageManageParam.bEnable);
	convert.field(pRootJson, "AvailableSpace", StorageManageParam.strAvailableSpace);

	convert.field(pRootJson, "CaptureSpace", StorageManageParam.strCaptureSpace);
	convert.field(pRootJson, "CaptureRemainingSpace", StorageManageParam.strCaptureRemainingSpace);

	convert.field(pRootJson, "RecordSpace", StorageManageParam.strRecordSpace);
	convert.field(pRootJson, "RecordRemainingSpace", StorageManageParam.strRecordRemainingSpace);

	convert.field(pRootJson, "CaptureQuotaPercentage", StorageManageParam.nCaptureQuotaPercentage);
	convert.field(pRootJson, "RecordQuotaPercentage", StorageManageParam.nRecordQuotaPercentage);

	convert.field(pRootJson, "StorageTime", StorageManageParam.nStorageTime);

	// convert.field(pRootJson, "SdCardUuid", StorageManageParam.strSdCardUuid);
}

void Convert::deal(Json::Object* pRootJson, StorageManage_NS::SdCardStatus_S &stSdCardStatus, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }

    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "Status", stSdCardStatus.nStatus);
    convert.field(pRootJson, "StatusText", stSdCardStatus.strStatusText);
    convert.field(pRootJson, "Ready", stSdCardStatus.bReady);
}

void Convert::deal(Json::Object* pRootJson, StorageManage_NS::DirInfo_S &stDirInfo, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);

	convert.field(pRootJson, "FileCount", stDirInfo.nFileCount);
	convert.field(pRootJson, "DirTotalSize", stDirInfo.llDirTotalSize);
}

void Convert::deal(Json::Object* pRootJson, bool &bIsInitSdCard, bool bOutStruct)
{
	if (!pRootJson)
	{
		return;
	}

	Convert::CConvert convert(bOutStruct);

	convert.field(pRootJson, "IsInitSdCard", bIsInitSdCard);
}
