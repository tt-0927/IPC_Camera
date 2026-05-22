/**
 * @FilePath     : mqtt_sdk_gateway.h
 * @Description  : MQTT-SDK 命令转发网关，将 MQTT 命令桥接到 CTaskManage 执行
 * @Note         : 复用 tvsdk_callbacks.cpp 中的 execute_get_result 同步执行模式，
 *                 通过 CTaskManage::execute() 将 SDK 命令转发到 IPC 业务层
 */

#pragma once

#include <string>
#include <functional>

class CTaskManage;

/**
 * @brief MQTT-SDK 命令网关
 * @note  静态工具类，提供 MQTT 命令到 IPC 业务层的桥接能力
 *        · 命令名 → SDK 命令码 → ActionCode 的解析链
 *        · 通过 CTaskManage::execute() 同步执行 GET/SET 命令
 *        · 线程安全，可从 MQTT 回调线程直接调用
 */
class CMqttSdkGateway
{
public:
    /**
     * @brief  : 设置 CTaskManage 实例指针
     * @param  {CTaskManage *} pTaskManage：任务管理器指针
     * @note   : 在 ControlManage::init_server() 中调用
     */
    static void set_task_manage(CTaskManage *pTaskManage);

    /**
     * @brief  : 执行 MQTT GET 命令（同步等待结果）
     * @param  {const std::string &} strCommand：SDK 命令名（如 "NET_TV_GET_FACECAPTUREINFO"）
     * @param  {const std::string &} strData：请求数据（JSON）
     * @param  {std::string &} strResult：输出结果（JSON）
     * @return {int} 0：成功，非0：失败
     * @note   : 内部解析命令名 → ActionCode，通过 CTaskManage 同步执行并捕获结果
     */
    static int execute_get(const std::string &strCommand, const std::string &strData, std::string &strResult);

    /**
     * @brief  : 执行 MQTT SET 命令
     * @param  {const std::string &} strCommand：SDK 命令名（如 "NET_TV_SET_FACECAPTUREINFO"）
     * @param  {const std::string &} strData：请求数据（JSON）
     * @return {int} 0：成功，非0：失败
     * @note   : 内部解析命令名 → ActionCode，通过 CTaskManage 执行
     */
    static int execute_set(const std::string &strCommand, const std::string &strData);

    /**
     * @brief  : 判断命令是否为 GET 类型
     * @param  {const std::string &} strCommand：SDK 命令名
     * @return {bool} true：GET 命令，false：SET 或未知命令
     */
    static bool is_get_command(const std::string &strCommand);

    /**
     * @brief  : 判断命令是否已注册支持
     * @param  {const std::string &} strCommand：SDK 命令名
     * @return {bool} true：已注册，false：不支持
     */
    static bool is_command_supported(const std::string &strCommand);

private:
    /**
     * @brief  : 标准化命令名（去空格、转大写）
     * @param  {const std::string &} strCommand：原始命令名
     * @return {std::string} 标准化后的命令名
     */
    static std::string normalize_command_name(const std::string &strCommand);

    /**
     * @brief  : 根据命令名查找 SDK 命令码
     * @param  {const std::string &} strCommand：标准化后的命令名
     * @return {int} SDK 命令码，未找到返回 0
     */
    static int resolve_sdk_command(const std::string &strCommand);

    /**
     * @brief  : 将 SDK 命令码映射为设备内部 ActionCode
     * @param  {int} nSdkCommand：SDK 命令码
     * @return {int} 内部 ActionCode，未配置映射返回 0
     */
    static int sdk_command_to_action_code(int nSdkCommand);

    /**
     * @brief  : 通过 CTaskManage 同步执行 GET 命令并获取结果
     * @param  {int} nActionCode：内部 ActionCode
     * @param  {const std::string &} strData：请求数据（JSON）
     * @param  {std::string &} strResult：输出结果（JSON）
     * @return {int} 0：成功，非0：失败
     */
    static int execute_get_result(int nActionCode, const std::string &strData, std::string &strResult);

    /**
     * @brief  : 通过 CTaskManage 执行 SET 命令
     * @param  {int} nActionCode：内部 ActionCode
     * @param  {const std::string &} strData：请求数据（JSON）
     * @return {int} 0：成功，非0：失败
     */
    static int execute_set_action(int nActionCode, const std::string &strData);
};
