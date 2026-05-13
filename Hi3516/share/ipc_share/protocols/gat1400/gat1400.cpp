/*
 * @FilePath     : /cam/share/ipc_share/protocols/gat1400/gat1400.cpp
 * @Author       : zhengxh@kfb.cn
 * @Date         : 2025-12-03 16:51:19
 * @LastEditors  : zhengxh@kfb.cn
 * @LastEditTime : 2025-12-24 09:08:25
 * @Description  : 
 */


#include "gat1400.h"
#include "convert_interface.h"
#include "dlog.h"
#include "gat1400_category.h"
#include "gat1400_types.h"
#include "time_utils.h"
#include <cstdio>
#include <mutex>
#include <thread>
#include <algorithm>
#include "IpcRet.h"
#include "path_define.h"
#include "action_code.h"
#include "task_publish.h"

namespace GAT1400 {
// NOTE: This code came up with the following stackoverflow post:
// https://stackoverflow.com/questions/180947/base64-decode-snippet-in-c
std::string base64_encode(const std::string &in) {
    static const auto lookup =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  
    std::string out;
    out.reserve(in.size());
  
    int val = 0;
    int valb = -6;
  
    for (auto c : in) {
      val = (val << 8) + static_cast<uint8_t>(c);
      valb += 8;
      while (valb >= 0) {
        out.push_back(lookup[(val >> valb) & 0x3F]);
        valb -= 6;
      }
    }
  
    if (valb > -6) { out.push_back(lookup[((val << 8) >> (valb + 8)) & 0x3F]); }
  
    while (out.size() % 4) {
      out.push_back('=');
    }
    
    return out;
}

CGAT1400::CGAT1400()
{
    m_strConfigPath = GAT1400_CONFIG_FILE;
    m_bRuning.store(true);
    m_thread = std::thread(&CGAT1400::runKeepAlive, this);
}

CGAT1400::~CGAT1400()
{
    dlog_debug("CGAT1400::~CGAT1400()");
    m_bRuning.store(false);
    m_condition.notify_all();

    if (m_thread.joinable()) {
        m_thread.join();
    }
    dlog_debug("GAT1400 Destroyed");
}

int CGAT1400::init()
{
    dlog_debug("GAT1400 init");
    Network::Gat1400Client_S stParam;
    if (Convert::read_file(m_strConfigPath, stParam)) {
        dlog_debug("not found gat1400 config");
        return ERR_FREAD;
    }
    
    dumpInfo(stParam);

    if (!stParam.enableGat1400) {
        dlog_debug("GAT1400 disable");
        m_stParam = stParam;
        unRegister();
        return OK;
    }

    int ret = checkUpdateParam(stParam);
    if (ret) {
        dlog_debug("No update required")
        if (ret == OK_EXIST) {
            return OK;
        }
        return ret;
    }

    /* 需要处理 */
    /* 注销上次连接 */
    unRegister();

    /* 配置参数并注册 */
    m_stParam = stParam;
    Register();
    return OK;
}

int CGAT1400::deinit()
{

    return OK;
}

int CGAT1400::uploadFaces(security_faces_t &faces)
{
    if (m_enState != State_EN::CONNECTED) {
        dlog_debug("servers not registered");
        return OK_NOT_EXIST;
    }

    if (!m_stParam.enableFace) return ERR_NOT_ENABLED;
    // 配置相关信息
    updateFacesInfo(faces);
    int status = m_stInterface.add_faces(faces);
    if (status) {
        dlog_debug("add faces faild %d", status);
        return status;
    }

    return status;
}

int CGAT1400::uploadPersons(security_persons_t &persons)
{
    if (m_enState != State_EN::CONNECTED) {
        dlog_debug("servers not registered");
        return OK_NOT_EXIST;
    }

    if (!m_stParam.enablePerson) return ERR_NOT_ENABLED;

    // 配置相关信息
    for (auto &person : persons) {
        std::string time = TimeUtils_NS::get_currentDate() + TimeUtils_NS::get_currentTime();
        
        person.SourceID = toBasicObjectId(BASIC_OBJECT_TYPE_IMAGE, time);
        person.PersonID = toImageCntObjectID(person.SourceID, IMAGE_CNT_OBJECT_TYPE_PERSON);
        person.DeviceID = m_stParam.deviceID;
        person.PersonAppearTime = time;
        person.PersonDisAppearTime = time;
        person.LocationMarkTime = time;

        /* VIID_2018 版本新增字段 */
        if (!m_stParam.version.compare("VIID_2018")) {
            person.ShotTime = time;
        }

        for (auto &image_info : person.SubImageList) {
            image_info.ImageID = toBasicObjectId(BASIC_OBJECT_TYPE_IMAGE, time);
            image_info.DeviceID = m_stParam.deviceID;
            image_info.ShotTime = time;
            image_info.Data = base64_encode(image_info.Data);
        }

        person.PointInfo.pointType = m_stParam.pointType;
        if (!person.SubImageList.empty()) {
            person.PointInfo.srcWidth = person.SubImageList[0].Width;
            person.PointInfo.srcHeight = person.SubImageList[0].Height;
        }
    }

    int status = m_stInterface.add_persons(persons);
    if (status) {
        dlog_debug("add persons faild %d", status);
        return status;
    }
    return status;
}

int CGAT1400::uploadNonmotorvehicles(security_nonmotorvehicles_t &nonmotorvehicles)
{
    if (m_enState != State_EN::CONNECTED) {
        dlog_debug("servers not registered");
        return OK_NOT_EXIST;
    }
    if (!m_stParam.enableNonmotorvehicle) return ERR_NOT_ENABLED;

    for (auto &nomotor : nonmotorvehicles) {
        std::string time = TimeUtils_NS::get_currentDate() + TimeUtils_NS::get_currentTime();
        
        nomotor.SourceID = toBasicObjectId(BASIC_OBJECT_TYPE_IMAGE, time);
        nomotor.NonMotorVehicleID = toImageCntObjectID(nomotor.SourceID, IMAGE_CNT_OBJECT_TYPE_NONMOTOR_VEHICLE);
        nomotor.DeviceID = m_stParam.deviceID;
        nomotor.AppearTime = time;
        nomotor.DisappearTime = time;
        nomotor.MarkTime = time;

        /* VIID_2018 版本新增字段 */
        if (!m_stParam.version.compare("VIID_2018")) {
            nomotor.ShotTime = time;
        }

        for (auto &image_info : nomotor.SubImageList) {
            image_info.ImageID = toBasicObjectId(BASIC_OBJECT_TYPE_IMAGE, time);
            image_info.DeviceID = m_stParam.deviceID;
            image_info.ShotTime = time;
            image_info.Data = base64_encode(image_info.Data);
        }

        nomotor.PointInfo.pointType = m_stParam.pointType;
        if (!nomotor.SubImageList.empty()) {
            nomotor.PointInfo.srcWidth = nomotor.SubImageList[0].Width;
            nomotor.PointInfo.srcHeight = nomotor.SubImageList[0].Height;
        }
    }

    int status = m_stInterface.add_nonmotorvehicles(nonmotorvehicles);
    if (status) {
        dlog_debug("add nonmotorvehicles faild %d", status);
        return status;
    }
    return status;
}

int CGAT1400::uploadMotorvehicles(security_motorvehicles_t &motorvehicles)
{
    if (m_enState != State_EN::CONNECTED) {
        dlog_debug("servers not registered");
        return OK_NOT_EXIST;
    }
    if (!m_stParam.enableMotorvehicle) return ERR_NOT_ENABLED;

    // 配置相关信息
    for (auto &vehicle : motorvehicles) {
        std::string time = TimeUtils_NS::get_currentDate() + TimeUtils_NS::get_currentTime();
        
        vehicle.SourceID = toBasicObjectId(BASIC_OBJECT_TYPE_IMAGE, time);
        vehicle.MotorVehicleID = toImageCntObjectID(vehicle.SourceID, IMAGE_CNT_OBJECT_TYPE_MOTOR_VEHICLE);
        vehicle.DeviceID = m_stParam.deviceID;
        vehicle.AppearTime = time;
        vehicle.DisappearTime = time;
        vehicle.MarkTime = time;

        /* VIID_2018 版本新增字段 */
        if (!m_stParam.version.compare("VIID_2018")) {
            vehicle.ShotTime = time;
        }

        for (auto &image_info : vehicle.SubImageList) {
            image_info.ImageID = toBasicObjectId(BASIC_OBJECT_TYPE_IMAGE, time);
            image_info.DeviceID = m_stParam.deviceID;
            image_info.ShotTime = time;
            image_info.Data = base64_encode(image_info.Data);
        }

        vehicle.PointInfo.pointType = m_stParam.pointType;
        if (!vehicle.SubImageList.empty()) {
            vehicle.PointInfo.srcWidth = vehicle.SubImageList[0].Width;
            vehicle.PointInfo.srcHeight = vehicle.SubImageList[0].Height;
        }
    }

    int status = m_stInterface.add_motorvehicles(motorvehicles);
    if (status) {
        dlog_debug("add motorvehicles faild %d", status);
        return status;
    }
    return status;
}

void CGAT1400::updateFacesInfo(security_faces_t &faces)
{
    for (auto &face : faces) {
        std::string time = TimeUtils_NS::get_currentDate() + TimeUtils_NS::get_currentTime();
        
        face.SourceID = toBasicObjectId(BASIC_OBJECT_TYPE_IMAGE, time);
        face.FaceID = toImageCntObjectID(face.SourceID, IMAGE_CNT_OBJECT_TYPE_FACE);
        face.DeviceID = m_stParam.deviceID;
        face.FaceAppearTime = time;
        face.FaceDisAppearTime = time;
        face.LocationMarkTime = time;

        /* VIID_2018 版本新增字段 */
        if (!m_stParam.version.compare("VIID_2018")) {
            face.ShotTime = time;
        }

        for (auto &image_info : face.SubImageList) {
            image_info.ImageID = toBasicObjectId(BASIC_OBJECT_TYPE_IMAGE, time);
            image_info.DeviceID = m_stParam.deviceID;
            image_info.ShotTime = time;
            image_info.Data = base64_encode(image_info.Data);
        }
        face.PointInfo.pointType = m_stParam.pointType;
        if (!face.SubImageList.empty()) {
            face.PointInfo.srcWidth = face.SubImageList[0].Width;
            face.PointInfo.srcHeight = face.SubImageList[0].Height;
        }
    }
}

std::string CGAT1400::toBasicObjectId(const std::string strBasicObjectType, const std::string strTime)
{
    std::unique_lock<std::mutex> l(m_BasicMutex);
    std::string id = m_stParam.deviceID;
    id += strBasicObjectType;
    if (strTime.empty()) {
        id += TimeUtils_NS::get_currentDate() + TimeUtils_NS::get_currentTime();
    } else {
        id += strTime;
    }
    static int nSourceSeq = 0;
    char buffer[6];
    std::sprintf(buffer, "%05d", ++nSourceSeq);
    id += std::string(buffer);
    return id;
}

std::string CGAT1400::toImageCntObjectID(const std::string strBasicObectId, const std::string strImageCntObjectType)
{
    std::unique_lock<std::mutex> l(m_ImageMutex);
    std::string id = strBasicObectId;
    id += strImageCntObjectType;

    static int nImageSeq = 0;
    char buffer[6];
    std::sprintf(buffer, "%05d", ++nImageSeq);
    id += std::string(buffer);
    return id;
}

void CGAT1400::runKeepAlive()
{
    /* 注册失败等待重连时间 */
    long long l_RegisterFaildWaitMs = 60 * 1000;
    /* 保活间隔时间 */
    long long l_KeepAliveDruationMs = 60 * 1000;
    while (m_bRuning.load()) {

        /* 未注册状态，阻塞等待 */
        if (m_enState == State_EN::DISCONNECTED) {
            std::unique_lock<std::mutex> l(m_mutex);
            m_condition.wait(l);
        }
        
        if (m_enState == State_EN::CONNECTED) {
            /* 标准：在90s内未交互信息则进行心跳保活 */
            /* 由于部分平台策略不一样，这里采用60s */
            long long l_duration = TimeUtils_NS::get_currentTimestampMs() - m_lLastTime;
            if (l_duration < l_KeepAliveDruationMs) {
                long long l_sleep = std::min<long long>(l_KeepAliveDruationMs - l_duration, 5 * 1000);
                this_thread::sleep_for(std::chrono::milliseconds(l_sleep));
                continue;
            }

            keepAlive();
            m_lLastTime = TimeUtils_NS::get_currentTimestampMs();
        }

        /* 重新注册 */
        /* 300s内随机重新注册,这里固定60s */
        if (m_enState == State_EN::RECONNECTING) {
            dlog_debug("reconnecting...");
            init();
            if (m_enState == State_EN::RECONNECTING) {
                dlog_debug("reconnect faild. wait 60s");
                this_thread::sleep_for(std::chrono::milliseconds(l_RegisterFaildWaitMs));
            }
        }
    }
}

int CGAT1400::checkUpdateParam(const Network::Gat1400Client_S &stParam)
{
    // 比较参数是否合法
    if (stParam.ip.empty()
        || stParam.deviceID.empty()
        || stParam.name.empty()
        || stParam.passwd.empty()
        || stParam.port <= 0) {
        dlog_debug("parm parse faild");
        return ERR_PARAM;
    }

    // 处于未连接状态，无需后续检查
    if (m_enState != State_EN::CONNECTED) {
        dlog_debug("gat1400 server no connect, register");
        return OK;
    }

    // 比较配置是否相同
    if (m_stParam.deviceID.compare(stParam.deviceID)
        || m_stParam.ip.compare(stParam.ip)
        || m_stParam.name.compare(stParam.name)
        || m_stParam.passwd.compare(stParam.passwd)
        || m_stParam.port != stParam.port
        || m_stParam.keepAlive != stParam.keepAlive) {
            dlog_debug("config update, register");
            return OK;
        }
    
    // 上传选项，直接赋值
    m_stParam.enableFace = stParam.enableFace;
    m_stParam.enablePerson = stParam.enablePerson;
    m_stParam.enableMotorvehicle = stParam.enableMotorvehicle;
    m_stParam.enableNonmotorvehicle = stParam.enableNonmotorvehicle;
    m_stParam.keepAlive = stParam.keepAlive;
    m_stParam.pointType = stParam.pointType;
    m_stParam.version = stParam.version;
    return OK_EXIST;
}

void CGAT1400::getGat1400Config(Network::Gat1400Client_S &config)
{
    config = m_stParam;
    config.status = m_enState == State_EN::CONNECTED;
}

int CGAT1400::unRegister()
{
    /* 连接状态下，注销 */
    if (m_enState == State_EN::CONNECTED) {
        m_stInterface.sys_unregister();
    }
    m_enState = State_EN::DISCONNECTED;
    /* 发布连接状态 */
    publishStatus();
    return OK;
}

int CGAT1400::Register()
{
    /* 注册 */
    int ret = m_stInterface.sys_register(
        m_stParam.ip.c_str(),
        m_stParam.port,
        m_stParam.deviceID.c_str(),
        m_stParam.name.c_str(),
        m_stParam.passwd.c_str(),
        m_stParam.keepAlive
    );

    /* 注册失败，进入重连状态 */
    if (ret) {
        dlog_debug("register faild %d", ret);
        m_enState = State_EN::RECONNECTING;
    }
    /* 注册成功 */ 
    else {
        m_lLastTime = TimeUtils_NS::get_currentTimestampMs();
        m_enState = State_EN::CONNECTED;
    }

    m_condition.notify_all();
    /* 发布连接状态 */
    publishStatus();
    return OK;
}

int CGAT1400::keepAlive()
{
    int status = m_stInterface.sys_keepalive();
    if (status != OK) {
        // 保活失败。重新注册
        dlog_debug("keepalive faild. re-register");
        m_enState = State_EN::RECONNECTING;
    }
    // 发布连接状态
    publishStatus();
    return ERR;
}

void CGAT1400::dumpInfo(Network::Gat1400Client_S &info)
{
    dlog_debug("Gat1400 info :%s", Convert::to_string(info).c_str());
}

void CGAT1400::publishStatus()
{
    Network::Gat1400Client_S stParam;
    getGat1400Config(stParam);
    SecurityString info = Convert::to_string(stParam);
    if (!info.empty()) {
        /* 适配网页校验格式，主要增加Return字段。*/
        nlohmann::json j_msg;
        nlohmann::json j_info = nlohmann::json::parse(info);
        if (!j_info.is_object()) {
            dlog_debug("info format faild");
            return;
        }
        j_msg["ActionCode"] = AC_GET_GAT1400_INFO;
        j_msg["DeviceName"] = "";
        j_msg["UserName"] = "";
        j_msg["Return"] = 0;
        j_msg["Data"] = j_info;
        SecurityString msg = j_msg.dump();
        TaskPublish::instance()->message(AC_GET_GAT1400_INFO, msg.c_str(), msg.size());
    }
}

}