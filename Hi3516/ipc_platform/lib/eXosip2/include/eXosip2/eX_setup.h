/**
 * @file eX_setup.h
 * @brief eXosip - 扩展的osip库
 *
 * @copyright 版权所有 (C) 2001-2020 Aymeric MOIZARD amoizard@antisip.com
 *
 * eXosip是自由软件；您可以根据GNU通用公共许可证自由分发或修改，
 * 许可证版本为第2版或（按您的选择）任何更高版本。
 *
 * eXosip的分发希望它有用，但不提供任何担保；包括适销性或特定用途适用性的暗示担保。
 * 详情请参阅GNU通用公共许可证。
 *
 * 您应已收到GNU通用公共许可证副本；如果没有，请致信自由软件基金会，
 * 地址：59 Temple Place, Suite 330, Boston, MA 02111-1307 USA。
 *
 * 此外，作为特殊例外，版权持有人允许在特定条件下将此程序的部分代码与OpenSSL库链接，
 * 条件在各源文件中描述，并分发包含两者的组合。
 * 对于除OpenSSL外的代码，您必须完全遵守GNU通用公共许可证。
 */

 #ifdef ENABLE_MPATROL
 #include <mpatrol.h>  /* 内存调试工具头文件 */
 #endif
 
 #ifndef __EX_SETUP_H__
 #define __EX_SETUP_H__
 
 #include <eXosip2/eXosip.h>        /* eXosip主头文件 */
 #include <osipparser2/osip_message.h>  /* osip消息解析头文件 */
 #include <time.h>                  /* 时间相关函数 */
 
 #ifdef __cplusplus
 extern "C" {
 #endif
 
 struct eXosip_t;                  /* eXosip上下文前向声明 */
 struct osip_srv_record;           /* SRV记录结构前向声明 */ 
 struct osip_naptr;                /* NAPTR记录结构前向声明 */
 
 /**
  * @file eX_setup.h
  * @brief eXosip设置API
  *
  * 该文件提供配置SIP终端所需的API
  */
 
 /**
  * @defgroup eXosip2_conf eXosip2配置API
  * @ingroup eXosip2_setup
  * @{
  */
 
 /**
  * 分配eXosip上下文
  *
  * @return 新分配的eXosip_t实例
  */
 struct eXosip_t *eXosip_malloc(void);
 
 /**
  * 初始化扩展的oSIP库
  *
  * @param excontext eXosip实例
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_init(struct eXosip_t *excontext);
 
 /**
  * 释放扩展的oSIP库使用的资源
  *
  * @param excontext eXosip实例
  */
 void eXosip_quit(struct eXosip_t *excontext);
 
 /**
  * 锁定扩展的oSIP库
  *
  * @param excontext eXosip实例
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_lock(struct eXosip_t *excontext);
 
 /**
  * 解锁扩展的oSIP库
  *
  * @param excontext eXosip实例
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_unlock(struct eXosip_t *excontext);
 
 /**
  * 处理eXosip事件（仅限非线程模式）
  *
  * @param excontext eXosip实例
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_execute(struct eXosip_t *excontext);
 
 /* 配置选项定义 */
 #define EXOSIP_OPT_BASE_OPTION 0
 #define EXOSIP_OPT_UDP_KEEP_ALIVE (EXOSIP_OPT_BASE_OPTION + 1)  /**< int *: 保活包间隔（UDP、TCP、TLS、DTLS） */
 #define EXOSIP_OPT_AUTO_MASQUERADE_CONTACT (EXOSIP_OPT_BASE_OPTION + 2) /**< int *: 特定重用"rport" */
 #define EXOSIP_OPT_UDP_LEARN_PORT EXOSIP_OPT_AUTO_MASQUERADE_CONTACT /**< 已废弃，由EXOSIP_OPT_AUTO_MASQUERADE_CONTACT替代 */
 #define EXOSIP_OPT_USE_RPORT (EXOSIP_OPT_BASE_OPTION + 7)        /**< int *: 启用或禁用Via头中的rport */
 #define EXOSIP_OPT_SET_IPV4_FOR_GATEWAY (EXOSIP_OPT_BASE_OPTION + 8) /**< char *: 通常为代理地址 */
 #define EXOSIP_OPT_ADD_DNS_CACHE (EXOSIP_OPT_BASE_OPTION + 9)    /**< struct eXosip_dns_cache *: 强制缓存条目避免DNS查询 */
 #define EXOSIP_OPT_DELETE_DNS_CACHE (EXOSIP_OPT_BASE_OPTION + 10) /**< struct eXosip_dns_cache *: 强制删除缓存条目 */
 #define EXOSIP_OPT_SET_IPV6_FOR_GATEWAY (EXOSIP_OPT_BASE_OPTION + 12) /**< char *: 通常为代理地址 */
 #define EXOSIP_OPT_ADD_ACCOUNT_INFO (EXOSIP_OPT_BASE_OPTION + 13) /**< struct eXosip_account_info *: 内部使用 */
 #define EXOSIP_OPT_DNS_CAPABILITIES (EXOSIP_OPT_BASE_OPTION + 14) /**< int *: 0禁用，2（默认）使用NAPTR/SRV记录 */
 #define EXOSIP_OPT_SET_DSCP (EXOSIP_OPT_BASE_OPTION + 15)        /**< int *: 设置SIP套接字的DSCP值 */
 #define EXOSIP_OPT_REGISTER_WITH_DATE (EXOSIP_OPT_BASE_OPTION + 16) /**< int *: 在REGISTER中启用Date头 */
 #define EXOSIP_OPT_SET_HEADER_USER_AGENT (EXOSIP_OPT_BASE_OPTION + 17) /**< char *: 设置User-Agent头 */
 #define EXOSIP_OPT_ENABLE_DNS_CACHE (EXOSIP_OPT_BASE_OPTION + 18) /**< int *: 0禁用缓存 */
 #define EXOSIP_OPT_ENABLE_AUTOANSWERBYE (EXOSIP_OPT_BASE_OPTION + 19) /**< int *: 0禁用自动应答BYE */
 #define EXOSIP_OPT_ENABLE_IPV6 (EXOSIP_OPT_BASE_OPTION + 20)     /**< int *: 0禁用，1仅IPv6，2自动选择最佳 */
 #define EXOSIP_OPT_ENABLE_REUSE_TCP_PORT (EXOSIP_OPT_BASE_OPTION + 21) /**< int *: 0禁用，1启用重用本地TCP端口 */
 #define EXOSIP_OPT_ENABLE_USE_EPHEMERAL_PORT (EXOSIP_OPT_BASE_OPTION + 22) /**< int *: 0禁用，1在Contact头中使用临时TCP端口 */
 #define EXOSIP_OPT_SET_CALLBACK_WAKELOCK (EXOSIP_OPT_BASE_OPTION + 23) /**< CbSipWakeLock *: 设置事务开始/结束时的回调 */
 #define EXOSIP_OPT_ENABLE_OUTBOUND (EXOSIP_OPT_BASE_OPTION + 24) /**< int *: 0禁用，1启用对话框Contact头中的ob参数 */
 #define EXOSIP_OPT_SET_OC_LOCAL_ADDRESS (EXOSIP_OPT_BASE_OPTION + 25) /**< char *: 设置出站连接绑定的IP地址 */
 #define EXOSIP_OPT_SET_OC_PORT_RANGE (EXOSIP_OPT_BASE_OPTION + 26) /**< int[2] *: 设置出站连接的端口范围 */
 #define EXOSIP_OPT_REMOVE_PREROUTESET (EXOSIP_OPT_BASE_OPTION + 27) /**< int *: 0保留初始INVITE中的预路由集，1（默认）移除 */
 #define EXOSIP_OPT_SET_SIP_INSTANCE (EXOSIP_OPT_BASE_OPTION + 28) /**< char *: 定义Contact头中的+sip.instance参数 */
 #define EXOSIP_OPT_SET_MAX_MESSAGE_TO_READ (EXOSIP_OPT_BASE_OPTION + 29) /**< int: 设置每次网络处理读取的最大消息数 */
 #define EXOSIP_OPT_SET_MAX_READ_TIMEOUT (EXOSIP_OPT_BASE_OPTION + 30) /**< long int: 设置读取SIP消息的超时时间（纳秒）*/
 #define EXOSIP_OPT_SET_DEFAULT_CONTACT_DISPLAYNAME (EXOSIP_OPT_BASE_OPTION + 31) /**< char *: 定义Contact头中的显示名称 */
 #define EXOSIP_OPT_SET_SESSIONTIMERS_FORCE (EXOSIP_OPT_BASE_OPTION + 32) /**< int *: 0（默认）双方支持时激活会话定时器，1远程不支持时本地激活 */
 #define EXOSIP_OPT_FORCE_CONNECTIONREUSE (EXOSIP_OPT_BASE_OPTION + 33) /**< int *: 0禁用，1强制重用已建立的连接 */
 #define EXOSIP_OPT_SET_CONTACT_DIALOG_EXTRA_PARAMS (EXOSIP_OPT_BASE_OPTION + 34) /**< char *: 定义Contact头中的额外参数 */
 
 /* TLS相关选项 */
 #define EXOSIP_OPT_SET_TLS_VERIFY_CERTIFICATE (EXOSIP_OPT_BASE_OPTION + 500) /**< int *: 启用TLS连接的证书验证 */
 #define EXOSIP_OPT_SET_TLS_CERTIFICATES_INFO (EXOSIP_OPT_BASE_OPTION + 501) /**< eXosip_tls_ctx_t *: 客户端/服务器证书信息 */
 #define EXOSIP_OPT_SET_TLS_CLIENT_CERTIFICATE_NAME (EXOSIP_OPT_BASE_OPTION + 502) /**< char*: 选择Windows证书存储中的特定证书 */
 #define EXOSIP_OPT_SET_TLS_SERVER_CERTIFICATE_NAME (EXOSIP_OPT_BASE_OPTION + 503) /**< char*: 选择Windows证书存储中的特定证书 */
 
 /* 非标准选项 */
 #define EXOSIP_OPT_KEEP_ALIVE_OPTIONS_METHOD (EXOSIP_OPT_BASE_OPTION + 1000)
 #define EXOSIP_OPT_SET_TSC_SERVER (EXOSIP_OPT_BASE_OPTION + 1001) /**< 已废弃 */
 
 #define EXOSIP_OPT_GET_STATISTICS (EXOSIP_OPT_BASE_OPTION + 2000) /**< struct eXosip_stats*: 获取事务、注册、呼叫等统计信息 */
 
 /**
  * 用于插入DNS缓存条目避免DNS解析的结构
  * @struct eXosip_dns_cache
  */
 struct eXosip_dns_cache {
   char host[1024];  /* 主机名 */
   char ip[256];     /* IP地址 */
 };
 
 /**
  * 账户信息结构
  */
 struct eXosip_account_info {
   char proxy[1024];  /* 代理服务器地址 */
   char nat_ip[256];   /* NAT IP地址 */
   int nat_port;       /* NAT端口 */
 };
 
 /**
  * HTTP认证结构
  */
 struct eXosip_http_auth {
   char pszCallId[64];                   /* 呼叫ID */
   osip_proxy_authenticate_t *wa;        /* 代理认证信息 */
   char pszCNonce[64];                   /* 客户端nonce */
   int iNonceCount;                      /* nonce计数 */
   int answer_code;                      /* 应答代码 */
 };
 
 #ifndef MINISIZE
 /**
  * 用于获取eXosip内部统计信息的结构
  * 总数自eXosip启动或重启后累计
  * 平均值基于EXOSIP_STATS_PERIOD计算（默认为3600秒）
  *
  * @struct eXosip_stats
  */
 struct eXosip_stats {
   int allocated_transactions;    /**< 当前分配的事务数 */
   float average_transactions;    /**< 新事务平均数/小时 */
   int allocated_registrations;   /**< 当前分配的注册数 */
   float average_registrations;   /**< 新注册平均数/小时 */
   int allocated_calls;           /**< 当前分配的呼叫数 */
   float average_calls;           /**< 新呼叫平均数/小时 */
   int allocated_publications;    /**< 当前分配的发布数 */
   float average_publications;    /**< 新发布平均数/小时 */
   int allocated_subscriptions;   /**< 当前分配的订阅数 */
   float average_subscriptions;   /**< 新订阅平均数/小时 */
   int allocated_insubscriptions; /**< 当前分配的入站订阅数 */
   float average_insubscriptions; /**< 新入站订阅平均数/小时 */
 
   int reserved1[20]; /**< 保留字段 */
 };
 #endif
 
 /**
  * 设置eXosip选项
  *
  * @param excontext eXosip实例
  * @param opt 要配置的选项
  * @param value 选项值
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_set_option(struct eXosip_t *excontext, int opt, const void *value);
 
 /**
  * 描述客户端或服务器凭证的结构
  * 包含证书、对应私钥及其密码
  *
  * 如果服务器需要客户端证书，必须提供证书、私钥和私钥密码
  *
  * 可以"固定"公钥证书，public_key_pinned必须包含DER格式的公钥文件
  * 要从PEM证书提取DER格式公钥，可使用命令：
  * openssl x509 -in server-cert.pem -pubkey -noout | openssl enc -base64 -d > pub_key.der
  *
  * @struct eXosip_tls_credentials_s
  */
 typedef struct eXosip_tls_credentials_s {
   char priv_key[1024];          /**< 私钥文件绝对路径 */
   char priv_key_pw[1024];       /**< 打开私钥的密码 */
   char cert[1024];              /**< 私钥对应证书的绝对路径 */
   char public_key_pinned[1024]; /**< 服务器预期公钥的绝对路径 */
 } eXosip_tls_credentials_t;
 
 /**
  * 描述eXosip整个TLS上下文的结构
  *
  * 客户端连接服务器时，如要验证证书，只需配置root_ca_cert参数为包含所有受信任CA的文件
  * Windows和Macosx上会自动加载存储中的受信任证书
  *
  * @struct eXosip_tls_ctx_s
  */
 typedef struct eXosip_tls_ctx_s {
   char random_file[1024];          /**< 随机数据文件的绝对路径 */
   char dh_param[1024];             /**< Diffie-Hellman密钥交换所需文件 */
   char root_ca_cert[1024];         /**< 已知根CA的文件路径 */
   char cipher_list[2048];          /**< openssl加密列表 */
   unsigned long tls_flags;         /**< openssl附加标志 */
   unsigned long dtls_flags;        /**< openssl附加标志 */
   eXosip_tls_credentials_t client; /**< 客户端凭证 */
   eXosip_tls_credentials_t server; /**< 服务器凭证 */
 } eXosip_tls_ctx_t;
 
 /**
  * 设置eXosip_tls_ctx时可能发生的错误枚举
  */
 typedef enum {
   TLS_OK = 0,                    /**< 一切正常 */
   TLS_ERR_NO_RAND = -1,          /**< 未指定随机文件路径 */
   TLS_ERR_NO_DH_PARAM = -2,      /**< 未指定Diffie-Hellman文件路径 */
   TLS_ERR_NO_PW = -3,            /**< 未指定密码 */
   TLS_ERR_NO_ROOT_CA = -4,       /**< 未指定根CA文件路径 */
   TLS_ERR_MISSING_AUTH_PART = -5 /**< 缺少私钥或证书 */
 } eXosip_tls_ctx_error;
 
 /**
  * 启动并返回osip_naptr上下文
  * 注意DNS结果可能尚未可用
  *
  * 如提供FQDN，将对其执行NAPTR查询
  * 如要执行ENUM查询，需同时指定查询域和AUS（拨打的号码），用"!"分隔
  *
  * @param excontext eXosip实例
  * @param domain 要查询的域名或ENUM查询
  * @param protocol 使用的协议("SIP")
  * @param transport 使用的传输("UDP")
  * @param keep_in_cache 结果是否缓存
  * @return NAPTR记录结构
  */
 struct osip_naptr *eXosip_dnsutils_naptr(struct eXosip_t *excontext, const char *domain, const char *protocol, const char *transport, int keep_in_cache);
 
 /**
  * 释放eXosip_dnsutils_naptr分配的内存
  *
  * @param naptr_record 要释放的NAPTR结构
  */
 void eXosip_dnsutils_release(struct osip_naptr *naptr_record);
 
 /**
  * 继续处理异步DNS请求（如实现）
  *
  * @param output_record 结果结构
  * @param force 是否强制等待最终应答
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_dnsutils_dns_process(struct osip_naptr *output_record, int force);
 
 /**
  * 将第一个SRV条目旋转到最后
  *
  * @param output_record 结果结构
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_dnsutils_rotate_srv(struct osip_srv_record *output_record);
 
 /**
  * 在指定套接字上监听
  *
  * @param excontext eXosip实例
  * @param transport 传输协议(IPPROTO_UDP等)
  * @param addr 绑定地址(NULL表示所有接口)
  * @param port 监听端口(0表示随机端口)
  * @param family IP族(AF_INET或AF_INET6)
  * @param secure 0表示UDP/TCP，1表示TLS
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_listen_addr(struct eXosip_t *excontext, int transport, const char *addr, int port, int family, int secure);
 
 /**
  * 重置传输套接字
  *
  * @param excontext eXosip实例
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_reset_transports(struct eXosip_t *excontext);
 
 /**
  * 在指定套接字上监听
  *
  * @param excontext eXosip实例
  * @param transport 传输协议(IPPROTO_UDP等)
  * @param socket 用于监听UDP SIP消息的套接字
  * @param port 伪装用的监听端口
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_set_socket(struct eXosip_t *excontext, int transport, int socket, int port);
 
 /**
  * 设置SIP User-Agent头字符串
  *
  * @param excontext eXosip实例
  * @param user_agent 要插入消息中的User-Agent头
  */
 void eXosip_set_user_agent(struct eXosip_t *excontext, const char *user_agent);
 
 /**
  * 获取eXosip版本字符串
  *
  * @return 版本字符串
  */
 const char *eXosip_get_version(void);
 
 /* 回调函数类型定义 */
 #ifdef WIN32
 typedef void(__stdcall *CbSipCallback)(osip_message_t *msg, int received);
 typedef void(__stdcall *CbSipWakeLock)(int state);
 #else
 typedef void (*CbSipCallback)(osip_message_t *msg, int received);
 typedef void (*CbSipWakeLock)(int state);
 #endif
 
 /**
  * 设置回调以获取发送和接收的SIP消息
  *
  * @param excontext eXosip实例
  * @param cbsipCallback 获取消息的回调
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_set_cbsip_message(struct eXosip_t *excontext, CbSipCallback cbsipCallback);
 
 /**
  * 用NAT的公网地址替换联系地址
  * IP地址应手动获取（固定IP）或通过STUN获取
  * 仅当远程通信方位于不同LAN时使用
  *
  * @param excontext eXosip实例
  * @param public_address 公网IP地址
  * @param port 伪装端口
  *
  * 如设为NULL，则自动猜测本地IP地址（返回默认模式）
  */
 void eXosip_masquerade_contact(struct eXosip_t *excontext, const char *public_address, int port);
 
 /**
  * 查找空闲的IPPROTO_UDP或IPPROTO_TCP端口
  *
  * @param excontext eXosip实例
  * @param free_port 搜索的起始端口
  * @param transport 传输协议(IPPROTO_UDP或IPPROTO_TCP)
  * @return 找到的端口号
  */
 int eXosip_find_free_port(struct eXosip_t *excontext, int free_port, int transport);
 
 #ifndef DOXYGEN
 /**
  * 唤醒eXosip_event_wait方法
  *
  * @param excontext eXosip实例
  */
 void eXosip_wakeup_event(struct eXosip_t *excontext);
 #endif
 
 /** @} */
 
 /**
  * @defgroup eXosip2_network eXosip2网络API
  * @ingroup eXosip2_setup
  * @{
  */
 
 /**
  * 修改发送SIP消息使用的传输协议
  *
  * @param msg 要修改的SIP消息
  * @param transport 使用的传输协议("UDP"、"TCP"或"TLS")
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_transport_set(osip_message_t *msg, const char *transport);
 
 /**
  * 查找当前本地IP（有默认路由的接口）
  *
  * @param excontext eXosip实例
  * @param family IP族(AF_INET或AF_INET6)
  * @param address 存储本地IP地址的字符串
  * @param size 字符串大小
  * @return 成功返回0，失败返回错误码
  */
 int eXosip_guess_localip(struct eXosip_t *excontext, int family, char *address, int size);
 
 /** @} */
 
 #ifdef __cplusplus
 }
 #endif
 #endif  /* __EX_SETUP_H__ */