#ifndef MAINTENANCEDATAV2_H
#define MAINTENANCEDATAV2_H

#include "common/MaintenanceStruct.h"
#include "common/MaintenanceJsonParse.h"

namespace MaintenanceNS
{

    class CMaintenanceData
    {
    public:
        static CMaintenanceData *getInstance();

    public:
        /**
         * @brief 初始化
         * @param [string] 配置文件路径
         * @return [bool] 成功返回true
         */
        bool init(const std::string &strConfigPath);
        /**
         * @brief 初始化
         * @param [string] 配置信息Json
         * @return [bool] 成功返回true
         */
        bool init(std::string &strConfigureJson);
        /**
         * @brief 初始化配置信息
         */
        void init_config();
        /**
         * @brief 是否已经初始化成功
         * @return [bool] 已初始化返回true
         */
        bool isInit();

        /**
         * @brief 获取配置信息
         * @return [MaintenanceManagerConf] 配置信息结构体
         */
        MaintenanceManagerConf getConfig();
        /**
         * @brief 获取配置信息中的 HTTP 服务地址
         * @return [string] http请求地址
         */
        std::string getRequeryUrl();
        /**
         * @brief 设置配置信息中的 设备唯一ID
         * @param [string] 设备唯一ID
         */
        void setDeviceCode(std::string strDeviceCode);
        /**
         * @brief 获取配置信息中的 设备唯一ID
         * @return [string] 设备唯一ID
         */
        std::string getDeviceCode();
        /**
         * @brief 设置配置信息中的项目唯一码
         * @param [string] 项目唯一码
         */
        void setProjectCode(std::string strProjectCode);
        /**
         * @brief 获取配置信息中的项目唯一码
         * @return [string] 项目唯一码
         * @note 项目唯一码，用于匹配服务器中查询下来项目码
         *       进行比较，从而获取项目的ID
         */
        std::string getProjectCode();
        /**
         * @brief 获取配置信息中的日志文件存放路径列表
         * @return [vector<string>] 日志文件存放路径列表
         */
        std::vector<std::string> getFilesPath();
        /**
         * @brief 获取配置信息中的记录文件存放路径
         * @return [string] 记录文件存放路径
         */
        std::string getRecordFilePath();
        /**
         * @brief 获取当前系统的时间字符串
         * @return [string ] 当前系统的时间字符串，格式为: yyyy-MM-dd
         */
        std::string getCurDate();
        /**
         * @brief 获取当前系统的时间字符串
         * @return [string ] 当前系统的时间字符串，格式为: yyyy_MM_dd
         */
        std::string getCurDateBySeparatorIs_();

        /**
         * @brief 设置 记录文件 的全路径，包括文件名
         * @param [string] 记录文件路径
         * @note 当m_bNextDate为false赋值给m_strRecordFilePath，
         *      反之赋值给m_strRecordFilePathTmp
         */
        void setRecordFileFullPath(const std::string &strFileFullPath);
        /**
         * @brief 设置 过滤文件 的全路径，包括文件名
         * @param [string] 过滤文件路径
         */
        void setFilterFileFullPath(const std::string &strFileFullPath);

        /**
         * @brief 获取 记录文件 读取后的数据
         * @param [bool] 是否从新读取，true：从新读取， false: 返回历史记录
         * @return [std::vector<RecordInfo>] 记录文件信息数组
         */
        std::vector<RecordInfo> getRecordInfos(bool bReParse = false);
        /**
         * @brief 获取 过滤文件 读取后的数据
         * @param [bool] 是否从新读取，true：从新读取， false: 返回历史记录
         * @return [std::vector<std::string>] 过滤文件信息数组
         */
        std::vector<std::string> getFilterInfos(bool bReParse = false);

        /**
         * @brief 设置登录状态
         * @param [bool] 状态，true：登录成功，Token有效；
         *                    false：登录失败，Token无效
         */
        void setLoginStatus(bool bStatus);
        /**
         * @brief 获取登录状态
         * @return [bool] 状态，true：登录成功，Token有效
         *                     false：登录失败，Token无效
         */
        bool getLoginStatus();

        /**
         * @brief 设置Token
         * @param [string] Token
         */
        void setToken(const std::string &strToken);
        /**
         * @brief 从文件中读取Token并覆盖
         * @param [string] Token的文件路径
         * @note 用于测试Token失效的情况
         */
        void setApiTokenFile(const std::string &strFilePath);

        /**
         * @brief 获取Token
         * @return [string] Token
         * @note 当 m_bIsLogin 为true的时候，该m_strToken才是有效的
         */
        std::string getToken();

        /**
         * @brief 设置项目唯一ID
         * @param [int] 项目唯一ID
         */
        void setProjectID(const int &nID);
        /**
         * @brief 获取项目唯一ID
         * @return [int] 项目唯一ID
         * @note 当 m_bIsLogin 为true的时候，项目唯一ID才是有效的
         */
        int getProjectID();

        /**
         * @brief 将下一天状态设置为true，并根据情况情况map
         * @note 如果map中的文件全部上传成功，写出过滤文件
         *       如果没有上传完成，那么等待最后一条上传成功，再写出过滤文件
         */
        void nextDate();
        /**
         * @brief 获取下一天状态
         * @return [bool] true：已经是下一天了
         */
        bool getNextDateStatus();

        /**
         * @brief 更改m_mapUploadRecord中对应的文件唯一ID的上传状态为enStatus的值
         * @param [string]  文件唯一ID
         * @param [UploadStatus] 上传状态
         */
        void changedRecordUploadStatus(const std::string &strIdentifier,
                                       const MaintenanceNS::UploadStatus &enStatus);

        /**
         * @brief 打印输出所有的容器中的内容
         * @note 用于调试
         */
        void printfAllContainers();

    public:
        /**
         * @brief 设置文件信息
         * @param [FileInfo] 文件信息
         * @note 当 m_mapUploadRecord 中和历史记录中不存在该文件信息
         *       则将当前信息添加到 m_mapUploadRecord 中
         */
        void setFileInfoCheckRecord(FileInfo &stFileInfo);

        /**
         * @brief 获取一条需要上传的记录信息
         * @return [RecordInfo] 记录信息
         * @note 这个函数从m_mapUploadRecord查找，第一条上传状态为UPLOAD_NOT的数据
         */
        RecordInfo getNeedUploadFile();

    private:
        /**
         * @brief 根据strFormat格式获取的日期字符串的内联函数
         * @param [string] 日期格式
         * @return [string] 日期字符串
         */
        inline std::string getStringDate(const char *strFormat);

        /**
         * @brief 递归创建文件夹
         * @param [string] 文件路径
         * @return [bool] 创建成功返回true
         */
        bool createDirectory(const std::string &strPath);
        /**
         * @brief 读取文件内容
         * @param [string] 文件路径
         * @return [string] 文件内容
         */
        std::string readFile(std::string &strFile);
        /**
         * @brief 解析配置文件
         * @return [bool] 解析成功返回true
         */
        bool parseConfigFile();
        /**
         * @brief 解析配置信息Json
         * @return [bool] 解析成功返回true
         */
        bool parseConfigByJson(std::string strConfigureJson);
        /**
         * @brief 解析上传历史记录文件
         * @return [bool] 解析成功返回true
         */
        bool parseRecordFile();
        /**
         * @brief 解析过滤日期文件
         * @return [bool] 解析成功返回true
         */
        bool parseFilterFile();

        /**
         * @brief 写出文件
         * @param [string] 文件路径
         * @param [cahr*] 写出的内容
         * @param [size_t] 写出的大小
         * @return [bool] 写出成功返回true
         */
        bool wirteFile(std::string &strFile, char *buffer, std::size_t size);
        /**
         * @brief 写出上传历史记录文件
         * @param [RecordInfo] 一条待写出的记录文件信息
         * @return [bool] 写出成功返回true
         */
        bool writeRecordFile(const RecordInfo &stRecordInfo);
        /**
         * @brief 写出过滤日期文件
         * @param [set<string>] 过滤日期set容器
         * @return [bool] 写出成功返回true
         */
        bool writeFilterFile(std::set<std::string> &setFilter);

    private:
        /* 是否初始化 */
        bool m_bIsInit = false;
        /* 解析Json */
        CMaintenanceJsonParse m_cParse;
        /* 配置文件的路径，包含文件名称 */
        std::string m_strConfigFilePath;
        /* 运维平台用户的配置 */
        MaintenanceManagerConf m_stConfig;
        std::shared_mutex m_configMutex;

    private:
        /* 记录文件的路径，包含文件名称 */
        std::string m_strRecordFilePath;
        /* 记录文件的路径，包含文件名称，
         * 下一天未上传完记录时会给赋值 */
        std::string m_strRecordFilePathTmp;
        /* 过滤文件的路径，包含文件名称 */
        std::string m_strFilterFilePath;

        /* 读取历史文件的记录列表，防止中途掉电重复上传以上传 */
        std::vector<RecordInfo> m_vecRecord;
        std::string m_strRecordJson;
        std::shared_mutex m_recordMutex;

        /* 记录已经上传完毕的日期，昨日的上传完毕将会写出昨日日期至该文件 */
        std::vector<std::string> m_vecFilter;
        std::shared_mutex m_filterMutex;

        /* 是否进入下一天，true：进入下一天 */
        bool m_bNextDate = false;

    private:
        /* 是否登录的标志 */
        bool m_bIsLogin = false;
        std::shared_mutex m_loginMutex;

        /* 当前Http通信的Token */
        std::string m_strToken;
        std::shared_mutex m_tokenMutex;

        /* 当前项目对应的 project id */
        int m_nProjectID = -1;
        std::shared_mutex m_projectIDMutex;

    private:
        /* 上传记录，该map只记录自程序启动后的记录 */
        std::map<std::string, RecordInfo> m_mapUploadRecord;
        std::shared_mutex m_uploadRecordMutex;

    private:
        CMaintenanceData();
        static CMaintenanceData *m_pThis;
        static std::mutex m_thisMutex;
    };

}

#endif // MAINTENANCEDATAV2_H
