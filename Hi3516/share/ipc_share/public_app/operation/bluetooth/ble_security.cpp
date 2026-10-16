#include "ble_security.h"

#include "dlog.h"
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <cerrno>
#include <sys/stat.h>

namespace BleProvisioning
{
namespace
{
const char *kSecretPath = "/opt/course/config/ble_setup_secret";
const unsigned int kMaxAuthFailures = 5;
const int64_t kAuthLockSeconds = 60;
const int64_t kSessionSeconds = 300;

/* 从安全信封中读取一个4字节大端整数。 */
uint32_t readU32(const uint8_t *p)
{
    return (static_cast<uint32_t>(p[0]) << 24) |
           (static_cast<uint32_t>(p[1]) << 16) |
           (static_cast<uint32_t>(p[2]) << 8) | p[3];
}

/* 把32位整数按大端序追加到认证或 AAD字节数组。 */
void appendU32(std::vector<uint8_t> &out, uint32_t value)
{
    out.push_back(static_cast<uint8_t>(value >> 24));
    out.push_back(static_cast<uint8_t>(value >> 16));
    out.push_back(static_cast<uint8_t>(value >> 8));
    out.push_back(static_cast<uint8_t>(value));
}

/* 把单个十六进制字符转换为0～15；非法字符返回-1。 */
int hexValue(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

/* 统一封装 HMAC-SHA256计算并验证输出长度为32字节。 */
bool hmacSha256(const uint8_t *key, std::size_t keySize,
                const uint8_t *data, std::size_t dataSize,
                uint8_t output[32])
{
    unsigned int length = 0;
    return HMAC(EVP_sha256(), key, static_cast<int>(keySize), data, dataSize,
                output, &length) != NULL && length == 32;
}
} // namespace

BleSecurityManager::BleSecurityManager()
    : m_sessionId(0), m_lastCounter(0), m_failedAttempts(0),
      m_sessionExpiresAt(0), m_lockUntil(0), m_secretReady(false),
      m_challengeReady(false), m_authenticated(false)
{
    m_setupSecret.fill(0);
    m_sessionKey.fill(0);
    m_appNonce.fill(0);
    m_deviceNonce.fill(0);
}

bool BleSecurityManager::initialize()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_secretReady)
        return true;
    return loadOrCreateSecret();
}

bool BleSecurityManager::loadOrCreateSecret()
{
    /*
     * setupSecret 是每台设备唯一的长期密钥，二维码应携带同一份值。
     * 文件采用 64 个十六进制字符保存 32 字节密钥，并限制为 root 读写。
     */
    FILE *file = std::fopen(kSecretPath, "rb");
    if (file)
    {
        char text[65] = {0};
        const std::size_t readSize = std::fread(text, 1, 64, file);
        std::fclose(file);
        if (readSize == 64)
        {
            for (std::size_t i = 0; i < m_setupSecret.size(); ++i)
            {
                const int high = hexValue(text[i * 2]);
                const int low = hexValue(text[i * 2 + 1]);
                if (high < 0 || low < 0)
                    return false;
                m_setupSecret[i] = static_cast<uint8_t>((high << 4) | low);
            }
            m_secretReady = true;
            return true;
        }
        return false;
    }

    if (mkdir("/opt/course/config", S_IRWXU) != 0 && errno != EEXIST)
        return false;
    if (RAND_bytes(m_setupSecret.data(), m_setupSecret.size()) != 1)
        return false;
    file = std::fopen(kSecretPath, "wb");
    if (!file)
        return false;
    static const char hex[] = "0123456789ABCDEF";
    char text[64];
    for (std::size_t i = 0; i < m_setupSecret.size(); ++i)
    {
        text[i * 2] = hex[m_setupSecret[i] >> 4];
        text[i * 2 + 1] = hex[m_setupSecret[i] & 0x0f];
    }
    const bool written = std::fwrite(text, 1, sizeof(text), file) == sizeof(text);
    std::fclose(file);
    chmod(kSecretPath, S_IRUSR | S_IWUSR);
    m_secretReady = written;
    if (written)
        dlog_info("[BLE安全] 已生成设备独立配网密钥，文件=%s", kSecretPath);
    return written;
}

void BleSecurityManager::clearSessionLocked() const
{
    /* OPENSSL_cleanse 防止临时会话密钥仅被编译器优化成普通内存丢弃。 */
    OPENSSL_cleanse(m_sessionKey.data(), m_sessionKey.size());
    m_sessionId = 0;
    m_lastCounter = 0;
    m_sessionExpiresAt = 0;
    m_challengeReady = false;
    m_authenticated = false;
}

void BleSecurityManager::expireSessionLocked() const
{
    if ((m_challengeReady || m_authenticated) && std::time(NULL) >= m_sessionExpiresAt)
        clearSessionLocked();
}

bool BleSecurityManager::deriveSessionKeyLocked()
{
    /*
     * 使用 appNonce || deviceNonce 作为 salt，通过两步 HMAC 派生会话密钥。
     * 每次认证随机数均不同，因此长期 setupSecret 不会直接暴露给业务加密。
     */
    uint8_t salt[32];
    std::memcpy(salt, m_appNonce.data(), 16);
    std::memcpy(salt + 16, m_deviceNonce.data(), 16);
    uint8_t prk[32];
    if (!hmacSha256(salt, sizeof(salt), m_setupSecret.data(), m_setupSecret.size(), prk))
        return false;
    static const uint8_t info[] = "TV-BLE-PROVISION-V1\x01";
    const bool ok = hmacSha256(prk, sizeof(prk), info, sizeof(info) - 1,
                               m_sessionKey.data());
    OPENSSL_cleanse(prk, sizeof(prk));
    return ok;
}

CommandResult BleSecurityManager::beginAuthentication(const CommandFrame &frame)
{
    CommandResult result;
    std::lock_guard<std::mutex> lock(m_mutex);
    expireSessionLocked();
    const int64_t now = std::time(NULL);
    if (!m_secretReady || frame.payload.size() != 16)
    {
        result.status = STATUS_INVALID_PAYLOAD;
        return result;
    }
    if (now < m_lockUntil)
    {
        result.status = STATUS_AUTH_LOCKED;
        return result;
    }
    if (m_authenticated)
    {
        result.status = STATUS_BUSY;
        return result;
    }
    /* App 先提供 16 字节随机数，设备再生成自己的随机数及非零 sessionId。 */
    std::copy(frame.payload.begin(), frame.payload.end(), m_appNonce.begin());
    if (RAND_bytes(m_deviceNonce.data(), m_deviceNonce.size()) != 1 ||
        RAND_bytes(reinterpret_cast<unsigned char *>(&m_sessionId), sizeof(m_sessionId)) != 1 ||
        m_sessionId == 0 || !deriveSessionKeyLocked())
    {
        clearSessionLocked();
        result.status = STATUS_INTERNAL_ERROR;
        return result;
    }
    m_challengeReady = true;
    m_sessionExpiresAt = now + kSessionSeconds;
    appendU32(result.payload, m_sessionId);
    result.payload.insert(result.payload.end(), m_deviceNonce.begin(), m_deviceNonce.end());
    return result;
}

CommandResult BleSecurityManager::proveAuthentication(const CommandFrame &frame)
{
    CommandResult result;
    std::lock_guard<std::mutex> lock(m_mutex);
    expireSessionLocked();
    if (!m_challengeReady || frame.payload.size() != 36 ||
        readU32(frame.payload.data()) != m_sessionId)
    {
        result.status = STATUS_UNAUTHORIZED;
        return result;
    }
    /*
     * App 证明：HMAC(setupSecret,
     *                  "TV-BLE-AUTH-V1" || sessionId || appNonce || deviceNonce)。
     * CRYPTO_memcmp 使用恒定时间比较，降低时序侧信道风险。
     */
    std::vector<uint8_t> input;
    static const char label[] = "TV-BLE-AUTH-V1";
    input.insert(input.end(), label, label + sizeof(label) - 1);
    appendU32(input, m_sessionId);
    input.insert(input.end(), m_appNonce.begin(), m_appNonce.end());
    input.insert(input.end(), m_deviceNonce.begin(), m_deviceNonce.end());
    uint8_t expected[32];
    const bool calculated = hmacSha256(m_setupSecret.data(), m_setupSecret.size(),
                                       input.data(), input.size(), expected);
    const bool matched = calculated &&
        CRYPTO_memcmp(expected, frame.payload.data() + 4, sizeof(expected)) == 0;
    OPENSSL_cleanse(expected, sizeof(expected));
    if (!matched)
    {
        /* 连续失败达到阈值后短时锁定，减缓近距离暴力尝试。 */
        ++m_failedAttempts;
        clearSessionLocked();
        if (m_failedAttempts >= kMaxAuthFailures)
        {
            m_lockUntil = std::time(NULL) + kAuthLockSeconds;
            m_failedAttempts = 0;
            result.status = STATUS_AUTH_LOCKED;
        }
        else
        {
            result.status = STATUS_AUTH_FAILED;
        }
        return result;
    }
    m_authenticated = true;
    m_challengeReady = false;
    m_lastCounter = 0;
    m_failedAttempts = 0;
    m_sessionExpiresAt = std::time(NULL) + kSessionSeconds;
    const char ok[] = "authenticated";
    result.payload.assign(ok, ok + sizeof(ok) - 1);
    return result;
}

CommandResult BleSecurityManager::authenticationStatus() const
{
    CommandResult result;
    std::lock_guard<std::mutex> lock(m_mutex);
    expireSessionLocked();
    const char *state = m_authenticated ? "authenticated" : "unauthenticated";
    result.payload.assign(state, state + std::strlen(state));
    return result;
}

void BleSecurityManager::resetSession()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    clearSessionLocked();
}

bool BleSecurityManager::requiresSecureSession(uint16_t command)
{
    /* 认证和心跳保持明文；所有 Wi-Fi、平台业务命令必须使用安全信封。 */
    return command >= CMD_WIFI_SET_CREDENTIALS && command <= CMD_PLATFORM_GET_STATUS;
}

StatusCode BleSecurityManager::decryptProvisioningPayload(
    const CommandFrame &frame, std::vector<uint8_t> &plainText)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    expireSessionLocked();
    /*
     * 安全信封：sessionId(4) + counter(4) + nonce(12) +
     *           ciphertext(0..N) + GCM tag(16)。
     * counter 必须在同一会话内严格递增，用于拒绝重复发送的旧密文。
     */
    if (!m_authenticated || frame.payload.size() < 36)
        return STATUS_UNAUTHORIZED;
    const uint32_t sessionId = readU32(frame.payload.data());
    const uint32_t counter = readU32(frame.payload.data() + 4);
    if (sessionId != m_sessionId)
        return STATUS_UNAUTHORIZED;
    if (counter <= m_lastCounter)
        return STATUS_REPLAY_DETECTED;

    const uint8_t *nonce = frame.payload.data() + 8;
    const uint8_t *cipher = frame.payload.data() + 20;
    const std::size_t cipherSize = frame.payload.size() - 36;
    const uint8_t *tag = frame.payload.data() + frame.payload.size() - 16;
    /*
     * AAD 不加密但参与 GCM 完整性校验，将命令码、sequence、sessionId 和
     * counter 与密文绑定，攻击者无法把合法密文挪用到另一条命令。
     */
    std::vector<uint8_t> aad;
    aad.push_back(frame.version);
    aad.push_back(static_cast<uint8_t>(frame.command >> 8));
    aad.push_back(static_cast<uint8_t>(frame.command));
    aad.push_back(static_cast<uint8_t>(frame.sequence >> 8));
    aad.push_back(static_cast<uint8_t>(frame.sequence));
    appendU32(aad, sessionId);
    appendU32(aad, counter);

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx)
        return STATUS_INTERNAL_ERROR;
    plainText.assign(cipherSize, 0);
    uint8_t emptyOutput = 0;
    uint8_t *plainOutput = cipherSize ? plainText.data() : &emptyOutput;
    int outLength = 0;
    int finalLength = 0;
    bool ok = EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) == 1 &&
              EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, 12, NULL) == 1 &&
              EVP_DecryptInit_ex(ctx, NULL, NULL, m_sessionKey.data(), nonce) == 1 &&
              EVP_DecryptUpdate(ctx, NULL, &outLength, aad.data(),
                                static_cast<int>(aad.size())) == 1 &&
              EVP_DecryptUpdate(ctx, plainOutput, &outLength, cipher,
                                static_cast<int>(cipherSize)) == 1 &&
              EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16,
                                  const_cast<uint8_t *>(tag)) == 1 &&
              EVP_DecryptFinal_ex(ctx, plainOutput + outLength, &finalLength) == 1;
    EVP_CIPHER_CTX_free(ctx);
    /* DecryptFinal 同时校验 GCM tag；失败时绝不能使用任何已输出的明文。 */
    if (!ok)
    {
        plainText.clear();
        return STATUS_AUTH_FAILED;
    }
    plainText.resize(static_cast<std::size_t>(outLength + finalLength));
    /* 只有完整认证成功后才推进 counter，避免无效报文消耗合法序号。 */
    m_lastCounter = counter;
    m_sessionExpiresAt = std::time(NULL) + kSessionSeconds;
    return STATUS_OK;
}

} // namespace BleProvisioning
