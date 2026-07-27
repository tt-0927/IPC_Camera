/**
 * @file NetTVConfigCbExecute.h
 * @brief Device config callback execute declarations
 */
#ifndef _NETTVCONFIGCBEXECUTE_H
#define _NETTVCONFIGCBEXECUTE_H

#include "NetTVSDKServerInterface.h"

#ifdef __cplusplus
extern "C" {
#endif

int NetSDK_ExecuteCb_GetDevConfig(INT32 dwChannelID, INT32 dwCommand, LPVOID lpOutBuffer);
int NetSDK_ExecuteCb_SetDevConfig(INT32 dwChannelID, INT32 dwCommand, LPVOID lpInBuffer);
int NetSDK_ExecuteCb_GetReplayUrl(pNET_ReplayUrlInfo_S pInfo);
int NetSDK_ExecuteCb_ControlReplay(pNET_ReplayCtrlInfo_S pInfo);
int NetSDK_ExecuteCb_GetReplayRecordList(pNET_ReplayRecordList_S pInfo);

#ifdef __cplusplus
}
#endif

#endif

