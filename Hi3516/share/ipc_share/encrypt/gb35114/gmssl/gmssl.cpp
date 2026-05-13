/**
 * @FilePath     : gmssl.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-04-21 10:09:53
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2025-08-25 19:47:11
 * @Description  : gmssl命令行封装
 */

#include "gmssl.h"

/*待签名数据的临时文件路径*/
#define SM2_SIGN_INPUT_FILE "/tmp/sm2_sign.der"
/*签名结果数据的临时文件路径*/
#define SM2_SIGN_RESULT_FILE "/tmp/sm2_sign_result.der"
/*待解密数据的临时文件路径*/
#define SM2_DECRYPT_FILE "/tmp/sm2_decrypt.der"
/*解密结果数据的临时文件路径*/
#define SM2_DECRYPT_RESULT_FILE "/tmp/sm2_decrypt_result.der"
/*待杂凑的临时文件路径*/
#define SM3_HASH_FILE "/tmp/sm3.der"
/*杂凑结果数据的临时文件路径*/
#define SM3_HASH_RESULT_FILE "/tmp/sm3_result.der"

CGmSSL* CGmSSL::m_self = NULL;
std::mutex CGmSSL::m_mutex;

CGmSSL::CGmSSL()
{
}

CGmSSL::~CGmSSL()
{
}

std::string CGmSSL::rand(size_t length, bool hex_output)
{
    std::ostringstream ossCmd;
    ossCmd << TOOLS_PATH
           << "gmssl rand -outlen " << std::to_string(length);
    if (hex_output)
    {
        ossCmd << " -hex";
    }
    return exec(ossCmd.str().c_str());
}

void CGmSSL::sm2keygen(const std::string &pass, const std::string &privkey_path, const std::string &pubkey_path)
{
    std::ostringstream ossCmd;
    ossCmd << TOOLS_PATH 
           << "gmssl sm2keygen -pass " << pass
           << " -out " << privkey_path
           << " -pubout " << pubkey_path;
    exec(ossCmd.str().c_str());
}

void CGmSSL::sm2keygen(const std::string &pass, const std::string &privkey_path)
{
    std::ostringstream ossCmd;
    ossCmd << TOOLS_PATH 
           << "gmssl sm2keygen -pass " << pass
           << " -out " << privkey_path;
    exec(ossCmd.str().c_str());
}

std::string CGmSSL::sm2sign(const std::string &privkey_path, const std::string &pass,
                            const std::string &data, const std::string &id)
{
    std::string tmpfile = create_tempfile(data);
    std::ostringstream ossCmd;
    ossCmd << TOOLS_PATH 
           << "gmssl sm2sign -key " << privkey_path
           << " -pass " << pass
           << " -id " << id
           << " -in " << tmpfile;
    auto result = exec(ossCmd.str().c_str());
    remove_tempfile(tmpfile);
    return result;
}

bool CGmSSL::sm2verify(const std::string &pubkey_path, const std::string &data,
                       const std::string &signature, const std::string &id)
{
    std::string data_tmp = create_tempfile(data);
    std::string sig_tmp = create_tempfile(signature);

    try
    {
        std::ostringstream ossCmd;
        ossCmd << TOOLS_PATH 
               << "gmssl sm2verify -pubkey " << pubkey_path
               << " -id " << id
               << " -in " << data_tmp
               << " -sig " << sig_tmp;
        exec(ossCmd.str().c_str());
        remove_tempfile(data_tmp);
        remove_tempfile(sig_tmp);
        return true;
    }
    catch (const std::runtime_error &e)
    {
        remove_tempfile(data_tmp);
        remove_tempfile(sig_tmp);
        return false;
    }
}

std::string CGmSSL::sm2decrypt(const std::vector<uint8_t> &bytesInput, const char *strKeyPath)
{
    /*写入待解密数据到临时文件*/
    writeBytesToFile(SM2_DECRYPT_FILE, bytesInput);

    /*使用 SM2 解密*/
    std::ostringstream ossCmd;
    ossCmd << TOOLS_PATH 
           << "gmssl sm2decrypt"
           << " -key " << strKeyPath
           << " -pass " << SM2_PRIVATE_KEY_ENCRYPT_PASSWORD
           << " -in " << SM2_DECRYPT_FILE
           << " -out " << SM2_DECRYPT_RESULT_FILE;
    exec(ossCmd.str());

    /*读取解密后的内容*/
    return readFileToString(SM2_DECRYPT_RESULT_FILE);
}

std::vector<uint8_t> CGmSSL::sm3(const std::vector<uint8_t> &bytesInput)
{
    /*写入待解密数据到临时文件*/
    writeBytesToFile(SM3_HASH_FILE, bytesInput);
    
    /*使用 SM2 解密*/
    std::ostringstream ossCmd;
    ossCmd << TOOLS_PATH 
           << "gmssl sm3"
           << " -bin "
           << " -in " << SM3_HASH_FILE 
           << " -out " << SM3_HASH_RESULT_FILE;
    exec(ossCmd.str());

    /*读取解密后的内容*/
    return readFileToBytes(SM3_HASH_RESULT_FILE);
}

std::string CGmSSL::sm4_encrypt_cbc(const std::string &key, const std::string &iv,
                                    const std::string &plaintext)
{
    return sm4_crypt(key, iv, plaintext, true, true);
}

std::string CGmSSL::sm4_decrypt_cbc(const std::string &key, const std::string &iv,
                                    const std::string &ciphertext)
{
    return sm4_crypt(key, iv, ciphertext, false, true);
}

bool CGmSSL::reqgen(const std::string &strCountry,
                    const std::string &strState,
                    const std::string &strLocality,
                    const std::string &strOrganization,
                    const std::string &strOrganizationUnit,
                    const std::string &strCommonName,
                    const std::string &strKeyFile,
                    const std::string &strPassword,
                    const std::string &strOutputFile)
{
    /* 参数验证 */
    if (strCommonName.empty() || strKeyFile.empty() || strPassword.empty() || strOutputFile.empty())
    {
        std::cerr << "Error: Required parameters cannot be empty" << std::endl;
        return false;
    }

    /* 构建gmssl reqgen命令 */
    std::ostringstream ossCmd;
    ossCmd << TOOLS_PATH << "gmssl reqgen";

    /* 添加证书主题信息参数 */
    if (!strCountry.empty())
    {
        ossCmd << " -C " << strCountry;
    }
    if (!strState.empty())
    {
        ossCmd << " -ST " << strState;
    }
    if (!strLocality.empty())
    {
        ossCmd << " -L " << strLocality;
    }
    if (!strOrganization.empty())
    {
        ossCmd << " -O \"" << strOrganization << "\"";
    }
    if (!strOrganizationUnit.empty())
    {
        ossCmd << " -OU \"" << strOrganizationUnit << "\"";
    }

    /* 添加必需参数 */
    ossCmd << " -CN " << strCommonName << " -key " << strKeyFile << " -pass " << strPassword << " -out "
                 << strOutputFile;

    /* 执行命令 */
    std::cout << "Executing: " << ossCmd.str() << std::endl;
    exec(ossCmd.str());

    return true;
}

std::string CGmSSL::reqparse(const std::string &strPath)
{
    /* 参数验证 */
    if (strPath.empty())
    {
        dlog_error("参数为空");
        return std::string();
    }

    /* 构建gmssl reqparse命令 */
    std::ostringstream ossCmd;
    ossCmd << TOOLS_PATH << "gmssl reqparse";

    /* 添加证书参数 */
    if (!strPath.empty())
    {
        ossCmd << " -in " << strPath;
    }

    /* 执行命令 */
    dlog_info("Executing: %s", ossCmd.str().c_str());
    return exec(ossCmd.str());
}

std::string CGmSSL::crlparse(const std::string &strPath)
{
    /* 参数验证 */
    if (strPath.empty())
    {
        dlog_error("参数为空");
        return std::string();
    }

    /* 构建gmssl crlparse 命令 */
    std::ostringstream ossCmd;
    ossCmd << TOOLS_PATH << "gmssl crlparse";

    /* 添加证书参数 */
    if (!strPath.empty())
    {
        ossCmd << " -in " << strPath;
    }

    /* 执行命令 */
    dlog_info("Executing: %s", ossCmd.str().c_str());
    return exec(ossCmd.str());
}

std::string CGmSSL::crlverify(const std::string &strCrlPath,const std::string &strCertPath)
{
    /* 参数验证 */
    if (strCrlPath.empty() || strCertPath.empty())
    {
        dlog_error("参数为空");
        return std::string();
    }

    /* 构建gmssl crlverify 命令 */
    std::ostringstream ossCmd;
    ossCmd << TOOLS_PATH << "gmssl crlverify";

    /* 添加证书参数 */
    ossCmd << " -in " << strCrlPath << " -cacert " << strCertPath;

    /* 执行命令 */
    dlog_info("Executing: %s", ossCmd.str().c_str());
    return exec(ossCmd.str());
}

std::string CGmSSL::certparse(const std::string &strPath)
{
    /* 参数验证 */
    if (strPath.empty())
    {
        dlog_error("参数为空");
        return std::string();
    }

    /* 构建gmssl certparse 命令 */
    std::ostringstream ossCmd;
    ossCmd << TOOLS_PATH << "gmssl certparse";

    /* 添加证书参数 */
    if (!strPath.empty())
    {
        ossCmd << " -in " << strPath;
    }

    /* 执行命令 */
    dlog_info("Executing: %s", ossCmd.str().c_str());
    return exec(ossCmd.str());
}

int CGmSSL::readCert_from_file(const std::string &strCertPath)
{
    return OK;
}

int CGmSSL::digitalSignature(const std::vector<uint8_t> &inputData, std::string &strOutData, const char *strKeyPath)
{
    /*写入待签名数据到临时文件*/
    writeBytesToFile(SM2_SIGN_INPUT_FILE, inputData);

    /*使用 SM2 签名*/
    std::ostringstream ossCmd;
    ossCmd << TOOLS_PATH 
           << "gmssl sm2sign"
           << " -key " << strKeyPath
           << " -pass " << SM2_PRIVATE_KEY_ENCRYPT_PASSWORD
           << " -id " << SM2_DEFAULT_ID
           << " -in " << SM2_SIGN_INPUT_FILE
           << " -out " << SM2_SIGN_RESULT_FILE;
    exec(ossCmd.str());

    /*读取签名内容*/
    std::string strSig = readFileToString(SM2_SIGN_RESULT_FILE);

    /*Base64 编码*/
    strOutData = base64EncodeToString(strSig);
    /*去除换行符*/
    strOutData.erase(std::remove(strOutData.begin(), strOutData.end(), '\n'), strOutData.end());

    return OK;
}

int CGmSSL::verifySignature(const std::vector<uint8_t> &bytesSignData, const std::vector<uint8_t> &strVerifySign, const char *strCertPath)
{
    /*写入签名数据到临时文件*/
    writeBytesToFile(SM2_SIGN_INPUT_FILE, bytesSignData);
    /*写入签名结果数据到临时文件*/
    writeBytesToFile(SM2_SIGN_RESULT_FILE, strVerifySign);

    /*使用 SM2 验签*/
    std::ostringstream ossCmd;
    ossCmd << TOOLS_PATH 
           << "gmssl sm2verify"
           << " -cert " << strCertPath
           << " -id " << SM2_DEFAULT_ID
           << " -in " << SM2_SIGN_INPUT_FILE
           << " -sig " << SM2_SIGN_RESULT_FILE;
    std::string strResult = exec(ossCmd.str());
    if(strResult != "verify : success")
    {
        dlog_error("SM2 验签失败");
        return ERR;
    }

    return OK;
}
/*base64加密*/
std::vector<uint8_t> CGmSSL::base64Encode(const std::vector<uint8_t>& bytesInput)
{
    std::vector<uint8_t> bytesOutput;

    if (bytesInput.empty()) {
        return bytesOutput;
    }

    BASE64_CTX ctx;
    base64_encode_init(&ctx);

    size_t szMaxLen = BASE64_ENCODE_LENGTH(bytesInput.size());
    bytesOutput.resize(szMaxLen);

    int nOutlen = 0;
    if (base64_encode_update(&ctx, bytesInput.data(), static_cast<int>(bytesInput.size()), bytesOutput.data(), &nOutlen) != TRUE)
    {
        dlog_error("base64编码失败");
        return {};
    }

    int nFinalLen = 0;
    base64_encode_finish(&ctx, bytesOutput.data() + nOutlen, &nFinalLen);

    bytesOutput.resize(nOutlen + nFinalLen);
    return bytesOutput;
}

/*base64加密*/
std::vector<uint8_t> CGmSSL::base64Encode(const std::string &strInput)
{
    std::vector<uint8_t> bytesEncoded;

    if (strInput.empty())
    {
        return bytesEncoded;
    }

    BASE64_CTX ctx;

    /*计算最大可能的输出长度并预留空间*/
    size_t szMaxLen = BASE64_ENCODE_LENGTH(strInput.size());
    bytesEncoded.resize(szMaxLen);

    base64_encode_init(&ctx);

    int nOutlen = 0;
    if (base64_encode_update(&ctx, reinterpret_cast<const uint8_t *>(strInput.data()), static_cast<int>(strInput.size()), bytesEncoded.data(), &nOutlen) != TRUE)
    {
        bytesEncoded.clear();
        return bytesEncoded;
    }

    int nFinalLen = 0;
    base64_encode_finish(&ctx, bytesEncoded.data() + nOutlen, &nFinalLen);

    /*修正向量大小为实际有效输出长度c*/
    bytesEncoded.resize(nOutlen + nFinalLen);

    /*去除末尾换行符（如果存在）*/
    while (!bytesEncoded.empty() && bytesEncoded.back() == '\n')
    {
        bytesEncoded.pop_back();
    }

    return bytesEncoded;
}

/*base64加密*/
std::string CGmSSL::base64EncodeToString(const std::string &strInput)
{
    if (strInput.empty())
    {
        dlog_error("输入加密数据为空");
        return "";
    }

    BASE64_CTX ctx;
    std::string strOutput;
    /*计算最大可能的输出长度并预留空间*/
    size_t szMaxLen = BASE64_ENCODE_LENGTH(strInput.size());
    strOutput.resize(szMaxLen);
    /*初始化编码上下文*/
    base64_encode_init(&ctx);
    /*处理输入数据*/
    int nOutlen = 0;
    if (base64_encode_update(&ctx, reinterpret_cast<const uint8_t *>(strInput.data()), static_cast<int>(strInput.size()), reinterpret_cast<uint8_t *>(&strOutput[0]), &nOutlen) != TRUE)
    {
        dlog_error("base64编码失败");
        return "";
    }
    int nFinalLen = 0;
    base64_encode_finish(&ctx, reinterpret_cast<uint8_t *>(&strOutput[nOutlen]), &nFinalLen);
    /*调整输出字符串大小为实际编码后的长度*/
    strOutput.resize(nOutlen + nFinalLen);

    return strOutput;
}

/*base64加密*/
std::string CGmSSL::base64EncodeToString(const std::vector<uint8_t> &bytesInput)
{
    BASE64_CTX ctx;
    base64_encode_init(&ctx);

    std::vector<uint8_t> bytesOutBuf(BASE64_ENCODE_LENGTH(bytesInput.size()));
    int nOutlen = 0, tmplen = 0;

    base64_encode_update(&ctx, bytesInput.data(), bytesInput.size(), bytesOutBuf.data(), &nOutlen);
    base64_encode_finish(&ctx, bytesOutBuf.data() + nOutlen, &tmplen);
    nOutlen += tmplen;

    /*转换为字符串*/
    std::string result((char *)bytesOutBuf.data(), nOutlen);

    /*如果最后一个字符是换行符，则移除它*/
    if (!result.empty() && result.back() == '\n')
    {
        result.pop_back();
    }

    return result;
}

/*base64解密*/
std::vector<uint8_t> CGmSSL::base64Decode(const char *pInput)
{
    /*去除前后的双引号*/
    std::string strInput(pInput);
    if (!strInput.empty() && strInput.front() == '"')
    {
        strInput = strInput.substr(1, strInput.length() - 2);
    }

    BASE64_CTX ctx;
    base64_decode_init(&ctx);

    int nInlen = strInput.length();
    std::vector<uint8_t> bytesDecoded(BASE64_DECODE_LENGTH(nInlen));
    int nOutLen = 0, nTmpLen = 0;

    base64_decode_update(&ctx, (const uint8_t *)strInput.data(), nInlen, bytesDecoded.data(), &nOutLen);
    base64_decode_finish(&ctx, bytesDecoded.data() + nOutLen, &nTmpLen);
    nOutLen += nTmpLen;

    bytesDecoded.resize(nOutLen);
    return bytesDecoded;
}

/*base64解密*/
std::vector<uint8_t> CGmSSL::base64Decode(const std::string &strInput)
{
    /*去除前后的双引号*/
    std::string strData = strInput;
    if (!strData.empty() && strData.front() == '"')
    {
        strData = strData.substr(1, strData.length() - 2);
    }

    BASE64_CTX ctx;
    base64_decode_init(&ctx);

    int nInlen = strData.length();
    std::vector<uint8_t> bytesDecoded(BASE64_DECODE_LENGTH(nInlen));
    int nOutLen = 0, nTmpLen = 0;

    base64_decode_update(&ctx, (const uint8_t *)strData.data(), nInlen, bytesDecoded.data(), &nOutLen);
    base64_decode_finish(&ctx, bytesDecoded.data() + nOutLen, &nTmpLen);
    nOutLen += nTmpLen;

    bytesDecoded.resize(nOutLen);
    return bytesDecoded;
}

std::string CGmSSL::vectorToHexString(const std::vector<uint8_t>& data)
{
    std::ostringstream oss;
    for (auto byte : data)
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << (int)byte;
    }
    return oss.str();
}

//info /*----------------------- 私有函数 -----------------------*/

std::string CGmSSL::exec(const std::string &strCmd)
{
    std::array<char, 128> aBuffer;
    std::string strResult;
    std::shared_ptr<FILE> pipe(popen(strCmd.c_str(), "r"), pclose);

    if (!pipe)
    {
        dlog_error("popen() failed!");
        return "";
    }

    while (fgets(aBuffer.data(), aBuffer.size(), pipe.get()) != nullptr)
    {
        strResult += aBuffer.data();
    }
    /*去掉 strResult 中的换行符*/
    strResult.erase(std::remove(strResult.begin(), strResult.end(), '\n'), strResult.end());
    return strResult;
}

std::string CGmSSL::create_tempfile(const std::string &content)
{
    std::string filename = "/tmp/gmssl_" + std::to_string(getpid()) + "_" + rand(16);
    std::ofstream tmpfile(filename);
    tmpfile << content;
    tmpfile.close();
    return filename;
}

void CGmSSL::remove_tempfile(const std::string &filename)
{
    remove(filename.c_str());
}

std::string CGmSSL::sm4_crypt(const std::string &key, const std::string &iv,
                              const std::string &data, bool encrypt, bool cbc_mode)
{
    std::string tmp_in = create_tempfile(data);
    std::string tmp_out = create_tempfile("");

    std::string mode = cbc_mode ? "-cbc" : "-ctr";
    std::string operation = encrypt ? "-encrypt" : "-decrypt";

    std::string cmd = "gmssl sm4 " + mode + " " + operation +
                      " -key " + key +
                      " -iv " + iv +
                      " -in " + tmp_in +
                      " -out " + tmp_out;

    exec(cmd);

    std::ifstream result(tmp_out);
    std::string strOutput((std::istreambuf_iterator<char>(result)),
                       std::istreambuf_iterator<char>());

    remove_tempfile(tmp_in);
    remove_tempfile(tmp_out);

    return strOutput;
}

std::string CGmSSL::readFileToString(const std::string &strFilePath)
{
    std::ifstream file(strFilePath);
    if (!file.is_open())
    {
        dlog_error("打开%s文件失败", strFilePath.c_str());
    }

    std::stringstream ssBuffer;
    ssBuffer << file.rdbuf();
    file.close();

    return ssBuffer.str();
}

std::vector<uint8_t> CGmSSL::readFileToBytes(const std::string &strFilePath)
{
    std::ifstream file(strFilePath, std::ios::binary);
    if (!file.is_open())
    {
        dlog_error("打开%s文件失败", strFilePath.c_str());
        return {}; // 返回空vector
    }

    // 移动到文件末尾，获取文件大小
    file.seekg(0, std::ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(fileSize);
    if (fileSize > 0)
    {
        file.read(reinterpret_cast<char*>(buffer.data()), fileSize);
    }
    file.close();

    return buffer;
}

void CGmSSL::writeStringToFile(const std::string &strFilePath, const std::string &strWriteData)
{
    std::ofstream file(strFilePath);
    if (!file.is_open())
    {
        dlog_error("打开%s文件写入失败", strFilePath.c_str());
        return;
    }
    file << strWriteData; 
    file.close();
}

void CGmSSL::writeBytesToFile(const std::string &strFilePath, const std::vector<uint8_t> &vecData)
{
    std::ofstream file(strFilePath, std::ios::out | std::ios::binary);
    if (!file.is_open())
    {
        dlog_error("打开%s文件写入失败", strFilePath.c_str());
        return;
    }
    file.write(reinterpret_cast<const char *>(vecData.data()), vecData.size());
    file.close();
}
