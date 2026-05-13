/**
* @FilePath     : ourSHA256.cpp
* @Author       : 
* @Date         : 
* @LastEditors  : liuhm
* @LastEditTime : 2025-04-17 16:29:46
* @Descripttion : 
*/


#include "ourSHA256.hh"
#include <NetCommon.h>
#include <string.h>
#include <algorithm> 

#define SHA256_DIGEST_SIZE 32
#define SHA256_BLOCK_SIZE 64
#define SHA256_DIGEST_HEX_SIZE (2*SHA256_DIGEST_SIZE + 1)

class SHA256Context {
public:
    SHA256Context();
    ~SHA256Context();

    void addData(const uint8_t* data, size_t len);
    void finalize(uint8_t digest[SHA256_DIGEST_SIZE]);
    void end(char hexDigest[SHA256_DIGEST_HEX_SIZE]);

private:
    void transform(const uint8_t block[SHA256_BLOCK_SIZE]);
    void pad();
    void zeroize();

private:
    uint32_t state[8];          // 哈希状态
    uint64_t bitCount;          // 总位数
    uint8_t buffer[SHA256_BLOCK_SIZE]; // 数据缓冲区
    uint32_t bufferOffset;      // 缓冲区当前偏移
};

// 清理敏感数据
void SHA256Context::zeroize() {
    memset(state, 0, sizeof(state));
    memset(buffer, 0, sizeof(buffer));
    bitCount = bufferOffset = 0;
}

// 公共接口函数（示例）
void our_SHA256Data(unsigned char* data, unsigned len, char* output) {
    SHA256Context ctx;
    ctx.addData(data, len);
    if (!output) output = new char[SHA256_DIGEST_HEX_SIZE];
    ctx.end(output);
}

// 初始化哈希上下文
SHA256Context::SHA256Context() : bitCount(0), bufferOffset(0) {
    // 初始哈希值（Big-Endian）
    state[0] = 0x6a09e667;
    state[1] = 0xbb67ae85;
    state[2] = 0x3c6ef372;
    state[3] = 0xa54ff53a;
    state[4] = 0x510e527f;
    state[5] = 0x9b05688c;
    state[6] = 0x1f83d9ab;
    state[7] = 0x5be0cd19;
}

SHA256Context::~SHA256Context() {
  zeroize();
}
// 添加数据到哈希流
void SHA256Context::addData(const uint8_t* data, size_t len) {
    bitCount += len * 8;

    // 处理缓冲区剩余空间
    if (bufferOffset > 0) {
        size_t copyLen = std::min(len, SHA256_BLOCK_SIZE - bufferOffset);
        memcpy(buffer + bufferOffset, data, copyLen);
        bufferOffset += copyLen;
        data += copyLen;
        len -= copyLen;

        if (bufferOffset == SHA256_BLOCK_SIZE) {
            transform(buffer);
            bufferOffset = 0;
        }
    }

    // 处理完整块
    while (len >= SHA256_BLOCK_SIZE) {
        transform(data);
        data += SHA256_BLOCK_SIZE;
        len -= SHA256_BLOCK_SIZE;
    }

    // 保存剩余数据到缓冲区
    if (len > 0) {
        memcpy(buffer, data, len);
        bufferOffset = len;
    }
}

// 标准填充规则：0x80 + 0x00... + 64位长度
void SHA256Context::pad() {
    buffer[bufferOffset++] = 0x80;
    if (bufferOffset > 56) { // 需要额外转换
        memset(buffer + bufferOffset, 0, SHA256_BLOCK_SIZE - bufferOffset);
        transform(buffer);
        bufferOffset = 0;
    }
    memset(buffer + bufferOffset, 0, 56 - bufferOffset);
    
    // 附加总位数（Big-Endian 64位）
    uint64_t bits = htobe64(bitCount);
    memcpy(buffer + 56, &bits, 8);
    transform(buffer);
}

// SHA256轮函数宏定义
#define ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
#define CH(x, y, z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define EP0(x) (ROTR(x, 2) ^ ROTR(x,13) ^ ROTR(x,22))
#define EP1(x) (ROTR(x, 6) ^ ROTR(x,11) ^ ROTR(x,25))
#define SIG0(x) (ROTR(x, 7) ^ ROTR(x,18) ^ ((x) >> 3))
#define SIG1(x) (ROTR(x,17) ^ ROTR(x,19) ^ ((x) >>10))

// 常量数组K
static const uint32_t K[64] = {
    0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL,
    0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
    0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL,
    0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
    0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
    0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
    0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL,
    0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
    0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL,
    0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
    0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL,
    0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
    0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL,
    0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
    0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
    0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL
};

void SHA256Context::transform(const uint8_t block[SHA256_BLOCK_SIZE]) {
    uint32_t W[64];
    uint32_t a, b, c, d, e, f, g, h, t1, t2;

    // 将块转换为32位大端字
    for (int i = 0; i < 16; ++i) {
        W[i] = be32toh(*((uint32_t*)(block + i*4)));
    }

    // 扩展消息
    for (int i = 16; i < 64; ++i) {
        W[i] = SIG1(W[i-2]) + W[i-7] + SIG0(W[i-15]) + W[i-16];
    }

    // 初始化工作变量
    a = state[0]; b = state[1]; c = state[2]; d = state[3];
    e = state[4]; f = state[5]; g = state[6]; h = state[7];

    // 64轮循环
    for (int i = 0; i < 64; ++i) {
        t1 = h + EP1(e) + CH(e,f,g) + K[i] + W[i];
        t2 = EP0(a) + MAJ(a,b,c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }

    // 更新状态
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

void SHA256Context::finalize(uint8_t digest[SHA256_DIGEST_SIZE]) {
    pad();
    
    // 转换为大端序输出
    for (int i = 0; i < 8; ++i) {
        uint32_t val = htobe32(state[i]);
        memcpy(digest + i*4, &val, 4);
    }
    zeroize();
}

void SHA256Context::end(char hexDigest[SHA256_DIGEST_HEX_SIZE]) {
    uint8_t digest[SHA256_DIGEST_SIZE];
    finalize(digest);

    static const char hex[] = "0123456789abcdef";
    for (int i = 0; i < SHA256_DIGEST_SIZE; ++i) {
        hexDigest[2*i]   = hex[digest[i] >> 4];
        hexDigest[2*i+1] = hex[digest[i] & 0x0F];
    }
    hexDigest[2*SHA256_DIGEST_SIZE] = '\0';
}
