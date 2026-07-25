/**
 * @FilePath     : crypto_init.cpp
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-05-26 16:25:37
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-06-22 09:37:42
 * @Description  : 密码学模块业务层初始化
 */

#include "crypto_init.h"
#include "dlog.h"
#include "crypto_manager.h"
#include "gb35114_keystore.h"
#include "hi_crypto_provider.h"
#include "openssl_provider.h"

/* info: 当前阶段需要启动时执行国密自测，测试完成后可改为 0 或迁移为 CMake/配置开关 */
#define ENABLE_GB35114_CRYPTO_SELF_TEST 0
/* info: Hi3516 业务仓库默认启用海思硬件 Provider；未完成的非算法/格式桥接能力由 Provider 显式 fallback */
#define ENABLE_HI_CRYPTO_PROVIDER 1

CCryptoInit::CCryptoInit()
{
}

CCryptoInit::~CCryptoInit()
{
    if (m_bInitialized)
    {
        deinit();
    }
}

IpcRet_E CCryptoInit::init()
{
    if (m_bInitialized)
    {
        dlog_warn("CCryptoInit 已经初始化");
        return OK;
    }

    /* info: 业务层决定注入哪个 Provider，共享层不得依赖 hi3516_ipc 的具体实现 */
#if ENABLE_HI_CRYPTO_PROVIDER
    IpcRet_E ret = CCryptoManager::instance()->set_provider(CHiCryptoProvider::instance());
#else
    IpcRet_E ret = CCryptoManager::instance()->set_provider(COpenSSLProvider::instance());
#endif
    if (ret != OK)
    {
        dlog_error("CCryptoManager Provider 注入失败");
        return ret;
    }
    dlog_info("%s 已注入", CCryptoManager::instance()->get_provider()->name());

    /* 初始化 CCryptoManager */
    ret = CCryptoManager::instance()->init();
    if (ret != OK)
    {
        dlog_error("CCryptoManager 初始化失败");
        return ret;
    }

    /* info: 当前阶段默认注入软件 KeyStore；后续海思/瑞芯微硬件后端只在业务层替换注入对象 */
    ret = CGb35114KeyStoreManager::instance()->set_store(CSoftwareGb35114KeyStore::instance());
    if (ret != OK)
    {
        dlog_error("GB35114 KeyStore 注入失败");
        CCryptoManager::instance()->deinit();
        return ret;
    }

    ret = CGb35114KeyStoreManager::instance()->init();
    if (ret != OK)
    {
        dlog_error("GB35114 KeyStore 初始化失败");
        CCryptoManager::instance()->deinit();
        return ret;
    }

#if ENABLE_GB35114_CRYPTO_SELF_TEST
    /* info: 自测会在 /tmp 下创建临时密钥和 CSR 文件，用于验证当前注入 Provider 的基础能力 */
    ret = CCryptoManager::instance()->self_test();
    if (ret != OK)
    {
        dlog_error("CCryptoManager 自测失败");
        return ret;
    }
#endif

    m_bInitialized = true;
    dlog_trace("CCryptoInit::init() 完成");
    return OK;
}

IpcRet_E CCryptoInit::deinit()
{
    if (!m_bInitialized)
    {
        return OK;
    }

    dlog_trace("CCryptoInit::deinit() 开始");

    /* info: KeyStore 可能依赖 CryptoManager 生成迁移密钥，反初始化时先释放上层安全状态 */
    CGb35114KeyStoreManager::instance()->deinit();

    /* 调用 CCryptoManager 反初始化 */
    CCryptoManager::instance()->deinit();

    m_bInitialized = false;
    dlog_trace("CCryptoInit::deinit() 完成");
    return OK;
}
