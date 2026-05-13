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
int NetSDK_ExecuteCb_GetReplayUrl(LPNET_TV_REPLAY_URL_INFO_S pInfo);
int NetSDK_ExecuteCb_ControlReplay(LPNET_TV_REPLAY_CTRL_INFO_S pInfo);
int NetSDK_ExecuteCb_GetReplayRecordList(LPNET_TV_REPLAY_RECORD_LIST_S pInfo);

#ifdef __cplusplus
}
#endif

#endif

