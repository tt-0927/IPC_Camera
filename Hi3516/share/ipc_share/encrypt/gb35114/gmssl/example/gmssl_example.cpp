/**
 * @FilePath     : gmssl_example.cpp
 * @Author       : zhouzirui
 * @Date         : 2025-04-21 10:57:44
 * @LastEditors  : zhouzirui
 * @LastEditTime : 2025-04-21 11:22:01
 * @Description  : 
 */

#include "gmssl.h"
#include "path_define.h"

using namespace std;

int main()
{
    
    string strBuf =  CGmSSL::instance()->rand(16);
    // cout << strBuf << endl;

    CGmSSL::instance()->sm2keygen("1234", SM_DEVICE_CA_KEY, "/opt/cam/sm_cert/Device/DevicePub.key");

    return 0;
}