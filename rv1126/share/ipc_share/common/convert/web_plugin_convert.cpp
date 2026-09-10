/**
 * @FilePath     : web_plugin_convert.cpp
 * @Author       : zhangjc (zhangjc@kfb.cn)
 * @Date         : 2025-06-11 15:26:31
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-06-11 16:47:10
 * @Description  : PC端的插件配置参数转换
 */

#include "web_plugin_convert.h"

#include "convert.h" /* 这个要放在UserDefineConvert的后面 */

void Convert::deal(Json::Object *pRootJson, WebPlugin::StreamParam_S &stStreamParam, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "ProtoType", stStreamParam.nProtoType);
    convert.field(pRootJson, "StreamType", stStreamParam.nStreamType);
    convert.field(pRootJson, "Performance", stStreamParam.nPerformance);
    convert.field(pRootJson, "FrameRate", stStreamParam.nFrameRate);
    convert.field(pRootJson, "IsEnableRuleInfo", stStreamParam.bEnableRuleInfo);
    convert.field(pRootJson, "ImageScale", stStreamParam.nImageScale);
    convert.field(pRootJson, "IsEnableAutoPreview", stStreamParam.bEnableAutoPreview);
    convert.field(pRootJson, "ImageRecordType", stStreamParam.nImageRecordType);
    convert.field(pRootJson, "StreamKey", stStreamParam.streamKey);
}
void Convert::deal(Json::Object *pRootJson, WebPlugin::FileSavePath_S &stFileSavePath, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "RecordFile", stFileSavePath.recordFile);
    convert.field(pRootJson, "PlaybackDown", stFileSavePath.playbackDown);
    convert.field(pRootJson, "PreviewSnapshot", stFileSavePath.previewSnapshot);
    convert.field(pRootJson, "PlaybackSnapshot", stFileSavePath.playbackSnapshot);
    convert.field(pRootJson, "PlaybackClip", stFileSavePath.playbackClip);
    convert.field(pRootJson, "FacialSnapshot", stFileSavePath.facialSnapshot);
}
void Convert::deal(Json::Object *pRootJson, WebPlugin::RecordParam_S &stRecordParam, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.field(pRootJson, "RecordFileSize", stRecordParam.nRecordFileSize);
    convert.structure(pRootJson, stRecordParam.stFileSavePath);
}
void Convert::deal(Json::Object *pRootJson, WebPlugin::Param_S &stParam, bool bOutStruct)
{
    if (!pRootJson)
    {
        return;
    }
    Convert::CConvert convert(bOutStruct);
    convert.structure(pRootJson, stParam.stStreamParam);
    convert.structure(pRootJson, stParam.stRecordParam);
}