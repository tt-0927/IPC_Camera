/*
 * @FilePath     : /cam/share/ipc_share/protocols/gat1400/gat1400.h
 * @Author       : zhengxh@kfb.cn
 * @Date         : 2025-12-03 16:42:22
 * @LastEditors  : zhengxh@kfb.cn
 * @LastEditTime : 2025-12-18 15:41:48
 * @Description  : GAT1400 接入管理类
 */


#pragma once
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <atomic>
#include "Singleton.h"
#include "gat1400_viid.h"
#include "network_define.h"

namespace GAT1400 {

struct LoginInfo {
    int port;               // GAT1400 服务器端口
    std::string ip;         // GAT1400 服务器地址
    std::string deviceID;   // 设备ID
    std::string name;       // 接入平台用户名，与设备ID一致
    std::string passwd;     // 接入平台密码
};

class CGAT1400 : public CSingleton<CGAT1400>
{
private:
    CGAT1400();
public:
    ~CGAT1400();
    friend class CSingleton<CGAT1400>;

    enum class State_EN {
        DISCONNECTED = 0,
        CONNECTED = 1,
        RECONNECTING = 2
    };

    /**
     * @description  : 初始化GAT1400模块
     * @return        int
     */    
    int init();

    /**
     * @description  : 去初始化
     * @return        int
     */    
    int deinit();

    /**
     * @description  : 上传人脸图片列表
     * @param         {security_faces_t} &faces
     * @return        int
     */    
    int uploadFaces(security_faces_t &faces);

    /**
     * @description  : 上传人员图片列表
     * @param         {security_persons_t} &persons
     * @return        int
     */    
    int uploadPersons(security_persons_t &persons);

    /**
     * @description  : 上传非机动车图片列表
     * @param         {security_nonmotorvehicles_t} &nonmotorvehicles
     * @return        int
     */    
    int uploadNonmotorvehicles(security_nonmotorvehicles_t &nonmotorvehicles);

    /**
     * @description  : 上传机动车图片列表
     * @param         {security_motorvehicles_t} &motorvehicles
     * @return        {*}
     */    
    int uploadMotorvehicles(security_motorvehicles_t &motorvehicles);

    void getGat1400Config(Network::Gat1400Client_S &config);

private:
    /**
     * @description  : 保活线程
     * @return        {*}
     */
    void runKeepAlive();
     
    /**
     * @description  : 补充人脸图片的参数信息
     * @param         {security_faces_t} &faces
     * @return        {*}
     */
    void updateFacesInfo(security_faces_t &faces);

    /**
     * @description  : 图像信息基本要素ID
     * @param         {string} strBasicObjectType
     * @param         {string} strTime
     * @return        {*}
     */
    std::string toBasicObjectId(const std::string strBasicObjectType, const std::string strTime = "");

    /**
     * @description  : 图像信息内容要素ID
     * @param         {string} strBasicObectId
     * @param         {string} strImageCntObjectType
     * @return        {*}
     */   
    std::string toImageCntObjectID(const std::string strBasicObectId, const std::string strImageCntObjectType);

    /**
     * @description  : 是否需要更新配置
     * @param         {Gat1400Client_S} &stParam
     * @return        int
     */    
    int checkUpdateParam(const Network::Gat1400Client_S &stParam);

    /* 注册接口 */
    int Register();
    /* 注销接口 */
    int unRegister();
    /* 保活接口 */
    int keepAlive();
    /* 打印配置内容 */
    void dumpInfo(Network::Gat1400Client_S &info);
    /* 上报状态 */
    void publishStatus();

private:
    // gat1400接入接口类
    security_viid m_stInterface;
    // 模块配置信息
    Network::Gat1400Client_S m_stParam;
    // 保活线程
    std::thread m_thread;
    // 条件变量
    std::condition_variable m_condition;
    // 保活是否需要运行
    std::atomic<bool> m_bRuning;
    // 互斥锁
    std::mutex m_mutex;
    // ID 互斥锁
    std::mutex m_ImageMutex;
    std::mutex m_BasicMutex;
    // 最后一次交互时间
    long long m_lLastTime;
    // 配置文件路径
	std::string m_strConfigPath;
    // gat1400接入状态
    State_EN m_enState = State_EN::DISCONNECTED;
};

}