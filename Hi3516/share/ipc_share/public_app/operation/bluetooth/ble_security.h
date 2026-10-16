#pragma once

#include "ble_command_protocol.h"

#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

namespace BleProvisioning
{

/*
 * BLE 配网安全会话管理器。
 *
 * 认证阶段：App 与设备交换随机数，并使用设备独立 setupSecret 校验 HMAC。
 * 数据阶段：双方派生临时 sessionKey，配网指令使用 AES-256-GCM 解密。
 * 生命周期：会话超时、停止 Notify 或服务退出时立即清除临时密钥。
 *
 * 注意：setupSecret、sessionKey、随机数和认证证明均不得写入日志。
 */
class BleSecurityManager
{
public:
    /** 初始化所有密钥、随机数和认证状态为空。 */
    BleSecurityManager();

    /** 从安全文件加载设备 setupSecret；文件不存在时生成并持久化新密钥。 */
    bool initialize();
    /** 处理0x0010：接收 App nonce，生成设备 nonce、sessionId 和临时会话密钥。 */
    CommandResult beginAuthentication(const CommandFrame &frame);
    /** 处理0x0011：验证 App 的 HMAC证明，认证成功后启用安全会话。 */
    CommandResult proveAuthentication(const CommandFrame &frame);
    /** 处理0x0012：检查超时后返回 authenticated 或 unauthenticated。 */
    CommandResult authenticationStatus() const;
    /** 安全擦除临时会话密钥并清空会话状态；长期 setupSecret 不受影响。 */
    void resetSession();
    /**
     * 验证 sessionId、counter 和 GCM Tag，并解密安全业务 Payload。
     * 只有完整认证成功才推进防重放 counter 和会话滑动过期时间。
     */
    StatusCode decryptProvisioningPayload(const CommandFrame &frame,
                                          std::vector<uint8_t> &plainText);
    /** 判断某指令是否必须在认证会话中以 AES-256-GCM 安全信封发送。 */
    static bool requiresSecureSession(uint16_t command);

private:
    /** 读取或创建设备长期密钥；调用者必须持有 m_mutex。 */
    bool loadOrCreateSecret();
    /** 检查5分钟滑动过期时间，过期时清除临时会话；调用者必须持锁。 */
    void expireSessionLocked() const;
    /** 使用 OPENSSL_cleanse 清除会话密钥和认证状态；调用者必须持锁。 */
    void clearSessionLocked() const;
    /** 使用双方 nonce 和 setupSecret 派生单次认证会话密钥；调用者必须持锁。 */
    bool deriveSessionKeyLocked();

private:
    mutable std::mutex m_mutex;
    /* 设备长期密钥，仅用于认证和派生会话密钥，不直接加密业务数据。 */
    std::array<uint8_t, 32> m_setupSecret;
    /* 单次认证会话的临时状态。mutable 用于 const 状态查询时处理超时。 */
    mutable std::array<uint8_t, 32> m_sessionKey;
    mutable std::array<uint8_t, 16> m_appNonce;
    mutable std::array<uint8_t, 16> m_deviceNonce;
    mutable uint32_t m_sessionId;
    mutable uint32_t m_lastCounter;
    mutable unsigned int m_failedAttempts;
    mutable int64_t m_sessionExpiresAt;
    mutable int64_t m_lockUntil;
    mutable bool m_secretReady;
    mutable bool m_challengeReady;
    mutable bool m_authenticated;
};

} // namespace BleProvisioning
