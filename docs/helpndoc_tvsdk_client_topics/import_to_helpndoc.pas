// HelpNDoc Pascal Script
// 在 HelpNDoc 中打开目标 .hnd 工程后，运行本脚本。
// 作用：自动创建 SDK client 接口定义、接口参数结构体和接口调用Demo topic，并导入对应 HTML 内容。

var
  RootTopic: string;
var
  InterfaceTopic: string;
var
  StructTopic: string;
var
  DemoTopic: string;
var
  NewTopic: string;
var
  Editor: TObject;

begin
  Editor := HndEditor.CreateTemporaryEditor();
  try
    RootTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(RootTopic, 'SDK client接口定义');
    HndTopics.SetTopicHelpId(RootTopic, 'SDK_Client_Interface_Definition');

    InterfaceTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(InterfaceTopic, '接口定义');
    HndTopics.SetTopicHelpId(InterfaceTopic, 'SDK_Client_Interface_List');
    HndTopics.MoveTopic(InterfaceTopic, RootTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\接口定义.html');
    HndEditor.SetAsTopicContent(Editor, InterfaceTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_Cleanup');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_Cleanup');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_Cleanup.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ControlReplay');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ControlReplay');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ControlReplay.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_DeviceControl');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_DeviceControl');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_DeviceControl.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_Discovery_Search');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_Discovery_Search');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_Discovery_Search.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_GetDevConfig');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_GetDevConfig');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_GetDevConfig.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_GetDeviceCapability');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_GetDeviceCapability');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_GetDeviceCapability.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_GetLastError');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_GetLastError');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_GetLastError.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_GetReplayRecordList');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_GetReplayRecordList');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_GetReplayRecordList.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_GetReplayUrl');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_GetReplayUrl');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_GetReplayUrl.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_GetSDKVersion');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_GetSDKVersion');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_GetSDKVersion.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_Init');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_Init');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_Init.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_Login');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_Login');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_Login.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_Logout');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_Logout');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_Logout.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SetAlarmCallBack');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SetAlarmCallBack');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SetAlarmCallBack.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SetChannelStatusCallBack');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SetChannelStatusCallBack');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SetChannelStatusCallBack.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SetConnectTime');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SetConnectTime');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SetConnectTime.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SetDevConfig');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SetDevConfig');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SetDevConfig.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SetExceptionCallBack');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SetExceptionCallBack');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SetExceptionCallBack.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SetLogToFile');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SetLogToFile');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SetLogToFile.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SetRevTimeOut');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SetRevTimeOut');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SetRevTimeOut.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_StartListen');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_StartListen');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_StartListen.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_StartRecordFrameStream');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_StartRecordFrameStream');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_StartRecordFrameStream.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_StartVoiceCom');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_StartVoiceCom');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_StartVoiceCom.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_StopListen');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_StopListen');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_StopListen.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_StopRecordFrameStream');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_StopRecordFrameStream');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_StopRecordFrameStream.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_StopVoiceCom');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_StopVoiceCom');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_StopVoiceCom.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_UploadFile');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_UploadFile');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_UploadFile.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VoiceComSendData');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VoiceComSendData');
    HndTopics.MoveTopic(NewTopic, InterfaceTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_VoiceComSendData.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    StructTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(StructTopic, '接口参数结构体');
    HndTopics.SetTopicHelpId(StructTopic, 'SDK_Client_Struct_List');
    HndTopics.MoveTopic(StructTopic, RootTopic, htamAddChild);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_4G_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_4G_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_4G_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_AI_SIMPLE_RULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_AI_SIMPLE_RULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_AI_SIMPLE_RULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_AI_OBJECT_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_AI_OBJECT_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ALARM_AI_OBJECT_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_BASIC_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_BASIC_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ALARM_BASIC_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_EXCEPTION_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_EXCEPTION_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ALARM_EXCEPTION_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_FACE_COMPARE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_FACE_COMPARE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ALARM_FACE_COMPARE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_PLATE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_PLATE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ALARM_PLATE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_RULE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_RULE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ALARM_RULE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_SCHEDULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_SCHEDULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ALARM_SCHEDULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_STATISTICS_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_STATISTICS_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ALARM_STATISTICS_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARM_STATISTICS_TARGET_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARM_STATISTICS_TARGET_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ALARM_STATISTICS_TARGET_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ALARMER_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ALARMER_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ALARMER_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_AUDIO_ANOMALY_ALARM_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_AUDIO_ANOMALY_ALARM_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_AUDIO_ANOMALY_ALARM_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_AUDIO_ANOMALY_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_AUDIO_ANOMALY_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_AUDIO_ANOMALY_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_AUDIO_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_AUDIO_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_AUDIO_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_AUDIO_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_AUDIO_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_AUDIO_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_AUDIO_FORMAT_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_AUDIO_FORMAT_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_AUDIO_FORMAT_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_AUDIO_RANGE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_AUDIO_RANGE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_AUDIO_RANGE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_BACKLIGHT_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_BACKLIGHT_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_BACKLIGHT_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_BARE_SOIL_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_BARE_SOIL_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_BARE_SOIL_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_BEHAVIOR_DETECT_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_BEHAVIOR_DETECT_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_BEHAVIOR_DETECT_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_BOUNDARY_DETECT_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_BOUNDARY_DETECT_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_BOUNDARY_DETECT_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_BOUNDARY_PLANE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_BOUNDARY_PLANE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_BOUNDARY_PLANE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CAPTURE_CONFIG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CAPTURE_CONFIG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CAPTURE_CONFIG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CAPTURE_DAY_SCHEDULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CAPTURE_DAY_SCHEDULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CAPTURE_DAY_SCHEDULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CAPTURE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CAPTURE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CAPTURE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CAPTURE_PARAM_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CAPTURE_PARAM_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CAPTURE_PARAM_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CAPTURE_PLAN_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CAPTURE_PLAN_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CAPTURE_PLAN_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CAPTURE_TIME_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CAPTURE_TIME_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CAPTURE_TIME_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CHANNEL_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CHANNEL_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CHANNEL_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CHANNEL_LIST_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CHANNEL_LIST_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CHANNEL_LIST_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CLIMB_FENCE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CLIMB_FENCE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CLIMB_FENCE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CONGESTION_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CONGESTION_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CONGESTION_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CONGESTION_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CONGESTION_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CONGESTION_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CONSTRUCTION_OCCUPANCY_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CONSTRUCTION_OCCUPANCY_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CONSTRUCTION_OCCUPANCY_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CONSTRUCTION_OCCUPY_ROAD_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CROSS_LINE_ALARM_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CROSS_LINE_ALARM_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CROSS_LINE_ALARM_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CROWD_GATHERING_ALARM_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CROWD_GATHERING_ALARM_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CROWD_GATHERING_ALARM_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CROWD_GATHERING_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CROWD_GATHERING_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CROWD_GATHERING_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_CROWD_GATHERING_RULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_CROWD_GATHERING_RULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_CROWD_GATHERING_RULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_DAYNIGHT_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_DAYNIGHT_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_DAYNIGHT_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_DENOISE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_DENOISE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_DENOISE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_DEVICE_BASICINFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_DEVICE_BASICINFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_DEVICE_BASICINFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_DEVICE_CONTROL_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_DEVICE_CONTROL_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_DEVICE_CONTROL_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_DEVICE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_DEVICE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_DEVICE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_DEVICE_LOGIN_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_DEVICE_LOGIN_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_DEVICE_LOGIN_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_DIMISSION_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_DIMISSION_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_DIMISSION_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_DISCOVERY_DEVICE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_DISCOVERY_DEVICE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_DISCOVERY_DEVICE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ELECTRIC_VEHICLE_IN_ELEVATOR_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_EMERGENCY_LANE_OCCUPANCY_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_EMERGENCY_LANE_OCCUPANCY_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_EMERGENCY_LANE_OCCUPANCY_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ENTER_EXIT_DETECT_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ENTER_EXIT_DETECT_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ENTER_EXIT_DETECT_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ENTER_REGION_ALARM_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ENTER_REGION_ALARM_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ENTER_REGION_ALARM_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ENVIRONMENT_ANOMALY_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ENVIRONMENT_ANOMALY_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ENVIRONMENT_ANOMALY_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_EXPOSURE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_EXPOSURE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_EXPOSURE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_CAPTURE_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_CAPTURE_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_FACE_CAPTURE_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_CAPTURE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_CAPTURE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_FACE_CAPTURE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_CAPTURE_REGION_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_CAPTURE_REGION_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_FACE_CAPTURE_REGION_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_CAPTURE_RULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_CAPTURE_RULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_FACE_CAPTURE_RULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_COMPARE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_COMPARE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_FACE_COMPARE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_DETECT_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_DETECT_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_FACE_DETECT_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_ID_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_ID_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_FACE_ID_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_INFO_LIST_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_INFO_LIST_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_FACE_INFO_LIST_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_FACE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_LIB_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_LIB_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_FACE_LIB_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FACE_LIB_LIST_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FACE_LIB_LIST_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_FACE_LIB_LIST_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_FIRE_DETECT_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_FIRE_DETECT_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_FIRE_DETECT_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_GARBAGE_EXPOSURE_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_GARBAGE_EXPOSURE_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_GARBAGE_EXPOSURE_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_GARBAGE_EXPOSURE_RULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_GARBAGE_EXPOSURE_RULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_GARBAGE_EXPOSURE_RULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_GARBAGE_OVERFLOW_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_GARBAGE_OVERFLOW_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_GARBAGE_OVERFLOW_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_GARBAGE_OVERFLOW_RULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_GARBAGE_OVERFLOW_RULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_GARBAGE_OVERFLOW_RULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_HIGH_ALTITUDE_SEATBELT_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_HOLE_PROTECTION_BAR_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_HOLE_PROTECTION_BAR_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_HOLE_PROTECTION_BAR_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_HOTSPOT_CONN_DEVICE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_HOTSPOT_CONN_DEVICE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_HOTSPOT_CONN_DEVICE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_HOTSPOT_CONN_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_HOTSPOT_CONN_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_HOTSPOT_CONN_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_HOTSPOT_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_HOTSPOT_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_HOTSPOT_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ILLEGAL_LANE_CHANGE_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ILLEGAL_LANE_CHANGE_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ILLEGAL_LANE_CHANGE_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ILLEGAL_LANE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ILLEGAL_LANE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ILLEGAL_LANE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ILLEGAL_PARKING_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ILLEGAL_PARKING_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ILLEGAL_PARKING_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_IMAGE_SETTING_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_IMAGE_SETTING_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_IMAGE_SETTING_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_INTRUSION_ALARM_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_INTRUSION_ALARM_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_INTRUSION_ALARM_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_INTRUSION_DETECT_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_INTRUSION_DETECT_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_INTRUSION_DETECT_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_INTRUSION_RULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_INTRUSION_RULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_INTRUSION_RULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LEAVE_REGION_ALARM_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LEAVE_REGION_ALARM_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_LEAVE_REGION_ALARM_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LICENSE_PLATE_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LICENSE_PLATE_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_LICENSE_PLATE_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_LICENSE_PLATE_RECOGNITION_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LINKAGE_LIST_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LINKAGE_LIST_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_LINKAGE_LIST_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LOG_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LOG_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_LOG_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LOG_LIST_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LOG_LIST_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_LOG_LIST_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LOG_RETRIEVAL_COND_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LOG_RETRIEVAL_COND_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_LOG_RETRIEVAL_COND_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LOG_SERVER_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LOG_SERVER_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_LOG_SERVER_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LOGIN_LOCK_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LOGIN_LOCK_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_LOGIN_LOCK_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LOITERING_ALARM_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LOITERING_ALARM_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_LOITERING_ALARM_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LOITERING_DETECT_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LOITERING_DETECT_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_LOITERING_DETECT_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_LOITERING_RULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_LOITERING_RULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_LOITERING_RULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_MANHOLE_COVER_ABNORMAL_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_MOTION_ALARM_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_MOTION_ALARM_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_MOTION_ALARM_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_MOTION_DETECT_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_MOTION_DETECT_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_MOTION_DETECT_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_MOTION_EXPERT_MODE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_MOTION_EXPERT_MODE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_MOTION_EXPERT_MODE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_MOTION_NORMAL_MODE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_MOTION_NORMAL_MODE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_MOTION_NORMAL_MODE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_MOTION_REGION_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_MOTION_REGION_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_MOTION_REGION_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_NETWORKCFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_NETWORKCFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_NETWORKCFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_NON_MOTOR_VEHICLE_INTRUSION_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_NON_MOTOR_VEHICLE_INTRUSION_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_NON_MOTOR_VEHICLE_INTRUSION_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_NONMOTOR_VEHICLE_INTRUSION_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_OBJECT_CHANGE_DETECT_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_OBJECT_CHANGE_DETECT_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_OBJECT_CHANGE_DETECT_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_OBJECT_REMOVAL_ALARM_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_OBJECT_REMOVAL_ALARM_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_OBJECT_REMOVAL_ALARM_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_OBJECT_REMOVAL_RULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_OBJECT_REMOVAL_RULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_OBJECT_REMOVAL_RULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_OCCUPATION_EMERGENCY_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_OCCUPATION_EMERGENCY_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_OCCUPATION_EMERGENCY_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_OPEN_FLAME_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_OPEN_FLAME_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_OPEN_FLAME_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_OSD_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_OSD_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_OSD_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PAGE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PAGE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PAGE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PARKING_ALARM_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PARKING_ALARM_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PARKING_ALARM_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PARKING_DETECT_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PARKING_DETECT_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PARKING_DETECT_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PARKING_RULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PARKING_RULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PARKING_RULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PEDESTRIAN_INTRUSION_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PEDESTRIAN_INTRUSION_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PEDESTRIAN_INTRUSION_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PEOPLE_ALARM_CONFIG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PEOPLE_ALARM_CONFIG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PEOPLE_ALARM_CONFIG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PEOPLE_ALARM_RULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PEOPLE_ALARM_RULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PEOPLE_ALARM_RULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PEOPLE_DENSITY_DETECTION_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PEOPLE_FLOW_RULE_LINE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PEOPLE_FLOW_RULE_LINE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PEOPLE_FLOW_RULE_LINE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PEOPLE_FLOW_STATISTICS_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PERSON_FALL_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PERSON_FALL_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PERSON_FALL_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PERSON_FALL_DOWN_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PERSON_FALL_DOWN_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PERSON_FALL_DOWN_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PET_RECOGNITION_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PET_RECOGNITION_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PET_RECOGNITION_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PET_RECOGNITION_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PET_RECOGNITION_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PET_RECOGNITION_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PHONE_USAGE_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PHONE_USAGE_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PHONE_USAGE_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PREVIEW_IMAGE_PARAM_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PREVIEW_IMAGE_PARAM_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PREVIEW_IMAGE_PARAM_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PREVIEW_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PREVIEW_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PREVIEW_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PREVIEW_RTSP_URL_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PREVIEW_RTSP_URL_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PREVIEW_RTSP_URL_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PRIVACY_MASK_AREA_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PRIVACY_MASK_AREA_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PRIVACY_MASK_AREA_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PRIVACY_MASK_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PRIVACY_MASK_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PRIVACY_MASK_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_PWD_POLICY_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_PWD_POLICY_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_PWD_POLICY_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RANGE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RANGE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RANGE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_ADVANCED_PARAM_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_ADVANCED_PARAM_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_ADVANCED_PARAM_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_DAY_SCHEDULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_DAY_SCHEDULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_DAY_SCHEDULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_DOWNLOAD_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_DOWNLOAD_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_DOWNLOAD_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_DOWNLOAD_LIST_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_DOWNLOAD_LIST_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_DOWNLOAD_LIST_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_DOWNLOAD_PROGRESS_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_DOWNLOAD_PROGRESS_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_DOWNLOAD_PROGRESS_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_FILE_LIST_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_FILE_LIST_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_FILE_LIST_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_FIND_COND_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_FIND_COND_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_FIND_COND_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_FIND_RESULT_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_FIND_RESULT_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_FIND_RESULT_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_FRAME_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_FRAME_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_FRAME_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_FRAME_RTP_HEADER_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_FRAME_RTP_HEADER_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_FRAME_RTP_HEADER_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_FRAME_STOP_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_FRAME_STOP_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_FRAME_STOP_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_FRAME_STREAM_COND_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_FRAME_STREAM_COND_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_FRAME_STREAM_COND_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_FRAME_STREAM_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_FRAME_STREAM_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_FRAME_STREAM_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_SCHEDULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_SCHEDULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_SCHEDULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_STATUS_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_STATUS_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_STATUS_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_TIME_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_TIME_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_TIME_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RECORD_VIDEO_TIME_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RECORD_VIDEO_TIME_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RECORD_VIDEO_TIME_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_REFLECTIVE_CLOTHING_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_REFLECTIVE_CLOTHING_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_REFLECTIVE_CLOTHING_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_REPLAY_CTRL_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_REPLAY_CTRL_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_REPLAY_CTRL_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_REPLAY_RECORD_LIST_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_REPLAY_RECORD_LIST_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_REPLAY_RECORD_LIST_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_REPLAY_RECORD_TIME_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_REPLAY_RECORD_TIME_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_REPLAY_RECORD_TIME_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_REPLAY_TALKBACK_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_REPLAY_TALKBACK_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_REPLAY_TALKBACK_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_REPLAY_URL_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_REPLAY_URL_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_REPLAY_URL_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RETROGRADE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RETROGRADE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RETROGRADE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_REV_TIMEOUT_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_REV_TIMEOUT_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_REV_TIMEOUT_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_ROAD_PONDING_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_ROAD_PONDING_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_ROAD_PONDING_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_RTSP_URL_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_RTSP_URL_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_RTSP_URL_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SAFETY_EQUIPMENT_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SAFETY_EQUIPMENT_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SAFETY_EQUIPMENT_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SAFETY_HELMET_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SAFETY_HELMET_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SAFETY_HELMET_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SCENE_CHANGE_ALARM_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SCENE_CHANGE_ALARM_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SCENE_CHANGE_ALARM_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SCENE_CHANGE_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SCENE_CHANGE_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SCENE_CHANGE_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SCHED_TIME_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SCHED_TIME_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SCHED_TIME_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SECURITY_SERVICES_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SECURITY_SERVICES_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SECURITY_SERVICES_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SLEEP_ON_DUTY_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SLEEP_ON_DUTY_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SLEEP_ON_DUTY_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SMART_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SMART_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SMART_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SMART_LINE_RULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SMART_LINE_RULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SMART_LINE_RULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SMART_REGION_RULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SMART_REGION_RULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SMART_REGION_RULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SMART_REGION_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SMART_REGION_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SMART_REGION_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SMOKE_DETECT_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SMOKE_DETECT_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SMOKE_DETECT_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SMOKE_FIRE_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SMOKE_FIRE_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SMOKE_FIRE_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SMOKING_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SMOKING_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SMOKING_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SSH_ADMIN_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SSH_ADMIN_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SSH_ADMIN_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SSH_COUNTDOWN_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SSH_COUNTDOWN_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SSH_COUNTDOWN_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_STATISTICS_RESET_CONFIG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_STATISTICS_RESET_CONFIG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_STATISTICS_RESET_CONFIG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_SYSTEM_NTP_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_SYSTEM_NTP_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_SYSTEM_NTP_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_TALKBACK_STATE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_TALKBACK_STATE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_TALKBACK_STATE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_TALKBACK_STREAM_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_TALKBACK_STREAM_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_TALKBACK_STREAM_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_TAMPER_ALARM_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_TAMPER_ALARM_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_TAMPER_ALARM_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_TAMPER_DETECT_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_TAMPER_DETECT_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_TAMPER_DETECT_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_TRASH_OVERFLOW_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_TRASH_OVERFLOW_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_TRASH_OVERFLOW_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_UNATTENDED_OBJECT_ALARM_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_UNATTENDED_OBJECT_RULE_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_UNATTENDED_OBJECT_RULE_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_UNATTENDED_OBJECT_RULE_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_UPGRADE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_UPGRADE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_UPGRADE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_UPGRADE_STATUS_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_UPGRADE_STATUS_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_UPGRADE_STATUS_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_UPGRADE_VERSION_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_UPGRADE_VERSION_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_UPGRADE_VERSION_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VIDEO_ENCODE_ABILITY_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VIDEO_ENCODE_ABILITY_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_VIDEO_ENCODE_ABILITY_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VIDEO_ENCODE_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VIDEO_ENCODE_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_VIDEO_ENCODE_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VIDEO_ENCODE_OPTION_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VIDEO_ENCODE_OPTION_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_VIDEO_ENCODE_OPTION_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VIDEO_OSD_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VIDEO_OSD_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_VIDEO_OSD_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VIDEO_RESOLUTION_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VIDEO_RESOLUTION_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_VIDEO_RESOLUTION_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VIDEO_STREAM_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VIDEO_STREAM_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_VIDEO_STREAM_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VOICECOM_AUDIO_PARAM_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VOICECOM_AUDIO_PARAM_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_VOICECOM_AUDIO_PARAM_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_VOICECOM_START_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_VOICECOM_START_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_VOICECOM_START_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_WATER_ACCUMULATION_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_WATER_ACCUMULATION_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_WATER_ACCUMULATION_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_WHITEBALANCE_INFO_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_WHITEBALANCE_INFO_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_WHITEBALANCE_INFO_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_WIFI_STA_CFG_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_WIFI_STA_CFG_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_WIFI_STA_CFG_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_WIFI_STA_CONNECT_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_WIFI_STA_CONNECT_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_WIFI_STA_CONNECT_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_WIFI_WEP_KEY_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_WIFI_WEP_KEY_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_WIFI_WEP_KEY_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'NET_TV_WRONG_WAY_DRIVING_CAP_S');
    HndTopics.SetTopicHelpId(NewTopic, 'NET_TV_WRONG_WAY_DRIVING_CAP_S');
    HndTopics.MoveTopic(NewTopic, StructTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\NET_TV_WRONG_WAY_DRIVING_CAP_S.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    DemoTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(DemoTopic, '接口调用Demo');
    HndTopics.SetTopicHelpId(DemoTopic, 'SDK_Client_Demo_List');
    HndTopics.MoveTopic(DemoTopic, RootTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\接口调用Demo.html');
    HndEditor.SetAsTopicContent(Editor, DemoTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'Demo_alarm_main_cpp');
    HndTopics.SetTopicHelpId(NewTopic, 'Demo_alarm_main_cpp');
    HndTopics.MoveTopic(NewTopic, DemoTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\Demo_alarm_main_cpp.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'Demo_capability_main_c');
    HndTopics.SetTopicHelpId(NewTopic, 'Demo_capability_main_c');
    HndTopics.MoveTopic(NewTopic, DemoTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\Demo_capability_main_c.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'Demo_config_main_cpp');
    HndTopics.SetTopicHelpId(NewTopic, 'Demo_config_main_cpp');
    HndTopics.MoveTopic(NewTopic, DemoTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\Demo_config_main_cpp.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'Demo_discovery_main_cpp');
    HndTopics.SetTopicHelpId(NewTopic, 'Demo_discovery_main_cpp');
    HndTopics.MoveTopic(NewTopic, DemoTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\Demo_discovery_main_cpp.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

    NewTopic := HndTopics.CreateTopic();
    HndTopics.SetTopicCaption(NewTopic, 'Demo_http_face_main_cpp');
    HndTopics.SetTopicHelpId(NewTopic, 'Demo_http_face_main_cpp');
    HndTopics.MoveTopic(NewTopic, DemoTopic, htamAddChild);
    HndEditor.Clear(Editor);
    HndEditor.InsertFile(Editor, 'E:\Code\IPC_Camera\docs\helpndoc_tvsdk_client_topics\Demo_http_face_main_cpp.html');
    HndEditor.SetAsTopicContent(Editor, NewTopic);

  finally
    HndEditor.DestroyTemporaryEditor(Editor);
  end;
end.
