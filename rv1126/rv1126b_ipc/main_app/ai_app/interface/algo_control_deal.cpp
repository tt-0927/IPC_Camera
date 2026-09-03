/**
 * @FilePath     : algo_control_deal.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2025-06-06 16:02:10
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-07-30 16:54:32
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
        if(strData.empty())
        {
            CEventConfigure::instance()->get_configure(stAlgoConfig);
        }
        else 
        {
            Convert::to_struct(strData, stAlgoConfig);
        }
        
        CAlgoStreamDeal::instance()->set_Algo_EnConfig(stAlgoConfig);
        strData.clear();
        break;
    }
    case AC_GET_AUDIO_ANOMALY_DETECT_CURRENT_DB: /* 获取音频异常侦测实时音量 */
    {
        if(pData != nullptr)
        {
            float fDb = CAlgoStreamDeal::instance()->getCurrentDb();
            // memcpy_s(pData, sizeof(float), &fDb, sizeof(float));
            memcpy(pData, &fDb, sizeof(float));
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
