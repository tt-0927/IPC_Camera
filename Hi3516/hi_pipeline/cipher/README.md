# 安全子系统 API

海思 Hi3516CV610-20G 平台安全子系统 API 封装

## 目录结构

### cipher/ - 安全算法模块

**功能描述**  
CIPHER DRIVER 提供以下安全算法功能：

• **对称加解密算法**  
  AES、SM4、TDES  
• **摘要算法**  
  HASH、SM3 及 HASH-MAC、SM3-MAC  
• **密钥派生算法**  
  PBKDF2  
• **非对称算法**  
  RSA、ECC、SM2（支持签名验签、密钥协商、加解密）  
• **大数运算**
• **随机数算法**

**架构组成**  
包含 3 个子模块：
1. SPACC（Security Protocol Accelerator，安全协议加速器）  
  实现对称加解密算法、HASH及HMAC摘要算法
2. PKE（Public Key Encryption，公钥加密），非对称加解密算法。  
    常用于文件的签名和验签，以及非对称密钥的加解密
3. TRNG  
  随机数获取模块，获取硬件产生的真随机数（满足FIPS 140-2标准）

**接口层级**  
• **MPI 层**：用户层接口  
• **KAPI 层**：内核模块接口  

---

### cipher_context 统一门面

`cipher_context` 是业务仓库与安全子系统交互的统一入口。业务侧不直接调用 `ot_mpi_cipher_*`，而是通过 `cipherContext_alloc/init/uninit/release` 管理生命周期，通过上下文能力函数使用 SM3、SM4、TRNG、PKE 等硬件能力。

`hi_pipeline` 内部日志统一使用 `mpi_common.h` 的模块日志宏，例如 `mpi_cipher_log`，禁止依赖业务侧 `dlog_*`。

SM4-CBC 需要物理连续内存与 KEYSLOT/KLAD 支持，相关资源申请、绑定与释放必须封装在 `hi_pipeline/cipher` 内部。OpenSSL Provider 只负责参数适配。

当前 HiSilicon OpenSSL Provider 仅通过 `cipher_context` 注册 SM3 与 TRNG。SM4-CBC 在 `cipher_context` 完成物理连续内存和 KEYSLOT/KLAD 收口前不注册硬件 Provider，避免业务侧保留伪物理内存或直接调用底层 MPI。

---

### km/ - 密钥管理模块

**功能描述**  
提供密钥管理功能：  
• 明文 KEY 传递  
• ROOTKEY 传递  

**接口层级**  
• **MPI 层**  
  用户层应用调用，为 CA/TA 提供密钥管理能力  
• **KAPI 层**  
  内核模块调用，为内核模块提供密钥管理能力  

---

### otp/ - OTP 存储模块

**功能描述**  
实现 OTP 非易失性存储器的读写操作，支持：  
• 按字节读  
• 按字节写  
• 按字读  

**接口层级**  
• **MPI 层**  
  用户层应用调用，为 CA/TA 提供 OTP 读写能力  
• **KAPI 层**  
  内核模块调用，为内核模块提供 OTP 读写能力  

---

> 注：所有模块均提供 MPI（用户层）和 KAPI（内核层）两级接口
