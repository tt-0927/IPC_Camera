/*
 * @Author       : EasonLu
 * @Date         : 2024-03-12 16:49:00
 * @LastEditors  : EasonLu
 * @LastEditTime : 2024-03-13 16:54:14
 * @FilePath     : async_demo.c
 * @Description  : mqtt封装库的使用示例
 */
#include "bl_mqtt.h"

static int onMsgCallback(BlMqttMsg_S stMsg)
{
    if(stMsg.enMsgType == BL_MQTT_MSG_TOPIC)
    {
        printf("收到主题[%s]的消息:\n%s\n", stMsg.pTopicName, stMsg.pMsg);
    }
    return 0;
}

int main(int argc, char* argv[])
{

    printf("=============> mqtt test <=============\n");
    BlMqttNeedParam_S stNeedParam = {
        .pfnCallback = onMsgCallback
    };
    BlMqtt_S *pMqtt = bl_mqtt_alloc(&stNeedParam, NULL);
    printf("mqtt alloc success\n");
    int nRet = pMqtt->init(pMqtt);
    if(0 != nRet)
    {
        printf("mqtt init failed\n");
        return -1;
    }
    /* NOTE 订阅必须要等待连接完成才能进行订阅，否则订阅失败 */
    printf("mqtt init success\n");
    while(1)
    {
        sleep(1);
        if(pMqtt->bConnected)
        {
            nRet = pMqtt->subscribe(pMqtt, "bg6test_sub", 1);
            if (0 != nRet)
            {
                printf("mqtt subscribe failed\n");
                return -1;
            }
            break;
        }
        else
        {
            printf("mqtt not connected\n");
        }
    }

    printf("mqtt subscribe success\n");
    char c;
    do
    {
        c = getchar();
        if(c == 's')
        {   
            char achSendMsg[1024] = {0};
            scanf("%s", achSendMsg);
            pMqtt->publish(pMqtt, "bg6test_pub", achSendMsg, strlen(achSendMsg), 1);
        }
    }while(c != 'q');

    nRet = pMqtt->uninit(pMqtt);
    if(0 != nRet)
    {
        printf("mqtt uninit failed\n");
        return -1;
    }

    bl_mqtt_release(pMqtt);
}