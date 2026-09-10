/**
 * @FilePath     : register_manage.h
 * @Author       : huangjunda
 * @Date         : 2026-01-21 11:12:16
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-03-31 11:29:43
 * @Description  : 注册管理
 */

#pragma once

#include "register_define.h"

#include <climits>
#include <atomic>
#include <thread>
#include <condition_variable>

#include "Singleton.h"
#include "IpcRet.h"
#include "dlog.h"
#include "path_define.h"

/* 激活永久时长 */
#define PERMANENT_TIMER LLONG_MAX

/* 机器码长度 */
#define MACHINE_HEAD_LENGTH 2
#define MACHINE_COND_LENGTH 20

/* 总共激活时间枚举数量为六个 */
#define ACTIVATION_TIME_MAX_NUM 6

/* 是否按运行时长检测 0-按照现实时长检测 1-按照运行时长检测*/
#define IS_RUNING_TIME_CHECK_MODE 0

/* 激活/注册码最大长度 */
#define ODE_MAX_LENGTH 32

#if IS_RUNING_TIME_CHECK_MODE
/* 定时校验时间 -单位 分钟 */
#define TIMEROUT_TIME (1)
#else
/* 定时校验时间 -单位 分钟 */
#define TIMEROUT_TIME (60)
#endif

/*分钟*/
#define TIMEROUT_UNIT (60 * 1000)

/* 计算最小值 */
#define MIN(x, y) ((x) > (y) ? (y) : (x))

/* 二层加密算法 */
#define TWO_EA(x) ((x) * 2 + 2)

class CRegisterManage : public CSingleton<CRegisterManage>
{
    CRegisterManage();

public:
    virtual ~CRegisterManage() = default;
    friend class CSingleton<CRegisterManage>; /* 允许 Singleton 访问私有构造函数 */

    /**
     * @brief 初始化注册配置
     * @return IpcRet_E
     */
    IpcRet_E init();

    /**
     * @brief 去初始化注册配置
     * @return IpcRet_E
     */
    IpcRet_E deinit();

    /**
     * @description: 激活/注册设备
     * @param [string] strRegisterCode: 机器码
     * @return [*] IpcRet_E::OK 成功  其他失败
     * @others:
     */
    IpcRet_E register_device(std::string strRegisterCode);
    /**
     * @description: 获取注册信息
     * @param [RegisterInfo_S] &strInfo: 注册信息
     * @return [*] IpcRet_E::OK 成功  其他失败
     * @others:
     */
    IpcRet_E get_register_info(Register::RegisterInfo_S &strInfo);

private:
    /**
     * @description: 初始化定时器
     * @return [*] IpcRet_E::OK 成功  其他失败
     * @others: 不采用线程轮询检查状态，退出的时候不能马上退出，所以改用定时器
     */
    IpcRet_E init_timer();

    /**
     * @description: 初始化配置文件
     * @return [*] IpcRet_E::OK 成功  其他失败
     * @others: 
     */    
    IpcRet_E init_config();

    /**
     * @brief   : 从主文件或备份文件加载注册信息
     * @param    {Register::RegisterInfo_S &} stInfo：输出的激活/注册信息
     * @return   {IpcRet_E} OK：成功，OK_NOT_EXIST：主文件和备份文件都不存在，其他：失败
     * @note    : 主文件读取失败时，自动回退到备份文件，避免空文件或坏文件触发误清注册码
     */
    IpcRet_E load_register_info(Register::RegisterInfo_S &stInfo);

    /**
     * @description: 定时器超时-检查激活/注册信息
     * @param [void*] pArgv: 自定义参数
     * @return [*]
     * @others: 
     */
    static int check_registerInfo(void* pArgv);

    /**
     * @brief 定时器线程
     */
    void timer_thread();

    /**
     * @description: 将Json数据转化为结构体信息
     * @param [RegisterInfo_S&] stInfo: 激活/注册信息
     * @param [char*] pJsonBuf: 需要转换的Json数据
     * @return [*] IpcRet_E::OK 成功  其他失败
     * @others:
     */
    IpcRet_E conver_jsonToStruct(Register::RegisterInfo_S &stInfo, char *pJsonBuf);

    /**
     * @description: 将结构体信息转化为Json数据
     * @param [RegisterInfo_S&] stInfo: 激活/注册信息
     * @param [char**] pOutJsonBuf: 转化为的Json数据
     * @return [*] IpcRet_E::OK 成功  其他失败
     * @others:
     */
    IpcRet_E conver_structToJson(Register::RegisterInfo_S &stInfo, char **pOutJsonBuf);

    /**
     * @description: 读取激活/注册信息配置文件
     * @param [RegisterInfo_S&] stInfo: 激活/注册信息
     * @param [string] strFilePath: 文件路径
     * @return [*] IpcRet_E::OK 成功  其他失败
     * @others:
     */
    IpcRet_E read_registerJson(Register::RegisterInfo_S &stInfo, std::string strFilePath);

    /**
     * @description: 写入激活/注册信息配置文件
     * @param [RegisterInfo_S&] stInfo: 激活/注册信息
     * @param [string] strFilePath: 文件路径
     * @return [*] IpcRet_E::OK 成功  其他失败
     * @others:
     */
    IpcRet_E write_registerJson(Register::RegisterInfo_S &stInfo, std::string strFilePath);

    /***
     * @description: 创建路径中的目录
     * @author     : huangjunda
     * @param       {char*} pchFilePath
     * @return      {*}
     */
    IpcRet_E create_file_path(const char *pchFilePath);

    /**
     * @description: 检查激活/注册码是否被使用
     * @param [string] strRegisterCode: 激活/注册码
     * @return [*]OK_EXIST-已存在 其他不存在
     * @others:
     */
    IpcRet_E check_code_used(std::string strRegisterCode);

    /**
     * @description: 激活/注册设备并获取激活/注册时间类型
     * @param [string] strMachineCode: 机器码
     * @param [string] strRegisterCode: 激活/注册码
     * @param [ActivationTime_E&] enActionTime: 激活成功-激活的时间类型
     * @return [*] IpcRet_E::OK 成功  其他失败
     * @others:
     */
    IpcRet_E set_register(std::string strMachineCode, std::string strRegisterCode, Register::ActivationTime_E &enActionTime);

    /**
     * @description: 生成激活/注册码
     * @param [string] strMachinSn: 机器码
     * @param [ActivationTime_E] emTime: 激活/注册时间类型
     * @param [string&] strRegisterCode: 生成的激活/注册码
     * @return [*] IpcRet_E::OK 成功  其他失败
     * @others:
     */
    IpcRet_E create_register_code(std::string strMachinSn, Register::ActivationTime_E enTime, std::string &strRegisterCode);

    /**
     * @description: 记录激活/注册码
     * @param [string] strRegisterCode: strRegisterCode：激活/注册码
     * @return [*]  IpcRet_E::OK 成功  其他失败
     * @others: 将激活/注册码加到管理ini文件中去
     */
    IpcRet_E write_register_code(std::string strRegisterCode);

    /**
     * @description: 设置注册信息
     * @param [RegisterInfo_S&] stInfo: 注册信息
     * @return [*] IpcRet_E::OK 成功  其他失败
     * @others:
     */
    IpcRet_E set_register_info(Register::RegisterInfo_S &stInfo);

    /**
     * @description: 获取机器码
     * @param [char] *pchMachinSn: 机器码
     * @param [char] *pchDevID: 设备ID
     * @return [*] IpcRet_E::OK 成功  其他失败
     * @others:
     */
    IpcRet_E get_machine_code(char *pchMachinSn, char *pchDevID);

    /**
     * @description: 获取CPU信息
     * @param [std::string] &strCpuId: CPUID
     * @param [std::string] &strMachineCode: 机器码
     * @return [*] IpcRet_E::OK 成功  其他失败
     * @others:
     */
    IpcRet_E get_cpu_info(std::string &strCpuId, std::string &strMachineCode);

    /* 当前激活/注册信息 */
    Register::RegisterInfo_S m_stRegister;

    /* 互斥锁 */
    std::mutex m_registerMutex;

    std::thread m_thread;
    std::atomic<bool> m_bStopFlag;
    // std::mutex m_mutex;
    // std::condition_variable m_cv;
};
