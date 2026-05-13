/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// "liveMedia"
// Copyright (c) 1996-2022 Live Networks, Inc.  All rights reserved.
// A class used for digest authentication.
// C++ header

#ifndef _DIGEST_AUTHENTICATION_HH
#define _DIGEST_AUTHENTICATION_HH

#ifndef _BOOLEAN_HH
#include <Boolean.hh>
#endif

// A class used for digest authentication.
// The "realm", and "nonce" fields are supplied by the server
// (in a "401 Unauthorized" response).
// The "username" and "password" fields are supplied by the client.
class Authenticator {
public:
  // info ------------------ zhouzr ------------------
  // 定义支持的哈希算法类型
  enum AuthAlgorithm
  {
    ALGO_MD5,   // 默认MD5
    ALGO_SHA256, // SHA-256
    ALGO_MD5_SHA256 // MD5/SHA-256
  };
  // info ------------------ zhouzr ------------------
  Authenticator(AuthAlgorithm algorithm = ALGO_MD5);
  Authenticator(char const* username, char const* password, Boolean passwordIsMD5 = False, AuthAlgorithm algorithm = ALGO_MD5); // 修改：增加算法参数
      // If "passwordIsMD5" is True, then "password" is actually the value computed
      // by md5(<username>:<realm>:<actual-password>)
  // info ------------------ zhouzr ------------------
  Authenticator(const Authenticator& orig);
  Authenticator& operator=(const Authenticator& rightSide);
  Boolean operator<(const Authenticator* rightSide);
  virtual ~Authenticator();

  void reset();
  void setRealmAndNonce(char const* realm, char const* nonce);
  void setRealmAndRandomNonce(char const* realm);
      // as above, except that the nonce is created randomly.
      // (This is used by servers.)
  void setUsernameAndPassword(char const* username, char const* password, Boolean passwordIsMD5 = False);
      // If "passwordIsMD5" is True, then "password" is actually the value computed
      // by md5(<username>:<realm>:<actual-password>)

  char const* realm() const { return fRealm; }
  char const* nonce() const { return fNonce; }
  char const* username() const { return fUsername; }
  char const* password() const { return fPassword; }

  /**
   * @brief   : 计算摘要认证的响应值 (response)
   * @note    : 这是认证过程的核心，根据用户名、密码、realm、nonce等信息生成客户端应答的哈希值
   * @param   {char const*} cmd：RTSP命令 (例如 "DESCRIBE", "SETUP")
   * @param   {char const*} url：请求的URL
   * @return  {char const*} 计算出的摘要响应字符串，调用者需要在使用后通过 reclaimDigestResponse 释放
   */
  char const* computeDigestResponse(char const* cmd, char const* url) const;

  /**
   * @description  : 计算MD5摘要认证的响应值
   * @param         {char const*} cmd
   * @param         {char const*} url
   * @return        {*}
   */  
  char const* computeDigestMD5Response(char const* cmd, char const* url) const;

  /**
   * @description  : 计算SHA256摘要认证的响应值
   * @param         {char const*} cmd
   * @param         {char const*} url
   * @return        {*}
   */ 
  char const* computeDigestSHA256Response(char const* cmd, char const* url) const;

      // The returned string from this function must later be freed by calling:
  void reclaimDigestResponse(char const* responseStr) const;

  // info ------------------ zhouzr ------------------
  /* 设置当前算法 */
  void setAuthAlgorithm(AuthAlgorithm algorithm);
  
  /* 获取当前算法 */
  AuthAlgorithm getAuthAlgorithm() const;

  /* 将算法枚举转换为标准字符串（如 "MD5"、"SHA-256"） */
  const char* getAlgorithmString() const;
  // info ------------------ zhouzr ------------------

private:
  void resetRealmAndNonce();
  void resetUsernameAndPassword();
  void assignRealmAndNonce(char const* realm, char const* nonce);
  void assignUsernameAndPassword(char const* username, char const* password, Boolean passwordIsMD5);
  // info ------------------ zhouzr ------------------
  void assign(char const* realm, char const* nonce,
	      char const* username, char const* password, Boolean passwordIsMD5, AuthAlgorithm algorithm);
  // info ------------------ zhouzr ------------------

private:
  char* fRealm; char* fNonce;
  char* fUsername; char* fPassword;
  Boolean fPasswordIsMD5;
  // info ------------------ zhouzr ------------------
  AuthAlgorithm fAuthAlgorithm; // 默认使用MD5
  // info ------------------ zhouzr ------------------
};

#endif
