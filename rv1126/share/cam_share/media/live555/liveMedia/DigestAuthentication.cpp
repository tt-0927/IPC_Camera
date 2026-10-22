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
// Implementation

#include "DigestAuthentication.hh"
#include "ourMD5.hh"
#include "ourSHA256.hh"
#include <strDup.hh>
#include <GroupsockHelper.hh> // for gettimeofday()
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// info ------------------ zhouzr ------------------
Authenticator::Authenticator(AuthAlgorithm algorithm) {
  assign(NULL, NULL, NULL, NULL, False, algorithm);
}

Authenticator::Authenticator(char const* username, char const* password, Boolean passwordIsMD5, AuthAlgorithm algorithm) {
  assign(NULL, NULL, username, password, passwordIsMD5, algorithm);
}

Authenticator::Authenticator(const Authenticator& orig) {
  assign(orig.realm(), orig.nonce(), orig.username(), orig.password(), orig.fPasswordIsMD5, orig.fAuthAlgorithm);
}

Authenticator& Authenticator::operator=(const Authenticator& rightSide) {
  if (&rightSide != this) {
    reset();
    assign(rightSide.realm(), rightSide.nonce(),
	   rightSide.username(), rightSide.password(), rightSide.fPasswordIsMD5, rightSide.fAuthAlgorithm);
  }

  return *this;
}
// info ------------------ zhouzr ------------------

Boolean Authenticator::operator<(const Authenticator* rightSide) {
  // Returns True if "rightSide" is 'newer' than us:
  if (rightSide != NULL && rightSide != this &&
      (rightSide->realm() != NULL || rightSide->nonce() != NULL ||
       username() == NULL || password() == NULL ||
       strcmp(rightSide->username(), username()) != 0 ||
       strcmp(rightSide->password(), password()) != 0)) {
    return True;
  }

  return False;
}

Authenticator::~Authenticator() {
  reset();
}

void Authenticator::reset() {
  resetRealmAndNonce();
  resetUsernameAndPassword();
}

void Authenticator::setRealmAndNonce(char const* realm, char const* nonce) {
  resetRealmAndNonce();
  assignRealmAndNonce(realm, nonce);
}

void Authenticator::setRealmAndRandomNonce(char const* realm) {
  resetRealmAndNonce();

  // Construct data to seed the random nonce:
  struct {
    struct timeval timestamp;
    unsigned counter;
  } seedData;
  gettimeofday(&seedData.timestamp, NULL);
  static unsigned counter = 0;
  seedData.counter = ++counter;

  // Use MD5 to compute a 'random' nonce from this seed data:
  char nonceBuf[33];
  our_MD5Data((unsigned char*)(&seedData), sizeof seedData, nonceBuf);

  assignRealmAndNonce(realm, nonceBuf);
}

void Authenticator::setUsernameAndPassword(char const* username,
					   char const* password,
					   Boolean passwordIsMD5) {
  resetUsernameAndPassword();
  assignUsernameAndPassword(username, password, passwordIsMD5);
}

// info ------------------ zhouzr ------------------
char const *Authenticator::computeDigestResponse(char const *cmd, char const *url) const
{
#if OLD
  // The "response" field is computed as:
  //    md5(md5(<username>:<realm>:<password>):<nonce>:md5(<cmd>:<url>))
  // or, if "fPasswordIsMD5" is True:
  //    md5(<password>:<nonce>:md5(<cmd>:<url>))
  char ha1Buf[33];
  if (fPasswordIsMD5) {
    strncpy(ha1Buf, password(), 32);
    ha1Buf[32] = '\0'; // just in case
  } else {
    unsigned const ha1DataLen = strlen(username()) + 1
      + strlen(realm()) + 1 + strlen(password());
    unsigned char* ha1Data = new unsigned char[ha1DataLen+1];
    sprintf((char*)ha1Data, "%s:%s:%s", username(), realm(), password());
    our_MD5Data(ha1Data, ha1DataLen, ha1Buf);
    delete[] ha1Data;
  }

  unsigned const ha2DataLen = strlen(cmd) + 1 + strlen(url);
  unsigned char* ha2Data = new unsigned char[ha2DataLen+1];
  sprintf((char*)ha2Data, "%s:%s", cmd, url);
  char ha2Buf[33];
  our_MD5Data(ha2Data, ha2DataLen, ha2Buf);
  delete[] ha2Data;

  unsigned const digestDataLen
    = 32 + 1 + strlen(nonce()) + 1 + 32;
  unsigned char* digestData = new unsigned char[digestDataLen+1];
  sprintf((char*)digestData, "%s:%s:%s",
          ha1Buf, nonce(), ha2Buf);
  char const* result = our_MD5Data(digestData, digestDataLen, NULL);
  delete[] digestData;
  return result;
#else
  char ha1Buf[65]; // 缓冲区大小调整为64字节（SHA-256）+1
  char ha2Buf[65];

  //=== 1. 计算 HA1（根据密码存储类型选择哈希算法） ===
  if (fPasswordIsMD5)
  {
    strncpy(ha1Buf, password(), 32);
    ha1Buf[32] = '\0';
  }
  else
  { // 明文密码 -> 根据认证算法选择哈希方式
    unsigned const ha1DataLen = strlen(username()) + 1 + strlen(realm()) + 1 + strlen(password());
    unsigned char *ha1Data = new unsigned char[ha1DataLen + 1];
    sprintf((char *) ha1Data, "%s:%s:%s", username(), realm(), password());

    // 根据认证类型选择哈希算法（如Digest-MD5或Digest-SHA256）
    if (fAuthAlgorithm == ALGO_SHA256)
    {
        our_SHA256Data(ha1Data, ha1DataLen, ha1Buf);
    }
    else
    {
        our_MD5Data(ha1Data, ha1DataLen, ha1Buf);
    }
    delete[] ha1Data;
  }

  //=== 2. 计算 HA2（根据认证算法选择哈希方式） ===
  unsigned const ha2DataLen = strlen(cmd) + 1 + strlen(url);
  unsigned char *ha2Data = new unsigned char[ha2DataLen + 1];
  sprintf((char *) ha2Data, "%s:%s", cmd, url);

  if (fAuthAlgorithm == ALGO_SHA256)
  {
    our_SHA256Data(ha2Data, ha2DataLen, ha2Buf);
  }
  else
  {
    our_MD5Data(ha2Data, ha2DataLen, ha2Buf);
  }
  delete[] ha2Data;

  //=== 3. 计算最终响应值 ===
  // response = HASH(HA1:nonce:HA2)
  // 必须使用 strlen 来获取 sprintf 构建的字符串的精确长度，而不是预先计算
  size_t const digestDataMaxLen = (fAuthAlgorithm == ALGO_SHA256 ? 64 : 32) + 1 + strlen(nonce()) + 1 + (fAuthAlgorithm == ALGO_SHA256 ? 64 : 32) + 1;
  unsigned char *digestData = new unsigned char[digestDataMaxLen];
  sprintf((char *) digestData, "%s:%s:%s", ha1Buf, nonce(), ha2Buf);

  size_t const digestDataLen = strlen((char*)digestData); // 使用 strlen 获取实际长度

  char *result = new char[fAuthAlgorithm == ALGO_SHA256 ? 65 : 33];
  if (fAuthAlgorithm == ALGO_SHA256)
  {
    our_SHA256Data(digestData, digestDataLen, result);
  }
  else // 默认MD5
  {
    our_MD5Data(digestData, digestDataLen, result);
  }
  delete[] digestData;
  return result;
#endif
}

void Authenticator::setAuthAlgorithm(AuthAlgorithm algorithm)
{
    fAuthAlgorithm = algorithm;
}

Authenticator::AuthAlgorithm Authenticator::getAuthAlgorithm() const
{
    return fAuthAlgorithm;
}

const char *Authenticator::getAlgorithmString() const
{
    switch (fAuthAlgorithm)
    {
    case ALGO_SHA256:
        return "SHA-256";
    case ALGO_MD5:
    default:
        return "MD5"; // 默认MD5
    }
}
// info ------------------ zhouzr ------------------

void Authenticator::reclaimDigestResponse(char const* responseStr) const {
  delete[](char*)responseStr;
}

void Authenticator::resetRealmAndNonce() {
  delete[] fRealm; fRealm = NULL;
  delete[] fNonce; fNonce = NULL;
}

void Authenticator::resetUsernameAndPassword() {
  delete[] fUsername; fUsername = NULL;
  delete[] fPassword; fPassword = NULL;
  fPasswordIsMD5 = False;
}

void Authenticator::assignRealmAndNonce(char const* realm, char const* nonce) {
  fRealm = strDup(realm);
  fNonce = strDup(nonce);
}

void Authenticator::assignUsernameAndPassword(char const* username, char const* password, Boolean passwordIsMD5) {
  if (username == NULL) username = "";
  if (password == NULL) password = "";

  fUsername = strDup(username);
  fPassword = strDup(password);
  fPasswordIsMD5 = passwordIsMD5;
}

// info ------------------ zhouzr ------------------
void Authenticator::assign(char const* realm, char const* nonce,
			   char const* username, char const* password, Boolean passwordIsMD5, AuthAlgorithm algorithm) {
  assignRealmAndNonce(realm, nonce);
  assignUsernameAndPassword(username, password, passwordIsMD5);
  fAuthAlgorithm = algorithm;
}
// info ------------------ zhouzr ------------------

// info ------------------ zhengxh ------------------
char const* Authenticator::computeDigestMD5Response(char const* cmd, char const* url) const
{
  const int md5_len = 32;
  const int md5_size = md5_len + 1;
  char ha1Buf[md5_size];
  char ha2Buf[md5_size];

  //=== 1. 计算 HA1（根据密码存储类型选择哈希算法） ===
  if (fPasswordIsMD5)
  {
    strncpy(ha1Buf, password(), 32);
    ha1Buf[32] = '\0';
  }
  else
  { // 明文密码 -> 根据认证算法选择哈希方式
    unsigned const ha1DataLen = strlen(username()) + 1 + strlen(realm()) + 1 + strlen(password());
    unsigned char *ha1Data = new unsigned char[ha1DataLen + 1];
    sprintf((char *) ha1Data, "%s:%s:%s", username(), realm(), password());

    // 使用Digest-MD5算法
    our_MD5Data(ha1Data, ha1DataLen, ha1Buf);
    delete[] ha1Data;
  }

  //=== 2. 计算 HA2 ===
  unsigned const ha2DataLen = strlen(cmd) + 1 + strlen(url);
  unsigned char *ha2Data = new unsigned char[ha2DataLen + 1];
  sprintf((char *) ha2Data, "%s:%s", cmd, url);
  our_MD5Data(ha2Data, ha2DataLen, ha2Buf);
  delete[] ha2Data;

  //=== 3. 计算最终响应值 ===
  // response = HASH(HA1:nonce:HA2)
  // 必须使用 strlen 来获取 sprintf 构建的字符串的精确长度，而不是预先计算
  size_t const digestDataMaxLen = md5_size + strlen(nonce()) + 1 + md5_size;
  unsigned char *digestData = new unsigned char[digestDataMaxLen];
  sprintf((char *) digestData, "%s:%s:%s", ha1Buf, nonce(), ha2Buf);

  size_t const digestDataLen = strlen((char*)digestData); // 使用 strlen 获取实际长度
  char *result = new char[md5_size];
  our_MD5Data(digestData, digestDataLen, result);

  delete[] digestData;
  return result;
}

char const* Authenticator::computeDigestSHA256Response(char const* cmd, char const* url) const
{
  const int sha256_len = 64;
  const int sha256_size = sha256_len + 1;
  char ha1Buf[sha256_size]; // 缓冲区大小调整为64字节（SHA-256）+1
  char ha2Buf[sha256_size];

  //=== 1. 计算 HA1（根据密码存储类型选择哈希算法） ===
  if (fPasswordIsMD5)
  {
    strncpy(ha1Buf, password(), 32);
    ha1Buf[32] = '\0';
  }
  else
  { // 明文密码 -> 使用Digest-SHA256算法
    unsigned const ha1DataLen = strlen(username()) + 1 + strlen(realm()) + 1 + strlen(password());
    unsigned char *ha1Data = new unsigned char[ha1DataLen + 1];
    sprintf((char *) ha1Data, "%s:%s:%s", username(), realm(), password());

    our_SHA256Data(ha1Data, ha1DataLen, ha1Buf);
    delete[] ha1Data;
  }

  //=== 2. 计算 HA2 ===
  unsigned const ha2DataLen = strlen(cmd) + 1 + strlen(url);
  unsigned char *ha2Data = new unsigned char[ha2DataLen + 1];
  sprintf((char *) ha2Data, "%s:%s", cmd, url);

  our_SHA256Data(ha2Data, ha2DataLen, ha2Buf);
  delete[] ha2Data;

  //=== 3. 计算最终响应值 ===
  // response = HASH(HA1:nonce:HA2)
  // 必须使用 strlen 来获取 sprintf 构建的字符串的精确长度，而不是预先计算
  size_t const digestDataMaxLen = sha256_size + strlen(nonce()) + 1 + sha256_size;
  unsigned char *digestData = new unsigned char[digestDataMaxLen];
  sprintf((char *) digestData, "%s:%s:%s", ha1Buf, nonce(), ha2Buf);

  size_t const digestDataLen = strlen((char*)digestData); // 使用 strlen 获取实际长度
  char *result = new char[sha256_size];
  our_SHA256Data(digestData, digestDataLen, result);

  delete[] digestData;
  return result;
}
// info ------------------ zhengxh ------------------