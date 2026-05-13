/**
 * @FilePath     : algo_control_deal.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-04-23 09:42:11
 * @Description  : aiapp->control通讯客户端
 */

#include "algo_control_deal.h"
#include "algo_stream_deal.h"
#include "face_manage.h"
#include "action_code.h"


void AlgoControlDeal::deal_message(int nCode, std::string strData, void *pData)
{
    dlog_debug("AI_APP: 接收到[%d]消息：%s", nCode, strData.c_str());

    Json::Object *pJsonData = Json::init(strData);

    if (!pJsonData)
    {
        dlog_error("AI_APP: 参数错误,不是json数据");
        return;
    }

    switch (nCode)
    {
    case AC_SET_ALGORITHM_CONFIG: /* 设置算法配置 */
    {
        Event::AlgorithmConfig stAlgoConfig;
        Convert::to_struct(strData, stAlgoConfig);
        // CEventConfigure::instance()->get_configure(stAlgoConfig);
        //if (stAlgoConfig.nEnFaceFea && !stAlgoConfig.nEnFaceDet) { stAlgoConfig.nEnFaceFea = 0; }

        CAlgoStreamDeal::instance()->set_Algo_EnConfig(stAlgoConfig);
        strData.clear();
        break;
    }
    case AC_GET_AUDIO_ANOMALY_DETECT_CURRENT_DB: /* 获取音频异常侦测实时音量 */
    {
        if(pData != nullptr)
        {
            float fDb = CAlgoStreamDeal::instance()->getCurrentDb();
            memcpy_s(pData, sizeof(float), &fDb, sizeof(float));
        }
        break;
    }
#if CAP_AI_PEOPLE_STATISTICS
    case AC_CLEAR_PEOPLE_FLOW_STATISTICS_RESULT: /* 清空人流统计运行态结果 */
    {
        RuntimeCommand_S stCommand;
        stCommand.nCode = AC_CLEAR_PEOPLE_FLOW_STATISTICS_RESULT;
        const int nRet = CAlgoStreamDeal::instance()->dispatchRuntimeCommand(stCommand);
        if (pData != nullptr)
        {
            *static_cast<int *>(pData) = nRet;
        }
        break;
    }
#endif
    case AC_ADD_FACE_INFO:                    /* 添加名单组成员 */
    {
        FaceLibsInfo_S stFaceList; stFaceList.clear();
        bool nRet;
        Json::get(pJsonData, "LibId", stFaceList.strFaceLibName);
        Json::get(pJsonData, "Name", stFaceList.strName);
        Json::get(pJsonData, "PhoneNum", stFaceList.strPhoneNum);
        Json::get(pJsonData, "PicPath", stFaceList.strPicPath);
        Json::get(pJsonData, "BinPath", stFaceList.BinPath);
        Json::get(pJsonData, "PicType", stFaceList.strPicType);
        Json::get(pJsonData, "PicSize", stFaceList.nPicSize);
        Json::get(pJsonData, "PicDate", stFaceList.strPicDate);
        Json::get(pJsonData, "PicWidth", stFaceList.PicWidth);
        Json::get(pJsonData, "PicHeight", stFaceList.PicHeight);
        /* 发送到 AlgoStreamDeal 通知 Algorithm */
        nRet = CAlgoStreamDeal::instance()->add_Facelib_Groups(stFaceList);

        

        if (pData != nullptr)
        {
            int* nRetData = (int*)pData;
            if(nRet!=true)
            {
                *nRetData = -1;
            }else {
                *nRetData = 0;
            }
            dlog_debug("添加名单组成员 %d",nRet);
        }
        
        break;
    }
    case AC_DEL_FACE_INFO:           /* 删除名单组成员 */
    {
        Json::Object* idsArray = Json::get(pJsonData, "Ids");
        
        std::vector<int> ids; 
        if (Json::Array::get(idsArray, ids))
        {
            for (const int& id : ids)
            {
                FaceManage::AIFaceManage::instance()->delFaceLibInfo(id);
            }
        }
        break;
    }
    case AC_SET_FACE_INFO:           /* 修改名单组成员 */
    {
        FaceLibsInfo_S stFaceList; stFaceList.clear();

        Json::get(pJsonData, "Id", stFaceList.nId);
        Json::get(pJsonData, "LibId", stFaceList.strFaceLibName);
        Json::get(pJsonData, "Name", stFaceList.strName);
        Json::get(pJsonData, "PhoneNum", stFaceList.strPhoneNum);
        Json::get(pJsonData, "PicPath", stFaceList.strPicPath);
        Json::get(pJsonData, "PicType", stFaceList.strPicType);
        Json::get(pJsonData, "PicSize", stFaceList.nPicSize);
        Json::get(pJsonData, "PicDate", stFaceList.strPicDate);
        
        FaceManage::AIFaceManage::instance()->updateFaceLibInfo(stFaceList.nId, stFaceList);
        break;
    }
    case AC_GET_FACE_INFO: /* 查询名单组成员 */
    {      
        Event::FaceFind_S stFaceFind;
        dlog_debug("1111")
        std::string   strData   = Json::to_string(pJsonData);
        Convert::to_struct(strData, stFaceFind);
        dlog_debug("2222")
        std::list<FaceLibsInfo_S> listOutInfo;
        int nRet = FaceManage::AIFaceManage::instance()->searchFaceInfoByCond(stFaceFind, listOutInfo);
        dlog_debug("3333")
        std::vector<Event::FaceInfo_S> listFaceInfo;
        for (const auto& OutInfo : listOutInfo)
        {
            Event::FaceInfo_S stFaceInfo;
            stFaceInfo.nId = OutInfo.nId;
            stFaceInfo.strFaceLibName = OutInfo.strFaceLibName;
            stFaceInfo.strName = OutInfo.strName;
            stFaceInfo.strPhoneNum = OutInfo.strPhoneNum;
            stFaceInfo.strPicPath = OutInfo.strPicPath;
            stFaceInfo.strPicType = OutInfo.strPicType;
            stFaceInfo.nPicSize = OutInfo.nPicSize;
            stFaceInfo.strPicDate = OutInfo.strPicDate;
            stFaceInfo.nModelState = OutInfo.nModelState;
            stFaceInfo.nRatingLevel = OutInfo.nRatingLevel;
            stFaceInfo.BinPath = OutInfo.BinPath;
            listFaceInfo.push_back(stFaceInfo);
        }
        
        dlog_debug("ai_app: \033[34m %s:%d 条件查询表 [%s : %ld] \033[m\n",__func__,__LINE__,stFaceFind.strFaceLibName.c_str(), listOutInfo.size());
        std::string strDataRet;
        if (nRet != 0)
        {
            strDataRet = "{\"result\": \"Failed to search face list information.\" }";
        }
        else
        {
            strDataRet = Convert::to_string(listFaceInfo);
        }
        if (pData != nullptr)
        {
            std::string* pOutString = (std::string*)pData;
            *pOutString = strDataRet; 
        }
        break;
    }
    case AC_FACE_COMPARE_BY_IMAGE: /* 人员检索人脸比对 */
    {
        std::string strPicPath;
        Json::get(pJsonData, "ImagePath", strPicPath);

        if (access(strPicPath.c_str(), F_OK) == 0)
        {
            /* 发送到 AlgoStreamDeal 通知 Algorithm */
            CAlgoStreamDeal::instance()->compare_Face_Retrieval(strPicPath);
        }
        else
        {
            std::string strData = "{\"result\":\"Input ImagePath is not exist !!!\"}";
            // SendResToControl(strData, nCode, -1);
        }
        break;
    }
    case  AC_ADD_TARGET_LIB: /* 添加目标库 */
    {
        std::string strTabName;
        int nRet;
        Json::get(pJsonData, "LibId", strTabName);   
        nRet =  FaceManage::AIFaceManage::instance()->creatFaceTable(strTabName);
        if (pData != nullptr)
        {
            int* nRetData = (int*)pData;
            *nRetData = nRet;
            dlog_debug("添加目标库结果 %d",nRet);
        }
        break;
    }
    case  AC_DEL_TARGET_LIB: /* 删除目标库 */
    {
        int nRet;
        if (pJsonData && cJSON_IsObject(pJsonData)) 
        {
            cJSON* libIdArray = cJSON_GetObjectItem(pJsonData, "LibId");
            if (libIdArray && cJSON_IsArray(libIdArray))
            {
                int size = cJSON_GetArraySize(libIdArray);
                for (int i = 0; i < size; i++) {
                    cJSON* item = cJSON_GetArrayItem(libIdArray, i); 
                    if (item && cJSON_IsString(item)) {   
                        const char* strLibId = item->valuestring; 
                        std::string strTabName(strLibId);
                        nRet = FaceManage::AIFaceManage::instance()->deleteFaceTable(strTabName);
                    }
                }
            }
            else if (cJSON_IsString(libIdArray) && libIdArray->valuestring)
            {
                nRet = FaceManage::AIFaceManage::instance()->deleteFaceTable(libIdArray->valuestring);
            }

        }
        if (pData != nullptr)
        {
            int* nRetData = (int*)pData;
            *nRetData = nRet;
        }
        break;
    }
    case  AC_SET_TARGET_LIB: /* 修改目标库 */
    {
        std::string strTabNameOld;
        std::string strTabNameNew;
        Json::get(pJsonData, "LibId_old", strTabNameOld);
        Json::get(pJsonData, "LibId_new", strTabNameNew);
        dlog_debug("修改目标库 AC_SET_TARGET_LIB");
        if (strTabNameOld != strTabNameNew)
        {
            FaceManage::AIFaceManage::instance()->renameFaceTable(strTabNameOld, strTabNameNew);
        }
        break;
    }
    case  AC_GET_TARGET_LIB: /* 查询目标库 */
    {
        std::vector<Event::FaceLibInfo_S> listTableReport;
        dlog_debug("查询目标库 AC_GET_TARGET_LIB");
        int nRet = FaceManage::AIFaceManage::instance()->getTableReport(listTableReport);
        if (nRet != 0)
        {
            dlog_error("查询人脸库失败，nRet=%d", nRet);
            return; // 或者返回错误码
        }
        std::string strJsonData = "[]";
        try 
        {
            strJsonData = Convert::to_string(listTableReport);
            dlog_debug("JSON 转换成功，长度: %zu, 内容: %s", strJsonData.length(), strJsonData.c_str());
        }
        catch (...)
        {
            dlog_error("JSON 转换异常");
            strJsonData = "[]";
        }
       
        if (pData != nullptr)
        {
            try
            {
                std::string* pOutString = (std::string*)pData;
                *pOutString = strJsonData;
                dlog_debug("赋值给 pData 成功");
            }
            catch (...)
            {
                dlog_error("赋值给 pData 时崩溃！请检查调用方传的指针类型");
            }
        }
        else
        {
            dlog_error("警告: pData 为空，无法返回数据");
        }
        break;
    }

    default:
        dlog_debug("AI_APP: 未知的消息代码: %d", nCode);
        break;
    }

    Json::deinit(pJsonData);
    return;
}
