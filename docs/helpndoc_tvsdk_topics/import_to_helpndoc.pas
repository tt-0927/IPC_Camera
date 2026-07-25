// HelpNDoc Pascal Script
// 在 HelpNDoc 中打开目标 .hnd 工程后，运行本脚本。
// 作用：自动创建接口回调、接口明细和接口参数结构体 topic，并导入对应 HTML 内容。

var
  RootTopic: string;
var
  StructTopic: string;
var
  CallbackTopic: string;
var
  NewTopic: string;
var
  Editor: TObject;

begin
  Editor := HndEditor.CreateTemporaryEditor();
  RootTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(RootTopic, 'SDK server接口定义');
  HndTopics.SetTopicHelpId(RootTopic, 'SDK_server');

  CallbackTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(CallbackTopic, '接口回调');
  HndTopics.SetTopicHelpId(CallbackTopic, 'Interface_Callback');
  HndTopics.MoveTopic(CallbackTopic, RootTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\接口回调.html');
  HndEditor.SetAsTopicContent(Editor, CallbackTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_Cleanup');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_Cleanup');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_Cleanup.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_Discovery_Start');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_Discovery_Start');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_Discovery_Start.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_Discovery_Stop');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_Discovery_Stop');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_Discovery_Stop.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_GetClientCount');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_GetClientCount');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_GetClientCount.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_GetSDKVersion');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_GetSDKVersion');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_GetSDKVersion.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_GetVoiceComAudioParam');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_GetVoiceComAudioParam');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_GetVoiceComAudioParam.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_Init');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_Init');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_Init.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_PushAlarmInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_PushAlarmInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_PushAlarmInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_PushChannelStatusInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_PushChannelStatusInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_PushChannelStatusInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_AddFaceInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_AddFaceInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_AddFaceInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_AddTargetLib');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_AddTargetLib');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_AddTargetLib.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_ConnectWifiSta');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_ConnectWifiSta');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_ConnectWifiSta.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_ControlRecordInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_ControlRecordInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_ControlRecordInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_ControlReplay');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_ControlReplay');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_ControlReplay.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_DelFaceInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_DelFaceInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_DelFaceInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_DelTargetLib');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_DelTargetLib');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_DelTargetLib.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_DisconnectWifiSta');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_DisconnectWifiSta');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_DisconnectWifiSta.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_DownloadRecordFile');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_DownloadRecordFile');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_DownloadRecordFile.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_ExportLog');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_ExportLog');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_ExportLog.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_FindLog');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_FindLog');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_FindLog.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_FindRecordFileInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_FindRecordFileInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_FindRecordFileInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_Get4GInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_Get4GInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_Get4GInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetAudioAnomalyAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetAudioAnomalyAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetAudioAnomalyAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetAudioCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetAudioCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetAudioCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetAudioEncodeCap');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetAudioEncodeCap');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetAudioEncodeCap.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetBackLightInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetBackLightInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetBackLightInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetBareSoilCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetBareSoilCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetBareSoilCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetCaptureParamInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetCaptureParamInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetCaptureParamInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetCapturePlanInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetCapturePlanInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetCapturePlanInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetChannelInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetChannelInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetChannelInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetChannelList');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetChannelList');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetChannelList.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetClimbFenceInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetClimbFenceInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetClimbFenceInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetCongestionCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetCongestionCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetCongestionCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetConstructionOccupyRoadCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetConstructionOccupyRoadCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetConstructionOccupyRoadCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetCrossLineAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetCrossLineAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetCrossLineAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetCrowGatheringAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetCrowGatheringAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetCrowGatheringAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetDayNightInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetDayNightInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetDayNightInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetDenoiseInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetDenoiseInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetDenoiseInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetDevConfig');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetDevConfig');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetDevConfig.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetDeviceCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetDeviceCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetDeviceCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetDeviceInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetDeviceInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetDeviceInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetDimissionInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetDimissionInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetDimissionInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetDiscoveryDeviceInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetDiscoveryDeviceInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetDiscoveryDeviceInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetElectricVehicleInElevatorCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetElectricVehicleInElevatorCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetElectricVehicleInElevatorCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetEnterRegionAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetEnterRegionAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetEnterRegionAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetExposureInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetExposureInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetExposureInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetFaceCaptureInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetFaceCaptureInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetFaceCaptureInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetFaceInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetFaceInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetFaceInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetGarbageExposureCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetGarbageExposureCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetGarbageExposureCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetGarbageOverflowCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetGarbageOverflowCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetGarbageOverflowCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetHighAltitudeSeatbeltCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetHighAltitudeSeatbeltCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetHighAltitudeSeatbeltCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetHoleProtectionBarCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetHoleProtectionBarCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetHoleProtectionBarCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetHotspotConn');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetHotspotConn');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetHotspotConn.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetIllegalLaneInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetIllegalLaneInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetIllegalLaneInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetImageCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetImageCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetImageCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetIntrusionAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetIntrusionAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetIntrusionAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetLeaveRegionAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetLeaveRegionAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetLeaveRegionAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetLicensePlateRecognitionCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetLicensePlateRecognitionCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetLicensePlateRecognitionCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetLogServer');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetLogServer');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetLogServer.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetLoiteringAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetLoiteringAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetLoiteringAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetManholeCoverAbnormalCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetManholeCoverAbnormalCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetManholeCoverAbnormalCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetMotionAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetMotionAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetMotionAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetNetworkCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetNetworkCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetNetworkCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetNonmotorVehicleIntrusionInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetNonmotorVehicleIntrusionInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetNonmotorVehicleIntrusionInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetNtpCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetNtpCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetNtpCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetObjectRemovalAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetObjectRemovalAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetObjectRemovalAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetOccupationEmergencyInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetOccupationEmergencyInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetOccupationEmergencyInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetOpenFlameCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetOpenFlameCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetOpenFlameCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetOsdCap');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetOsdCap');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetOsdCap.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetOsdCapCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetOsdCapCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetOsdCapCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetParkingAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetParkingAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetParkingAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPedestrianIntrusionInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPedestrianIntrusionInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetPedestrianIntrusionInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPeopleDensityDetectionCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPeopleDensityDetectionCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetPeopleDensityDetectionCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPeopleFlowStatisticsCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPeopleFlowStatisticsCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetPeopleFlowStatisticsCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPersonFallCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPersonFallCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetPersonFallCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPersonFallDownCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPersonFallDownCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetPersonFallDownCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPetRecognitionInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPetRecognitionInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetPetRecognitionInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPhoneUsageCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPhoneUsageCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetPhoneUsageCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPreviewInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPreviewInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetPreviewInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPrivacyMaskCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetPrivacyMaskCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetPrivacyMaskCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetRecordAdvancedParam');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetRecordAdvancedParam');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetRecordAdvancedParam.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetRecordSchedule');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetRecordSchedule');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetRecordSchedule.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetRecordStatus');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetRecordStatus');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetRecordStatus.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetReflectiveClothingCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetReflectiveClothingCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetReflectiveClothingCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetReplayRecordList');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetReplayRecordList');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetReplayRecordList.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetReplayUrl');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetReplayUrl');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetReplayUrl.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetRetrogradeInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetRetrogradeInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetRetrogradeInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetRoadPondingCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetRoadPondingCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetRoadPondingCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetRtspUrl');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetRtspUrl');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetRtspUrl.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetSafetyHelmetCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetSafetyHelmetCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetSafetyHelmetCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetSceneChangeAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetSceneChangeAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetSceneChangeAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetSecurityServicesInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetSecurityServicesInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetSecurityServicesInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetSleepOnDutyCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetSleepOnDutyCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetSleepOnDutyCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetSmokeFireCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetSmokeFireCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetSmokeFireCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetSmokingCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetSmokingCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetSmokingCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetSshCountdown');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetSshCountdown');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetSshCountdown.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetStreamCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetStreamCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetStreamCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetTalkbackFromStream');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetTalkbackFromStream');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetTalkbackFromStream.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetTamperAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetTamperAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetTamperAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetTargetLib');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetTargetLib');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetTargetLib.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetUnattendedObjectAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetUnattendedObjectAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetUnattendedObjectAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetUpgradeStatus');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetUpgradeStatus');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetUpgradeStatus.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetUpgradeVersion');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetUpgradeVersion');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetUpgradeVersion.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetVideoEncodeCap');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetVideoEncodeCap');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetVideoEncodeCap.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_GetWhiteBalanceInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_GetWhiteBalanceInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_GetWhiteBalanceInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_ResetPeopleFlowStatistics');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_ResetPeopleFlowStatistics');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_ResetPeopleFlowStatistics.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_Set4GInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_Set4GInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_Set4GInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetAudioAnomalyAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetAudioAnomalyAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetAudioAnomalyAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetAudioCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetAudioCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetAudioCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetBackLightInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetBackLightInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetBackLightInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetBareSoilCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetBareSoilCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetBareSoilCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetCaptureParamInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetCaptureParamInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetCaptureParamInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetCapturePlanInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetCapturePlanInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetCapturePlanInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetClimbFenceInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetClimbFenceInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetClimbFenceInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetConfigWifiSta');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetConfigWifiSta');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetConfigWifiSta.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetCongestionCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetCongestionCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetCongestionCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetConstructionOccupyRoadCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetConstructionOccupyRoadCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetConstructionOccupyRoadCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetCrossLineAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetCrossLineAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetCrossLineAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetCrowGatheringAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetCrowGatheringAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetCrowGatheringAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetDayNightInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetDayNightInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetDayNightInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetDenoiseInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetDenoiseInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetDenoiseInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetDevConfig');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetDevConfig');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetDevConfig.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetDeviceCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetDeviceCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetDeviceCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetDimissionInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetDimissionInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetDimissionInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetElectricVehicleInElevatorCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetElectricVehicleInElevatorCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetElectricVehicleInElevatorCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetEnterRegionAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetEnterRegionAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetEnterRegionAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetExposureInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetExposureInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetExposureInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetFaceCaptureInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetFaceCaptureInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetFaceCaptureInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetFaceCompareInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetFaceCompareInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetFaceCompareInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetFaceInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetFaceInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetFaceInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetGarbageExposureCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetGarbageExposureCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetGarbageExposureCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetGarbageOverflowCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetGarbageOverflowCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetGarbageOverflowCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetHighAltitudeSeatbeltCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetHighAltitudeSeatbeltCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetHighAltitudeSeatbeltCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetHoleProtectionBarCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetHoleProtectionBarCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetHoleProtectionBarCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetHotspotInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetHotspotInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetHotspotInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetIllegalLaneInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetIllegalLaneInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetIllegalLaneInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetImageCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetImageCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetImageCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetIntrusionAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetIntrusionAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetIntrusionAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetLeaveRegionAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetLeaveRegionAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetLeaveRegionAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetLicensePlateRecognitionCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetLicensePlateRecognitionCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetLicensePlateRecognitionCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetLogServer');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetLogServer');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetLogServer.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetLoiteringAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetLoiteringAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetLoiteringAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetManholeCoverAbnormalCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetManholeCoverAbnormalCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetManholeCoverAbnormalCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetMotionAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetMotionAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetMotionAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetNetworkCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetNetworkCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetNetworkCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetNonmotorVehicleIntrusionInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetNonmotorVehicleIntrusionInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetNonmotorVehicleIntrusionInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetNtpCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetNtpCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetNtpCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetObjectRemovalAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetObjectRemovalAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetObjectRemovalAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetOccupationEmergencyInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetOccupationEmergencyInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetOccupationEmergencyInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetOpenFlameCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetOpenFlameCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetOpenFlameCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetOsdCapCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetOsdCapCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetOsdCapCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetParkingAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetParkingAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetParkingAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPedestrianIntrusionInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPedestrianIntrusionInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetPedestrianIntrusionInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPeopleDensityDetectionCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPeopleDensityDetectionCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetPeopleDensityDetectionCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPeopleFlowStatisticsCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPeopleFlowStatisticsCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetPeopleFlowStatisticsCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPersonFallCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPersonFallCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetPersonFallCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPersonFallDownCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPersonFallDownCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetPersonFallDownCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPetRecognitionInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPetRecognitionInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetPetRecognitionInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPhoneUsageCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPhoneUsageCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetPhoneUsageCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPreviewInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPreviewInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetPreviewInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPrivacyMaskCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetPrivacyMaskCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetPrivacyMaskCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetRecordAdvancedParam');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetRecordAdvancedParam');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetRecordAdvancedParam.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetRecordSchedule');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetRecordSchedule');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetRecordSchedule.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetReflectiveClothingCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetReflectiveClothingCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetReflectiveClothingCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetReplayTalkback');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetReplayTalkback');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetReplayTalkback.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetRetrogradeInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetRetrogradeInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetRetrogradeInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetRoadPondingCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetRoadPondingCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetRoadPondingCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetSafetyHelmetCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetSafetyHelmetCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetSafetyHelmetCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetSceneChangeAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetSceneChangeAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetSceneChangeAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetSecurityServicesInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetSecurityServicesInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetSecurityServicesInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetSleepOnDutyCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetSleepOnDutyCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetSleepOnDutyCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetSmokeFireCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetSmokeFireCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetSmokeFireCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetSmokingCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetSmokingCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetSmokingCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetStreamCfg');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetStreamCfg');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetStreamCfg.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetTalkbackState');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetTalkbackState');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetTalkbackState.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetTalkbackToStream');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetTalkbackToStream');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetTalkbackToStream.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetTamperAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetTamperAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetTamperAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetTargetLib');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetTargetLib');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetTargetLib.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetUnattendedObjectAlarm');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetUnattendedObjectAlarm');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetUnattendedObjectAlarm.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetUpgrade');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetUpgrade');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetUpgrade.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_SetWhiteBalanceInfo');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_SetWhiteBalanceInfo');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_SetWhiteBalanceInfo.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_TestLogServer');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_TestLogServer');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_TestLogServer.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_VoiceComCapture');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_VoiceComCapture');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_VoiceComCapture.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_RegisterCb_VoiceComPlay');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_RegisterCb_VoiceComPlay');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_RegisterCb_VoiceComPlay.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_SendVoiceComData');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_SendVoiceComData');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_SendVoiceComData.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_SetLogToFile');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_SetLogToFile');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_SetLogToFile.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_SetUserPasswd');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_SetUserPasswd');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_SetUserPasswd.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_StartVoiceComServer');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_StartVoiceComServer');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_StartVoiceComServer.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SERVER_StopVoiceComServer');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SERVER_StopVoiceComServer');
  HndTopics.MoveTopic(NewTopic, CallbackTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SERVER_StopVoiceComServer.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  StructTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(StructTopic, '接口参数结构体');
  HndTopics.MoveTopic(StructTopic, RootTopic, htamAddChild);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_4G_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_4G_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_4G_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_AI_SIMPLE_RULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_AI_SIMPLE_RULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_AI_SIMPLE_RULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_AI_OBJECT_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_AI_OBJECT_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ALARM_AI_OBJECT_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_BASIC_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_BASIC_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ALARM_BASIC_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_EXCEPTION_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_EXCEPTION_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ALARM_EXCEPTION_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_FACE_COMPARE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_FACE_COMPARE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ALARM_FACE_COMPARE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_PLATE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_PLATE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ALARM_PLATE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_RULE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_RULE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ALARM_RULE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_SCHEDULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_SCHEDULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ALARM_SCHEDULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_STATISTICS_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_STATISTICS_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ALARM_STATISTICS_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_STATISTICS_TARGET_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_STATISTICS_TARGET_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ALARM_STATISTICS_TARGET_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARMER_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARMER_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ALARMER_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_AUDIO_ANOMALY_ALARM_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_AUDIO_ANOMALY_ALARM_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_AUDIO_ANOMALY_ALARM_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_AUDIO_ANOMALY_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_AUDIO_ANOMALY_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_AUDIO_ANOMALY_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_AUDIO_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_AUDIO_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_AUDIO_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_AUDIO_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_AUDIO_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_AUDIO_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_AUDIO_FORMAT_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_AUDIO_FORMAT_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_AUDIO_FORMAT_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_AUDIO_RANGE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_AUDIO_RANGE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_AUDIO_RANGE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_BACKLIGHT_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_BACKLIGHT_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_BACKLIGHT_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_BARE_SOIL_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_BARE_SOIL_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_BARE_SOIL_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_BEHAVIOR_DETECT_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_BEHAVIOR_DETECT_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_BEHAVIOR_DETECT_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_BOUNDARY_DETECT_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_BOUNDARY_DETECT_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_BOUNDARY_DETECT_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_BOUNDARY_PLANE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_BOUNDARY_PLANE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_BOUNDARY_PLANE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CAPTURE_CONFIG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CAPTURE_CONFIG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CAPTURE_CONFIG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CAPTURE_DAY_SCHEDULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CAPTURE_DAY_SCHEDULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CAPTURE_DAY_SCHEDULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CAPTURE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CAPTURE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CAPTURE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CAPTURE_PARAM_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CAPTURE_PARAM_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CAPTURE_PARAM_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CAPTURE_PLAN_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CAPTURE_PLAN_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CAPTURE_PLAN_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CAPTURE_TIME_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CAPTURE_TIME_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CAPTURE_TIME_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CHANNEL_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CHANNEL_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CHANNEL_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CHANNEL_LIST_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CHANNEL_LIST_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CHANNEL_LIST_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CLIMB_FENCE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CLIMB_FENCE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CLIMB_FENCE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CONGESTION_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CONGESTION_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CONGESTION_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CONGESTION_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CONGESTION_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CONGESTION_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CONSTRUCTION_OCCUPANCY_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CONSTRUCTION_OCCUPANCY_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CONSTRUCTION_OCCUPANCY_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CROSS_LINE_ALARM_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CROSS_LINE_ALARM_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CROSS_LINE_ALARM_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CROWD_GATHERING_ALARM_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CROWD_GATHERING_ALARM_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CROWD_GATHERING_ALARM_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CROWD_GATHERING_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CROWD_GATHERING_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CROWD_GATHERING_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CROWD_GATHERING_RULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CROWD_GATHERING_RULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_CROWD_GATHERING_RULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_DAYNIGHT_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_DAYNIGHT_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_DAYNIGHT_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_DENOISE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_DENOISE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_DENOISE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_DEVICE_BASICINFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_DEVICE_BASICINFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_DEVICE_BASICINFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_DEVICE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_DEVICE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_DEVICE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_DEVICE_LOGIN_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_DEVICE_LOGIN_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_DEVICE_LOGIN_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_DIMISSION_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_DIMISSION_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_DIMISSION_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_DISCOVERY_DEVICE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_DISCOVERY_DEVICE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_DISCOVERY_DEVICE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_EMERGENCY_LANE_OCCUPANCY_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_EMERGENCY_LANE_OCCUPANCY_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_EMERGENCY_LANE_OCCUPANCY_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ENTER_EXIT_DETECT_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ENTER_EXIT_DETECT_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ENTER_EXIT_DETECT_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ENTER_REGION_ALARM_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ENTER_REGION_ALARM_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ENTER_REGION_ALARM_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ENVIRONMENT_ANOMALY_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ENVIRONMENT_ANOMALY_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ENVIRONMENT_ANOMALY_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_EXPOSURE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_EXPOSURE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_EXPOSURE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_CAPTURE_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_CAPTURE_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_FACE_CAPTURE_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_CAPTURE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_CAPTURE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_FACE_CAPTURE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_CAPTURE_REGION_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_CAPTURE_REGION_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_FACE_CAPTURE_REGION_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_CAPTURE_RULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_CAPTURE_RULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_FACE_CAPTURE_RULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_COMPARE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_COMPARE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_FACE_COMPARE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_DETECT_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_DETECT_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_FACE_DETECT_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_ID_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_ID_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_FACE_ID_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_INFO_LIST_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_INFO_LIST_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_FACE_INFO_LIST_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_FACE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_LIB_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_LIB_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_FACE_LIB_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_LIB_LIST_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_LIB_LIST_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_FACE_LIB_LIST_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FIRE_DETECT_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FIRE_DETECT_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_FIRE_DETECT_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_GARBAGE_EXPOSURE_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_GARBAGE_EXPOSURE_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_GARBAGE_EXPOSURE_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_GARBAGE_EXPOSURE_RULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_GARBAGE_EXPOSURE_RULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_GARBAGE_EXPOSURE_RULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_GARBAGE_OVERFLOW_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_GARBAGE_OVERFLOW_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_GARBAGE_OVERFLOW_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_GARBAGE_OVERFLOW_RULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_GARBAGE_OVERFLOW_RULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_GARBAGE_OVERFLOW_RULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_HOLE_PROTECTION_BAR_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_HOLE_PROTECTION_BAR_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_HOLE_PROTECTION_BAR_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_HOTSPOT_CONN_DEVICE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_HOTSPOT_CONN_DEVICE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_HOTSPOT_CONN_DEVICE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_HOTSPOT_CONN_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_HOTSPOT_CONN_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_HOTSPOT_CONN_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_HOTSPOT_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_HOTSPOT_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_HOTSPOT_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ILLEGAL_LANE_CHANGE_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ILLEGAL_LANE_CHANGE_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ILLEGAL_LANE_CHANGE_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ILLEGAL_LANE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ILLEGAL_LANE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ILLEGAL_LANE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ILLEGAL_PARKING_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ILLEGAL_PARKING_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ILLEGAL_PARKING_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_IMAGE_SETTING_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_IMAGE_SETTING_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_IMAGE_SETTING_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_INTRUSION_ALARM_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_INTRUSION_ALARM_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_INTRUSION_ALARM_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_INTRUSION_DETECT_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_INTRUSION_DETECT_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_INTRUSION_DETECT_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_INTRUSION_RULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_INTRUSION_RULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_INTRUSION_RULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LEAVE_REGION_ALARM_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LEAVE_REGION_ALARM_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_LEAVE_REGION_ALARM_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LICENSE_PLATE_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LICENSE_PLATE_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_LICENSE_PLATE_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LINKAGE_LIST_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LINKAGE_LIST_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_LINKAGE_LIST_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LOG_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LOG_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_LOG_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LOG_LIST_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LOG_LIST_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_LOG_LIST_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LOG_RETRIEVAL_COND_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LOG_RETRIEVAL_COND_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_LOG_RETRIEVAL_COND_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LOG_SERVER_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LOG_SERVER_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_LOG_SERVER_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LOGIN_LOCK_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LOGIN_LOCK_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_LOGIN_LOCK_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LOITERING_ALARM_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LOITERING_ALARM_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_LOITERING_ALARM_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LOITERING_DETECT_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LOITERING_DETECT_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_LOITERING_DETECT_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LOITERING_RULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LOITERING_RULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_LOITERING_RULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_MOTION_ALARM_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_MOTION_ALARM_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_MOTION_ALARM_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_MOTION_DETECT_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_MOTION_DETECT_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_MOTION_DETECT_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_MOTION_EXPERT_MODE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_MOTION_EXPERT_MODE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_MOTION_EXPERT_MODE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_MOTION_NORMAL_MODE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_MOTION_NORMAL_MODE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_MOTION_NORMAL_MODE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_MOTION_REGION_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_MOTION_REGION_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_MOTION_REGION_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_NETWORKCFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_NETWORKCFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_NETWORKCFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_NON_MOTOR_VEHICLE_INTRUSION_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_NON_MOTOR_VEHICLE_INTRUSION_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_NON_MOTOR_VEHICLE_INTRUSION_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_OBJECT_CHANGE_DETECT_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_OBJECT_CHANGE_DETECT_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_OBJECT_CHANGE_DETECT_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_OBJECT_REMOVAL_ALARM_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_OBJECT_REMOVAL_ALARM_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_OBJECT_REMOVAL_ALARM_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_OBJECT_REMOVAL_RULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_OBJECT_REMOVAL_RULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_OBJECT_REMOVAL_RULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_OCCUPATION_EMERGENCY_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_OCCUPATION_EMERGENCY_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_OCCUPATION_EMERGENCY_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_OPEN_FLAME_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_OPEN_FLAME_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_OPEN_FLAME_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_OSD_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_OSD_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_OSD_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PAGE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PAGE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PAGE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PARKING_ALARM_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PARKING_ALARM_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PARKING_ALARM_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PARKING_DETECT_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PARKING_DETECT_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PARKING_DETECT_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PARKING_RULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PARKING_RULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PARKING_RULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PEDESTRIAN_INTRUSION_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PEDESTRIAN_INTRUSION_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PEDESTRIAN_INTRUSION_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PEOPLE_ALARM_CONFIG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PEOPLE_ALARM_CONFIG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PEOPLE_ALARM_CONFIG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PEOPLE_ALARM_RULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PEOPLE_ALARM_RULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PEOPLE_ALARM_RULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PEOPLE_FLOW_RULE_LINE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PEOPLE_FLOW_RULE_LINE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PEOPLE_FLOW_RULE_LINE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PERSON_FALL_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PERSON_FALL_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PERSON_FALL_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PERSON_FALL_DOWN_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PERSON_FALL_DOWN_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PERSON_FALL_DOWN_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PET_RECOGNITION_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PET_RECOGNITION_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PET_RECOGNITION_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PET_RECOGNITION_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PET_RECOGNITION_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PET_RECOGNITION_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PHONE_USAGE_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PHONE_USAGE_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PHONE_USAGE_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PREVIEW_IMAGE_PARAM_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PREVIEW_IMAGE_PARAM_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PREVIEW_IMAGE_PARAM_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PREVIEW_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PREVIEW_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PREVIEW_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PREVIEW_RTSP_URL_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PREVIEW_RTSP_URL_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PREVIEW_RTSP_URL_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PRIVACY_MASK_AREA_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PRIVACY_MASK_AREA_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PRIVACY_MASK_AREA_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PRIVACY_MASK_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PRIVACY_MASK_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PRIVACY_MASK_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PWD_POLICY_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PWD_POLICY_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_PWD_POLICY_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RANGE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RANGE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_RANGE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_ADVANCED_PARAM_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_ADVANCED_PARAM_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_RECORD_ADVANCED_PARAM_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_DAY_SCHEDULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_DAY_SCHEDULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_RECORD_DAY_SCHEDULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_DOWNLOAD_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_DOWNLOAD_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_RECORD_DOWNLOAD_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_DOWNLOAD_LIST_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_DOWNLOAD_LIST_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_RECORD_DOWNLOAD_LIST_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_DOWNLOAD_PROGRESS_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_DOWNLOAD_PROGRESS_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_RECORD_DOWNLOAD_PROGRESS_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_FILE_LIST_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_FILE_LIST_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_RECORD_FILE_LIST_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_FIND_COND_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_FIND_COND_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_RECORD_FIND_COND_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_FIND_RESULT_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_FIND_RESULT_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_RECORD_FIND_RESULT_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_RECORD_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_SCHEDULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_SCHEDULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_RECORD_SCHEDULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_STATUS_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_STATUS_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_RECORD_STATUS_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_TIME_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_TIME_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_RECORD_TIME_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_VIDEO_TIME_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_VIDEO_TIME_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_RECORD_VIDEO_TIME_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_REFLECTIVE_CLOTHING_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_REFLECTIVE_CLOTHING_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_REFLECTIVE_CLOTHING_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_REPLAY_CTRL_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_REPLAY_CTRL_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_REPLAY_CTRL_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_REPLAY_RECORD_LIST_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_REPLAY_RECORD_LIST_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_REPLAY_RECORD_LIST_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_REPLAY_RECORD_TIME_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_REPLAY_RECORD_TIME_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_REPLAY_RECORD_TIME_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_REPLAY_TALKBACK_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_REPLAY_TALKBACK_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_REPLAY_TALKBACK_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_REPLAY_URL_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_REPLAY_URL_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_REPLAY_URL_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RETROGRADE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RETROGRADE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_RETROGRADE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_REV_TIMEOUT_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_REV_TIMEOUT_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_REV_TIMEOUT_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ROAD_PONDING_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ROAD_PONDING_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_ROAD_PONDING_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RTSP_URL_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RTSP_URL_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_RTSP_URL_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SAFETY_EQUIPMENT_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SAFETY_EQUIPMENT_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SAFETY_EQUIPMENT_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SAFETY_HELMET_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SAFETY_HELMET_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SAFETY_HELMET_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SCENE_CHANGE_ALARM_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SCENE_CHANGE_ALARM_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SCENE_CHANGE_ALARM_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SCENE_CHANGE_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SCENE_CHANGE_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SCENE_CHANGE_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SCHED_TIME_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SCHED_TIME_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SCHED_TIME_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SECURITY_SERVICES_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SECURITY_SERVICES_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SECURITY_SERVICES_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SLEEP_ON_DUTY_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SLEEP_ON_DUTY_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SLEEP_ON_DUTY_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SMART_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SMART_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SMART_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SMART_LINE_RULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SMART_LINE_RULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SMART_LINE_RULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SMART_REGION_RULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SMART_REGION_RULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SMART_REGION_RULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SMART_REGION_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SMART_REGION_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SMART_REGION_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SMOKE_DETECT_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SMOKE_DETECT_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SMOKE_DETECT_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SMOKE_FIRE_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SMOKE_FIRE_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SMOKE_FIRE_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SMOKING_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SMOKING_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SMOKING_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SSH_ADMIN_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SSH_ADMIN_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SSH_ADMIN_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SSH_COUNTDOWN_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SSH_COUNTDOWN_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SSH_COUNTDOWN_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_STATISTICS_RESET_CONFIG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_STATISTICS_RESET_CONFIG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_STATISTICS_RESET_CONFIG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SYSTEM_NTP_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SYSTEM_NTP_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_SYSTEM_NTP_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_TALKBACK_STATE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_TALKBACK_STATE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_TALKBACK_STATE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_TALKBACK_STREAM_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_TALKBACK_STREAM_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_TALKBACK_STREAM_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_TAMPER_ALARM_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_TAMPER_ALARM_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_TAMPER_ALARM_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_TAMPER_DETECT_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_TAMPER_DETECT_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_TAMPER_DETECT_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_TRASH_OVERFLOW_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_TRASH_OVERFLOW_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_TRASH_OVERFLOW_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_UNATTENDED_OBJECT_RULE_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_UNATTENDED_OBJECT_RULE_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_UNATTENDED_OBJECT_RULE_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_UPGRADE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_UPGRADE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_UPGRADE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_UPGRADE_STATUS_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_UPGRADE_STATUS_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_UPGRADE_STATUS_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_UPGRADE_VERSION_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_UPGRADE_VERSION_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_UPGRADE_VERSION_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VIDEO_ENCODE_ABILITY_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VIDEO_ENCODE_ABILITY_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_VIDEO_ENCODE_ABILITY_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VIDEO_ENCODE_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VIDEO_ENCODE_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_VIDEO_ENCODE_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VIDEO_ENCODE_OPTION_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VIDEO_ENCODE_OPTION_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_VIDEO_ENCODE_OPTION_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VIDEO_OSD_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VIDEO_OSD_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_VIDEO_OSD_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VIDEO_RESOLUTION_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VIDEO_RESOLUTION_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_VIDEO_RESOLUTION_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VIDEO_STREAM_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VIDEO_STREAM_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_VIDEO_STREAM_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VOICECOM_AUDIO_PARAM_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VOICECOM_AUDIO_PARAM_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_VOICECOM_AUDIO_PARAM_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VOICECOM_START_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VOICECOM_START_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_VOICECOM_START_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_WATER_ACCUMULATION_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_WATER_ACCUMULATION_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_WATER_ACCUMULATION_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_WHITEBALANCE_INFO_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_WHITEBALANCE_INFO_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_WHITEBALANCE_INFO_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_WIFI_STA_CFG_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_WIFI_STA_CFG_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_WIFI_STA_CFG_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_WIFI_STA_CONNECT_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_WIFI_STA_CONNECT_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_WIFI_STA_CONNECT_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_WIFI_WEP_KEY_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_WIFI_WEP_KEY_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_WIFI_WEP_KEY_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  NewTopic := HndTopics.CreateTopic();
  HndTopics.SetTopicCaption(NewTopic, 'NET_TV_WRONG_WAY_DRIVING_CAP_S');
  HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_WRONG_WAY_DRIVING_CAP_S');
  HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
  HndEditor.Clear(Editor);
  HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_topics\NET_TV_WRONG_WAY_DRIVING_CAP_S.html');
  HndEditor.SetAsTopicContent(Editor, NewTopic);

  HndEditor.DestroyTemporaryEditor(Editor);
end.
