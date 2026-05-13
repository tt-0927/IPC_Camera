/*** 
 * @FilePath     : email_example.cpp
 * @Author       : tianl (tianl@kfb.cn)
 * @Date         : 2024-10-15 17:37:08
 * @LastEditors  : huangjunda
 * @LastEditTime : 2025-04-10 17:43:26
 * @Description  : 邮件测试用例
 */

#include "email_manage.h"

/* 测试图片附件路径 */
#define PIC_ATTACHMENT "/sdcard/test.png"

int main(int argc, char *argv[])
{
    Network::EmailInfo_S stEmInfo;
    CEmailManage::instance()->init();
    CEmailManage::instance()->GetEmailInfo(stEmInfo);
    CEmailManage::instance()->SendTestEmail();

    // // 创建一个 EmailInfo_S 实例并设置值
    // Network::EmailInfo_S emailInfo;

    // // 发件人信息
    // emailInfo.stSender.strName = "ITC test Email";
    // emailInfo.stSender.strAddress = "17777071630@163.com";
    // // smtp服务器
    // emailInfo.stServer.strAddress = "smtp.163.com";
    // // 服务器端口
    // emailInfo.stServer.nPort = 25;
    
    // // 开启ssl/tls加密
    // emailInfo.bTlsEnable = true;

    // // 图片附件传输
    // emailInfo.bImageAttachment = true;
    // // 图片上传间隔
    // emailInfo.nCaptureTimeInterval = 10;
    // // 服务器验证开启
    // emailInfo.bEnServerAuthentication = true;

    // // 添加一个空的 EmailUser_S 对象到收件人列表
    // emailInfo.stRecipient.emplace_back();

    // // 收件人信息
    // emailInfo.stRecipient[0].strName = "crazyturkeyda";
    // emailInfo.stRecipient[0].strAddress = "crazyturkeyda@foxmail.com";

    // // emailInfo.strPictrueFile = PIC_ATTACHMENT;
    
    // // 邮箱账号
    // emailInfo.strUserName = "17777071630@163.com";
    // // 邮箱授权密码
    // emailInfo.strPassword = "RPaXbcncbfzsLa6A";

    // CEmailManage::instance()->SendTestEmail(emailInfo.stRecipient[0]);

    // // 开始每隔10秒发送一次邮件
    
    // CEmailManage::instance()->StartSending(10);

    // // 等待一段时间后，停止发送邮件
    // std::this_thread::sleep_for(std::chrono::seconds(60));

    // // 停止发送邮件
    // CEmailManage::instance()->StopSending();

    return 0;
}
