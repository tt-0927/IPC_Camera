/*
 * @FilePath     : PlatformManage.hpp
 * @Author       : 李辉 lih@kfb.cn
 * @Date         : 2024-04-02 20:02:08
 * @LastEditors: weigl 308241951@qq.com
 * @LastEditTime: 2024-10-30 14:49:25
 * @Description  :
 */

#pragma once

#include <mutex>

#include "AiPlatformBase.hpp"
#include "BlError.h"
#include "dlog.h"
#include "ParseData.hpp"
#include "PlatformExtern.hpp"

namespace PlatformManage_NS
{
    class CPlatformManage : public CAiPlatformBase
    {
    public:

        CPlatformManage(AiPlatformInParam_S stInParam);
        ~CPlatformManage();

        /**
         * @description: 平台-获取Token值
         * @param [void*] pArgv: 自定义参数
         * @return [*]
         * @others:
         */
        BlError_E get_tokenInfo(void* pArgv = nullptr);

        /**
         * @description: 初始化函数
         * @return [*] BlError_E::OK 成功  其他失败
         * @others:
         */
        BlError_E init();

        /**
         * @description: 平台-获取班级信息
         * @param [void*] pArgv: 自定义参数
         * @return [*]
         * @others:
         */
        BlError_E get_classInfo(PlatformManage_NS::DataInfo_S& stDataInfo);

        /**
         * @description: 获取教育云平台班级信息接口
         * @param [void*] pArgv: 自定义参数
         * @return [*]
         * @others:
         */
        BlError_E get_platformClassInfo(PlatformManage_NS::DataInfo_S& stDataInfo);

        /**
         * @description: 平台-校验班级成员信息是否改变
         * @param [PlatformManage_NS::DataInfo_S] stDataInfo: 教室信息结构体
         * @return [*]
         * @others:
         */
        BlError_E check_dataChanged(PlatformManage_NS::DataInfo_S stDataInfo);

        /**
         * @description: 平台-班级成员信息
         * @param [PlatformManage_NS::DataInfo_S&] stDataInfo: 教室信息结构体
         * @return [*]
         * @others:
         */
        BlError_E get_humanInfo(PlatformManage_NS::DataInfo_S& stDataInfo);

        /**
         * @description: 平台-下载人脸图片信息
         * @param [void*] pArgv: 自定义参数
         * @return [*]
         * @others:
         */
        BlError_E get_downloadPicInfo(PlatformManage_NS::DataInfo_S& stDataInfo);

        /**
         * @description: 平台-人脸图片压缩包
         * @param [void*] pArgv: 自定义参数
         * @return [*]
         * @others:
         */
        BlError_E get_humanPic();

        /**
         * @description: 获取路径所属父目录
         * @param [*]
         * @return [*]
         * @others:
         */
        std::string get_pathBase();

    private:

        /**
         * @description: 判断md5值是否存在
         * @param [const std::list<HumanInfo_S>&] list 信息列表
         * @param [const std::string&] strMd5ToFind 需查找的MD5值
         * @return [*]
         * @others:
         */
        bool Md5Exists(const std::list<HumanInfo_S>& list, const std::string& strMd5ToFind);

        /**
         * @brief 获取文件后缀
         * @param [string&] strFilePath: 文件路径
         * @return [*]
         * @note
         */
        std::string getFileExtension(const std::string& strFilePath);

    private:

        /* token值 */
        std::string m_strToken;

        /* 解析处理类 */
        ParseData_NS::CParseBase* m_pParseBase = nullptr;

        /* 路径所属父目录 */
        std::string m_strPathBase = "/";
    };
}    // namespace PlatformManage_NS
