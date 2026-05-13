/*
 * @FilePath     : MaintenanceJsonParse.h
 * @Author       : xiezhh
 * @Date         : 2024-06-04 17:23
 * @LastEditors  : xiezhh
 * @LastEditTime : 2024-06-25 15:24
 * @Description  : 运维管理上传日志和配置的解析json类
 */
#ifndef CMAINTENANCEJSONPARSE_H
#define CMAINTENANCEJSONPARSE_H

#include "common/MaintenanceStruct.h"
#include "Json.h"

class CMaintenanceJsonParse
{
public:
    CMaintenanceJsonParse();

    /**
     * @brief 解析配置文件
     * @param [string] 文件Json内容
     * @return [MaintenanceManagerConf] 配置信息结构体
     */
    MaintenanceNS::MaintenanceManagerConf parseConfig(std::string &strJson);
    /**
     * @brief 解析注册激活文件中的机器码
     * @param [string] 文件Json内容
     * @return [string] 唯一机器码
     */
    std::string parseRegisterJson(std::string &strJson);
    /**
     * @brief 解析历史上传记录文件
     * @param [string] 文件Json内容
     * @return [vector<RecordInfo>] 历史上传记录列表
     */
    std::vector<MaintenanceNS::RecordInfo> parseRecordFileData(std::string &strJson);
    /**
     * @brief 解析过滤日期记录文件
     * @param [string] 文件Json内容
     * @return [vector<string>] 过滤日期记录列表
     */
    std::vector<std::string> parseFilterFileData(std::string &strJson);

    /**
     * @brief 解析登录请求结果
     * @param [string] 结果信息
     * @param [bool][out] 解析的结果, true: 成功， false: 失败
     * @return [LoginResult] 登录请求结果结构体
     */
    MaintenanceNS::LoginResult parseLoginResult(std::string &strRequeryResult, bool &bRet);
    /**
     * @brief 解析查询项目列表请求结果
     * @param [string] 结果信息
     * @param [bool][out] 解析的结果, true: 成功， false: 失败
     * @return [ReqProjectResult] 查询项目列表请求结果结构体
     */
    MaintenanceNS::ReqProjectResult parseProjectResult(std::string &strRequeryResult, bool &bRet);
    /**
     * @brief 解析上传文件请求结果
     * @param [string] 结果信息
     * @param [bool][out] 解析的结果, true: 成功， false: 失败
     * @return [ReqUploadResult] 上传文件结果结构体
     */
    MaintenanceNS::ReqUploadResult parseUploadResult(std::string &strRequeryResult, bool &bRet);

    /**
     * @brief 创建待写出的历史上传记录的char*指针
     * @param [string][out] 历史信息记录Json字符串，插入新的记录后覆盖
     * @param [RecordInfo]  新的上传成功记录信息
     * @param [size_t &][out] 创建的char*指针大小
     * @return [char *] 待写出的历史上传记录的char*指针
     * @note 根据strJson创建Json::Object并将stRecordInfo
     *       按照格式插入对应位置后，将Json::Object的根节点
     *       转为char*并return
     *       返回值不等与 nullptr 或 NULL， nSize > 0说明成功
     */
    char *createRecordFileBuffer(std::string &strJson,
                                 const MaintenanceNS::RecordInfo &stRecordInfo, std::size_t &nSize);
    /**
     * @brief 创建待写出的过滤日期记录的char*指针
     * @param [vector<string>] 过滤日期列表
     * @param [size_t &][out] 创建的char*指针大小
     * @return [char *] 待写出的过滤日期记录的char*指针
     * @note 创建Json::Object作为根节点，根据vecFilter并将数据
     *       按照格式插入对应位置后，将Json::Object的根节点
     *       转为char*并return。
     *       返回值不等与 nullptr 或 NULL， nSize > 0说明成功
     */
    char *createFilterFileBuffer(std::vector<std::string> &vecFilter, std::size_t &nSize);

private:
    /**
     * @brief 解析Http接口通用的头固定字段
     * @param [Json::Object *] 头部分Json对象
     * @param [Result] 接口公用头固定字段结构体
     * @return [bool] true: 成功， false: 失败
     */
    bool parseHeader(Json::Object *pObj, MaintenanceNS::Result &header);

    /**
     * @brief 添加斜杠
     * @param [string] 内容
     */
    void addSplitChar(std::string &strData);

    /* 组装与control通信的Json信息，定义为static */
public:
    /**
     * @brief 创建获取设备信息的Json消息
     * @param [int] 指令
     * @param [int] 操作类型
     * @return [string] Json消息
     */
    static std::string createGetDeviceInfoJson(int nCode, int nOpt);

private:
    /* 登录请求结果结构体 */
    MaintenanceNS::LoginResult m_stLoginResult;
    /* 查询项目列表请求结果结构体 */
    MaintenanceNS::ReqProjectResult m_stReqProjectResult;
    /* 上传文件结果结构体 */
    MaintenanceNS::ReqUploadResult m_stReqUploadResult;
};

#endif // CMAINTENANCEJSONPARSE_H
