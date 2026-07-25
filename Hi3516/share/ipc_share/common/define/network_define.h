/**
 * @file network_define.h
 * @author zhangjc (zhangjc@kfb.cn)
 * @date 2024-10-14
 *
 * @brief
 */
#pragma once

#include <string>
#include <vector>

extern "C"
{
#include "share_define.h"
}

namespace Network
{

    /**
     * @brief 网卡类型
     */
    typedef enum
    {
        UNKON_TYPE              = -1, /* 未知类型 */
        AUTO                    = 0,  /* 自适应 */
        SPEED_10M_HALF_DUPLEX   = 1,  /* 10M半双工 */
        SPEED_10M_FULL_DUPLEX   = 2,  /* 10M全双工 */
        SPEED_100M_HALF_DUPLEX  = 3,  /* 100M半双工 */
        SPEED_100M_FULL_DUPLEX  = 4,  /* 100M全双工 */
        SPEED_1000M_HALF_DUPLEX = 5,  /* 1000M半双工 */
        SPEED_1000M_FULL_DUPLEX = 6   /* 1000M全双工 */
    } NetTypeMode_E;

    /**
     * @brief 检查mac地址（物理地址）规范信息
     */
    typedef struct
    {
        bool bMacValid = false; /* mac地址是否规范 */
    } CheckMacValid_S;

    /**
     * @brief ip信息
     */
    struct Ip_S
    {
        NetTypeMode_E enType           = AUTO;              /* 网卡类型 */
        bool          bEnableDhcp      = false;             /* 是否开启dhcp */
        std::string   netName          = "eth0";            /* 网卡名称：eth0 */
        std::string   ipv4Ip           = "192.168.1.168";   /* ipv4地址 */
        std::string   ipv4Mask         = "255.255.255.0";   /* ipv4掩码 */
        std::string   ipv4Gateway      = "192.168.1.254";   /* ipv4网关 */
        std::string   ipv6Ip;                               /* ipv6地址 */
        std::string   ipv6Gateway;                          /* ipv6网关 */
        std::string   physicalAddress;                      /* 物理地址 */
        int           nMtu             = 0;                 /* mtu大小 */
        bool          bEnableMulticast = true;              /* 是否开启多播搜索 */
        std::string   multicastAddress = "239.225.225.105"; /* 多播地址 */

        /* 重载赋值运算符 */
        Ip_S& operator=(const Ip_S& other)
        {
            if (this != &other)
            {
                enType = other.enType;
                bEnableDhcp = other.bEnableDhcp;
                netName = other.netName;
                ipv4Ip = other.ipv4Ip;
                ipv4Mask = other.ipv4Mask;
                ipv4Gateway = other.ipv4Gateway;
                ipv6Ip = other.ipv6Ip;
                ipv6Gateway = other.ipv6Gateway;
                physicalAddress = other.physicalAddress;
                nMtu = other.nMtu;
                bEnableMulticast = other.bEnableMulticast;
                multicastAddress = other.multicastAddress;
            }
            return *this;
        }
    };

    /**
     * @brief dns信息
     */
    struct Dns_S
    {
        bool        bEnableAutoDns = false; /* 是否开启自动获取DNS服务器 */
        std::string main;                   /* 首选dns */
        std::string standby;                /* 备选nds */
        /* 重载赋值运算符 */
        Dns_S& operator=(const Dns_S& other)
        {
            if (this != &other)
            {
                bEnableAutoDns = other.bEnableAutoDns;
                main = other.main;
                standby = other.standby;
            }
            return *this;
        }
    };

    /**
     * @brief 网络信息
     */
    struct Info_S
    {
        Ip_S  stIp;  /* ip信息 */
        Dns_S stDns; /* dns信息 */
        /* 重载赋值运算符 */
        Info_S& operator=(const Info_S& other)
        {
            if (this != &other)
            {
                stIp = other.stIp;
                stDns = other.stDns;
            }
            return *this;
        }
    };

    /**
     * @brief DDNS类型
     */
    typedef enum
    {
        QDNS_TYPE   = 0, /* 3322的ddns服务器 */
        DYNDNS_TYPE = 1  /* dyn的ddns服务器 */
    }DdnsTye_E;

    /**
     * @brief ddns信息
     */
    typedef struct
    {
        bool        bEnableDdns = false; /* 是否开启dhcp */
        int         nDdnsType   = 0;     /* DDNS服务器类型 */
        std::string address;             /* 服务器地址 */
        std::string realmName;           /* 域名 */
        std::string user;                /* 用户名 */
        std::string password;            /* 密码 */
        int         nStatus     = 0;     /* 状态：-1 连接失败 0 未启用 1 连接成功 */
    } Ddns_S;

    /**
     * @brief pppoe信息
     */
    typedef struct
    {
        bool        bEnablePppoe = false;     /* 是否开启pppoe */
        std::string strIp        = "0.0.0.0"; /* 拨号上网ip地址 */
        std::string strUser;                  /* 用户名 */
        std::string strPassword;              /* 密码 */
    } Pppoe_S;

    /**
     * @brief 端口配置
     */
    typedef struct
    {
        int nHttpPort   = 80;
        int nRtspPort   = 554;
        int nHttpsPort  = 443;
        int nServerPort = 8000;
        int nWebServerPort = 9000;
    } PortConfig_S;

    /**
     * @brief 端口类型枚举
     */
    typedef enum 
    {
       HTTP_PORT_TYPE = 0,  /* http端口类型 */
       RTSP_PORT_TYPE,
       HTTPS_PORT_TYPE,
       SERVER_PORT_TYPE,
       WEB_SERVER_PORT_TYPE
    }PortType_E;

    /**
     * @brief 端口映射信息
     */
    typedef struct
    {
        int         nPortType     = 0;         /* 端口类型：0 Http 1 Rtsp 2 Https 3 Server 4 PlatformCmd 5 PlatformData */
        int         nExternPort   = 0;         /* 外部端口 */
        std::string externIp      = "0.0.0.0"; /* 外部IP */
        int         nInternalPort = 0;         /* 内部端口 */
        int         nStatus       = 0;         /* 状态 */
    } PortMap_S;

    /**
     * @brief 端口映射配置
     */
    typedef struct
    {
        std::string            strName        = DEVICE_CODE; /* 别名 */
        bool                   bEnablePortMap = false;      /* 是否开启端口映射 */
        int                    nMapType       = 0;          /* 映射类型：0 手动 1 自动 */
        std::vector<PortMap_S> portMap;
    } PortMapConfig_S;

    /**
     * @brief 日志服务器
     */
    typedef struct
    {
        bool        bEnableLogServer = false; /* 是否开启日志服务器 */
        std::string address;                  /* 服务器地址 */
        int         nPort            = 0;     /* 端口 */
        int         nUploadInterval  = 0;     /* 上传间隔 */
    } LogServer_S;

    /**
     * @brief 认证方式
     */
    typedef enum
    {
        AUTH_MD5 = 0, /* MD5认证 */
        AUTH_SHA      /* SHA认证 */
    } AuthProtocol_E;

    /**
     * @brief 加密方式
     */
    typedef enum
    {
        PRIV_DES = 0, /* DES加密 */
        PRIV_AES      /* AES加密 */
    } PrivProtocol_E;

    /**
     * @brief  SNMPv3 安全设置结构体
     */
    typedef struct
    {
        AuthProtocol_E enAuth = AUTH_MD5; /* 认证方式 */
        std::string    strAuthPassword;   /* 鉴权密码 */
        PrivProtocol_E enPriv = PRIV_DES; /* 加密方式 */
        std::string    strPrivPassword;   /* 加密密码 */
    } SnmpSecuritySettings_S;

    /**
     * @brief SNMP配置信息结构体
     */
    typedef struct
    {
        bool        bEnableSnmp   = false; /* 开启/关闭SNMP  */
        bool        bEnableSmnpV3 = false; /* 开启/关闭V3版本 */
        std::string strReadCommunityName;  /* 读共同体名称 */
        std::string strWriteCommunityName; /* 写共同体名称 */
        std::string strTrapAddress;        /* Trap地址 */
        int         nTrapPort     = 162;   /* Trap端口 */
        int         nSnmpPort     = 161;   /* SNMP端口 */

        /*********SNMPv3版本配置信息************/
        std::string            strReadSecurityName;     /* 读安全名称 */
        SnmpSecuritySettings_S stReadSecuritySettings;  /* 读安全设置 */
        std::string            strWriteSecurityName;    /* 写安全名称 */
        SnmpSecuritySettings_S stWriteSecuritySettings; /* 写安全设置 */
    } SnmpConfig_S;
    /**
     * @brief smtp服务器信息
     */
    typedef struct
    {
        std::string strAddress; /* 地址 */
        int         nPort = 0;  /* 端口 */
    } SmtpServer_S;

    /**
     * @brief 邮件用户
     */
    typedef struct
    {
        std::string strName;    /* 名称 */
        std::string strAddress; /* 邮件地址 */
    } EmailUser_S;

    /**
     * @brief 邮件事件信息
     */
    typedef struct
    {
        std::string strSubject;   /* 邮件主题 */
        std::string strMessage;   /* 邮件内容 */
        std::vector<std::string> vecImageFile; /* 图片附件集 */
    } EmailEventInfo_S;

    /**
     * @brief 邮件信息
     */
    typedef struct
    {
        bool                     bTlsEnable              = false; /* 是否开启TLS */
        bool                     bEnServerAuthentication = false; /* 是否开启服务器认证 */
        std::string              strUserName;                     /* 用户名 */
        std::string              strPassword;                     /* 密码 */
        bool                     bImageAttachment        = false; /* 使用附件 */
        int                      nCaptureTimeInterval    = 2;     /* 图片抓图时间间隔 */
        SmtpServer_S             stServer;                        /* stmp服务器 */
        EmailUser_S              stSender;                        /* 发送者 */
        std::vector<EmailUser_S> stRecipient;                     /* 接收者列表 */
    } EmailInfo_S;

    /*
    *@brief 编码Id
    */
    typedef struct
    {
        std::string alarmInputId;          /* 报警输入编码id */
        std::string chnId;                 /* 视频通道编码id */
        std::string whiteList;             /* 允许名单，ip */
    }EncodingId_S;

    /**
     * @brief gb28181客户端配置
     */
    typedef struct
    {
        bool        bEnableGB28181       = false; /* 是否开启28181 */
        bool        bStatus              = false; /* 设备注册状态 */ 
        bool        bEnableGB35114       = false; /* 是否开启35114 */
        bool        bIsTcp               = false; /* 是否使用TCP协议，默认UDP协议 */
        int         nLocalSipPort        = 0;     /* 本地sip端口 */
        std::string serverID;                     /* sip服务器id */
        std::string realm;                        /* sip服务器域 */
        std::string address;                      /* sip服务器地址 */
        int         nServerPort;                  /* sip服务器端口 */
        std::string userId;                       /* sip用户id */
        std::string password;                     /* 密码 */
        int         nValidityPeriod      = 86400; /* 注册有效期（单位：秒）规定缺省为1天 */
        int         nHeartbeatInterval   = 60;    /* 心跳周期（单位：秒）规定不低于60秒 */
        int         nMaxHeartbeatTimeout = 3;     /* 最大心跳超时次数 */
        int         nSpeedType           = 0;     /* 速度类型：0倍速 1倍率 */
        int         nSpeed               = 0;     /* 倍速 */
        int         nMagnification       = 0;     /* 倍率 */
        bool        bStreamPrivateInfo   = false; /* 是否开启码流私有信息 */
        std::vector<EncodingId_S> stEncode;       /* 编码Id */
    } GB28181Client_S;

    /**
     * @brief Isup信息
     */
    typedef struct
    {
        bool        bEnableIsup = false; /* 是否开启isup */
        std::string address;             /* 服务器地址 */
        int         nPort       = 0;     /* 端口 */
        int         nRegisterStatus;     /* 注册状态 */
        std::string protoVersion;        /* 协议版本 */
        std::string encrypKey;           /* 加密密钥 */
    } Isup_S;

    /**
     * @brief 集成协议
     */
    typedef struct
    {
        bool bEnableOnvif = false; /* 是否开启onvif */
        int  nOnvifMode   = 0;     /* onvif认证模式 */
        bool bEnableIsapi = false; /* 是否开启isapi */
    } IntegrationProto_S;

    /**
     * @brief 证书申请信息
     */
    typedef struct
    {
        int         nValday;  /* 有效期（天数） */
        /* 必选参数 */
        std::string strCN;    /* 域名或ip */
        std::string strC;     /* 国家代码（例如：CN 表示中国） */
        /* 可选参数 */
        std::string strO;     /* 组织名称（公司名称） */
        std::string strOU;    /* 组织单元名称（部门名称） */
        std::string strST;    /* 省份名称（例如：Guangdong 表示广东） */
        std::string strL;     /* 城市名称（例如：Guangzhou 表示广州） */
        std::string strEmail; /* 电子邮件地址 */
    } CertApplyInfo_S;

    /**
     * @brief 证书文件信息
     */
    typedef struct
    {
        int         nNum;          /* 证书序号 */
        std::string strExpiraDate; /* 到期时间 */
        std::string strSerialNum;  /* 证书序列号 */
        std::string strUser;       /* 使用者 */
        std::string strLicensor;   /* 颁发者 */
        std::string strPath;       /* 证书文件路径 */
    } CertFileInfo_S;

    /**
     * @brief https配置信息
     */
    typedef struct
    {
        bool        bEnHttps = false; /* 是否启用https */
        std::string strCertPath;      /* 证书路径 */
        std::string strKeyPath;       /* 密钥文件路径 */
        bool        bEnRtsp = true;   /* 是否启用rtsp */

    } HttpsConfigInfo_S;

    /**
     * @brief onvif配置信息
     */
    typedef struct
    {
        bool bEnOnvif      = true; /* 是否启用Onvif */
        int nOnvifAuthMode = 0;    /* onvif认证模式 */
    }OnvifConfigInfo_S;

    /**
     * @brief upnp配置信息
     */
    typedef struct
    {
        std::string strUdn;         /* 设备唯一标识符 */
        std::string strServiceType; /* 服务类型标识 */
        std::string strControlURL;  /* 控制接口地址 */
    } UpnpConfigInfo_S;

    /**
     * @brief qos配置信息
     */
    typedef struct
    {
        int nMediaDscp = 0;  /* 媒体DSCP（0~63） */
        int nAlarmDscp = 0;  /* 报警DSCP（0~63） */
        int nManageDscp = 0; /* 管理DSCP（0~63） */
    } QosConfigInfo_S;

    /**
     * @brief bonjour配置信息
     */
    typedef struct
    {
        bool        bEnBonjour = false; /* 是否启用bonjour */
        std::string strHostName;        /* 主机名称 */
        std::string strServerName;      /* 服务器名称 */
        int         nPort;              /* 监听端口 */
    } BonjourConfigInfo_S;

    //info /*--------------------- 国际证书管理（国密）---------------------*/

    /* 国密证书持有者网络类型 */
    typedef enum class _GmCertNetworkType_
    {
        PUBLIC_SECURITY_INFORMATION_NETWORK = 1, /* 公安信息网 */
        VIDEO_DEDICATED_NETWORK             = 2, /* 视频专网 */
    } GmCertNetworkType_E;

    /* 国密证书文件信息 */
    typedef struct _GmCertFileInfo_S_
    {
        int         nNum = 0;         /* 证书序号 */
        std::string strSerialNum;     /* 证书序列号 */
        std::string strUser;          /* 使用者 */
        std::string strFunction;      /* 证书功能 */
        std::string strEffectiveDate; /* 生效日期 */
        std::string strExpiraDate;    /* 失效日期 */
        std::string strPath;          /* 证书文件路径 */

        /* 重载小于号 */
        bool operator<(const _GmCertFileInfo_S_ &other) const
        {
            return nNum < other.nNum;
        }
    } GmCertFileInfo_S;

    /* 吊销证书信息 */
    typedef struct _RevokedCertInfo_S_
    {
        std::string strSerialNum;      /* 被吊销证书序列号 */
        std::string strRevocationDate; /* 吊销日期 */
        std::string strReasonCode;     /* 吊销原因 */
        std::string strInvalidityDate; /* 失效日期 */
    } RevokedCertInfo_S;

    /* 国密CRL文件信息 */
    typedef struct _GmCrlFileInfo_S_
    {
        std::string                    strIssuer;       /* 颁发者 */
        std::string                    strThisUpdate;   /* CRL更新时间 */
        std::string                    strNextUpdate;   /* CRL下次更新时间 */
        int                            nCrlNumber = 0;  /* CRL编号 */
        std::string                    strPath;         /* CRL文件路径 */
        std::vector<RevokedCertInfo_S> vecRevokedCerts; /* 被吊销的证书列表 */
    } GmCrlFileInfo_S;

    // info /*--------------------- 国际证书管理（国密）end ---------------------*/

#ifdef ENABLE_GAT1400_SRC
    /* GAT1400接入配置信息 */
    typedef struct _Gat1400Client_
    {
        bool enableGat1400 = false;         /* 是否启用GAT1400协议 */
        bool status = false;                /* 接入状态 */
        int port = -1;                      /* 接入 GAT1400 平台端口 */
        std::string ip;                     /* 接入 GAT1400 平台地址 */
        std::string deviceID;               /* 设备ID */
        std::string name;                   /* 接入平台用户名，与设备ID一致 */
        std::string passwd;                 /* 接入平台密码 */
        int pointType = 1;                  /* 坐标形式 0-万分比坐标、1-像素坐标、2-归一化坐标 */
        bool keepAlive = false;             /* 长连接/短连接 */
        bool enableFace = false;            /* 是否上传人脸图片 */
        bool enablePerson = false;          /* 是否上传人员图片 */
        bool enableMotorvehicle = false;    /* 是否上传机动车图片 */
        bool enableNonmotorvehicle = false; /* 是否上传非机动车图片 */
        std::string version;                /* 协议版本 VIID_2017、VIID_2018 */
    } Gat1400Client_S;
#endif

    typedef struct _WifiStaInfo_S_
    {
        bool bEnableWifi = false;  /* 是否开启wifi */
        bool bEnableBoost = false; /* 是否开启增强功能 */
    } WifiStaInfo_S;

    // 安全模式枚举
    enum class WifiSecurityMode
    {
        WPA_PERSONAL, // WPA-个人版
        OPEN,         // 开放网络
        WEP,          // WEP加密
        EAP_PEAP,     // WPA企业版 (PEAP)
        EAP_TLS,      // WPA企业版 (TLS)
        EAP_TTLS,
        WPA3_PERSONAL //WPA3加密
    };

    // WEP 密钥配置结构
    struct WepKeyConfig
    {
        int index;         // 密钥索引 1-4
        std::string value; // 密钥内容
    };

    typedef struct _WifiStaConncet_S_
    {
        std::string ssid;      // SSID
        WifiSecurityMode mode; // 安全模式
        std::string ip_address;

        // --- 通用配置 ---
        std::string password; // 密码 (用于 WPA-Personal, WEP, EAP-PEAP)

        // --- WPA-Personal 特有 ---
        std::string pairwise; // 加密类型 (TKIP/CCMP), 默认 CCMP

        // --- WEP 特有 ---
        int wep_key_len;                    // 64 or 128
        bool wep_is_hex;                    // true=16进制, false=ASCII
        std::string auth_alg;               // "OPEN" or "SHARED"
        std::vector<WepKeyConfig> wep_keys; // 密钥列表

        // --- EAP-PEAP 特有 ---
        std::string eap_identity;       // 用户名
        std::string eap_password;       // 密码
        std::string peap_version;       // "0" or "1"
        std::string phase2;             // 内部认证, 如 "auth=GTC"
        std::string anonymous_identity; // 匿名身份
        std::string ca_cert_path;       // CA证书路径
        std::string peap_label;         // PEAP标签（old)

        // --- EAP-TLS 特有 ---
        std::string tls_identity;       // 身份
        std::string private_key_passwd; // 私钥密码
        std::string eapol_version;      // EAPOL版本
        std::string client_cert_path;   // 用户证书路径
        std::string private_key_path;   // 私钥路径

        // --- EAP-TTLS 特有配置  ---
        std::string eap_anonymous_identity; // 匿名身份 (必填)
        std::string eap_ttls_phase2;        // 内部认证: "PAP" 或 "MSCHAPV2"

        std::string ctrl_interface;

        std::string interface_name;
        // 构造函数初始化默认值
        _WifiStaConncet_S_()
            : mode(WifiSecurityMode::OPEN), pairwise("CCMP"), wep_key_len(128), wep_is_hex(false), auth_alg("OPEN"),
              peap_version("0"), eapol_version("2"), eap_anonymous_identity(""), eap_ttls_phase2("PAP"), // 默认设为 PAP
              ctrl_interface("/var/run/wpa_supplicant"), interface_name("wlan0")
        {
        }
    } WifiStaConncet_S;
    enum WifiConnectErrorCode
    {
        WIFI_CONNECT_SUCCESS = 0,                    // 连接成功
        WIFI_CONNECT_TIMEOUT = 1,                    // 等待连接超时
        WIFI_CONNECT_WRONG_PASSWORD = 2,             // WiFi 密码或密钥错误
        WIFI_CONNECT_INVALID_SSID = 3,               // SSID 为空或无效
        WIFI_CONNECT_INVALID_CREDENTIALS = 4,        // 密码、账号、证书等参数缺失
        WIFI_CONNECT_TEMP_CONFIG_FAILED = 5,         // 临时配置文件创建失败
        WIFI_CONNECT_APPLY_CONFIG_FAILED = 6,        // 配置文件应用失败
        WIFI_CONNECT_SUPPLICANT_START_FAILED = 7,    // wpa_supplicant 启动失败
        WIFI_CONNECT_RECONFIGURE_FAILED = 8,         // RECONFIGURE 命令失败
        WIFI_CONNECT_REASSOCIATE_FAILED = 9,         // REASSOCIATE 命令失败
        WIFI_CONNECT_NETWORK_NOT_FOUND = 10,         // 未扫描到目标网络
        WIFI_CONNECT_AUTHENTICATION_FAILED = 11,     // 企业认证或其他鉴权失败
        WIFI_CONNECT_UNKNOWN_ERROR = 12,              // 无法进一步分类的错误
        WIFI_CONNECT_BUSY = 13,                      // 已有连接操作正在执行
        WIFI_CONNECT_ENABLE_NETWORK_FAILED = 14,     // 解除网络禁用状态失败
        WIFI_CONNECT_RECONNECT_FAILED = 15           // RECONNECT 命令失败
    };
    struct WifiConnectResult
    {
        bool success;           // 连接是否成功
        std::string ip_address; // 获取到的 IP 地址
        int error_code;         // 错误码 WifiConnectErrorCode

        // 构造函数，方便初始化
        WifiConnectResult() : success(false), ip_address(""), error_code(WIFI_CONNECT_SUCCESS)
        {
        }
    };

    // 4G
    typedef enum
    {
        AUTH_NONE = 0,    // 无鉴权
        AUTH_PAP = 1,     // PAP
        AUTH_CHAP = 2,    // CHAP
        AUTH_PAP_CHAP = 3 // 3=PAP&CHAP
    } Auth_Mode_t;

    // 网络制式枚举 (对应网页上的 "网络切换方式" 下拉框)
    typedef enum
    {
        NET_AUTO = 0,    // 自动
        NET_4G_ONLY = 1, // 仅4G
        NET_3G_ONLY = 2, // 仅3G
        NET_2G_ONLY = 3  // 仅2G
    } Network_Mode_t;

    // 拨号方式枚举 (对应网页上的 "拨号方式" 下拉框)
    typedef enum
    {
        DIAL_AUTO = 0,  // 自动拨号 (模块内部处理)
        DIAL_MANUAL = 1 // 手动拨号 (主机发送ATD指令)
    } Dial_Mode_t;

    typedef struct Network_4G_Config_t
    {
        bool enabled;                // 启用状态
        std::string apn;          // APN (例如: ctnet, cmnet)
        std::string username;     // 用户名 (专网参数)
        std::string password;     // 密码 (专网参数)
        std::string call_number;  // 拨号号码 (通常为 *99# 或 *99***1#)
        std::string phone_number; // 拨号号码 (通常为 *99# 或 *99***1#)
        int mtu;                  // MTU大小 (例如: 1500, 1400)

        // 以下使用int存储枚举值，比string更高效
        int auth_mode;    // 鉴权方式 (0:None, 1:PAP, 2:CHAP,3=PAP&CHAP)
        int network_mode; // 网络模式 (0:Auto, 1:4G, 2:3G...)
        int dial_mode;    // 拨号方式 (0:Auto, 1:Manual)

        // 构造函数：提供默认初始化值，防止空数据导致崩溃
        Network_4G_Config_t()
            : apn(""), username(""), password(""), call_number("*99#"), mtu(1500), auth_mode(AUTH_NONE), network_mode(NET_AUTO),
              dial_mode(DIAL_AUTO)
        {
        }
    } Network_4G_Config_t;

    typedef struct _HotspotConfig_
    {
        bool enabled;                // 启用状态
        std::string ssid;            // 网络名称
        std::string securityMode;    // 安全模式
        std::string encryptionType;  // 加密类型
        std::string password;        // 密码
        std::string confirmPassword; // 确认密码
        _HotspotConfig_()
            : enabled(false), ssid(""), securityMode("WPA2-personal"), password(""), encryptionType(""),
              confirmPassword("")
        {
        }
        void clear() {
            *this = _HotspotConfig_(); // 复用默认构造函数逻辑
        }
    } HotspotConfig;

    typedef struct SIM_Info_t
    {
        std::string status;         // 状态
        std::string ip_address;     // ip地址
        std::string subnet_mask;    // 子网掩码
        std::string gateway;        // 网关地址
        std::string dns_address;    // dns地址
        std::string signal_quality; // 信号强度
        std::string operator_name;  // 运营商名称（如 "China Telecom"）
        std::string imei;           // 手机设备识别码
        std::string iccid;          // SIM卡识别码
        std::string network_type;
        bool is_registered;
        Network_4G_Config_t set_config_ret;
    } SIM_Info_t;

    typedef struct _Platform_Info_t_
    {
        std::string server_ip; /* 服务器地址 */
        int server_port;       /* 端口 */
        int rtmp_port;         // RTMP推流端口
        int mqtt_port;         /* MQTT Broker 端口 */
        std::string user;      /* 账号 */
        std::string password;  /* 密码 */
        bool enable;           /* 是否开启 */
        bool Custom;           /* 是否自定义 */

        // _Platform_Info_t_()
        //     : server_ip("172.16.25.125"), server_port(388), mqtt_port(1884), user("admin"), password("Aa@135791"), enable(false), Custom(false),
        //       rtmp_port(1935)
        // {
        // }
        _Platform_Info_t_()
            : server_ip("183.129.224.253"), server_port(4910), mqtt_port(1884), user("admin"), password("Aa@135791"), enable(false), Custom(false),
              rtmp_port(4920)
        {
        }
    } Platform_Info_t;

    typedef struct _Platform_Store_Info_t_
    {
        std::string access_token;//服务器地址
        std::string user;//账号
        std::string password;//密码

        _Platform_Store_Info_t_() : access_token(""), user("admin"), password("Aa@135791")
        {
        }
    }Platform_Store_Info_t;

    typedef struct LoginInfo
    {
        std::string host = "";
        int port;
        std::string login_user = "";
        std::string login_password = "";
        bool enable;
        bool Custom;
    } LoginInfo;
}
