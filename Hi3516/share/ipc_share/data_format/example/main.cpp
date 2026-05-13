// Hello World example
// This example shows basic usage of DOM-style API.

#include "Json.h"
#include <cstdio>
#include <iostream>

int json_create();
int json_parse();
int main()
{
    json_create();
    json_parse();
    return 0;
}

int json_create()
{
    // 创建一个 Document 对象
    Json::Document document;
    document.SetObject();

    // 设置 ActionCode
    document.AddMember("ActionCode", 1004, document.GetAllocator());

    // 设置 DeviceName
    document.AddMember("DeviceName", "IPC", document.GetAllocator());

    // 设置 UserName
    document.AddMember("UserName", "", document.GetAllocator());

    // 设置 Return
    document.AddMember("Return", 0, document.GetAllocator());

    // 创建 Data 对象
    Json::Value data(Json::kObjectType);
    data.AddMember("Total", 1, document.GetAllocator());

    // 创建 Infos 数组
    Json::Value infos(Json::kArrayType);

    // 创建 Info 对象
    Json::Value info(Json::kObjectType);
    info.AddMember("Id", 1, document.GetAllocator());
    info.AddMember("Account", "admin", document.GetAllocator());
    info.AddMember("Password", "admin@123", document.GetAllocator());
    info.AddMember("LoginCnt", 2, document.GetAllocator());
    info.AddMember("AccountStatus", 0, document.GetAllocator());
    info.AddMember("AccountType", 0, document.GetAllocator());
    info.AddMember("Name", "", document.GetAllocator());
    info.AddMember("PhoneNumber", "", document.GetAllocator());
    info.AddMember("LogoPath", "", document.GetAllocator());

    // 创建 Permissions 对象
    Json::Value permissions(Json::kObjectType);
    permissions.AddMember("LocalUpgradeFormat", 0, document.GetAllocator());
    permissions.AddMember("LocalShutdownRestart", 0, document.GetAllocator());
    permissions.AddMember("LocalSetParameters", 0, document.GetAllocator());
    permissions.AddMember("LocalViewLogs", 0, document.GetAllocator());
    permissions.AddMember("LocalChannelManagement", 0, document.GetAllocator());
    permissions.AddMember("LocalPreview", 0, document.GetAllocator());
    permissions.AddMember("LocalPlayback", 0, document.GetAllocator());
    permissions.AddMember("LocalManualOperation", 0, document.GetAllocator());
    permissions.AddMember("LocalPTZControl", 0, document.GetAllocator());
    permissions.AddMember("LocalBackup", 0, document.GetAllocator());
    permissions.AddMember("RemoteSetParameters", 0, document.GetAllocator());
    permissions.AddMember("RemoteViewLogsStatus", 0, document.GetAllocator());
    permissions.AddMember("RemoteUpgradeFormat", 0, document.GetAllocator());
    permissions.AddMember("RemoteVoiceIntercom", 0, document.GetAllocator());
    permissions.AddMember("RemoteShutdownRestart", 0, document.GetAllocator());
    permissions.AddMember("RemoteAlarmRequestOutput", 0, document.GetAllocator());
    permissions.AddMember("RemoteSerialControl", 0, document.GetAllocator());
    permissions.AddMember("RemotePreview", 0, document.GetAllocator());
    permissions.AddMember("RemoteManualRecording", 0, document.GetAllocator());
    permissions.AddMember("RemotePTZControl", 0, document.GetAllocator());
    permissions.AddMember("RemotePlayback", 0, document.GetAllocator());

    // 将 Permissions 添加到 Info 对象中
    info.AddMember("Permissions", permissions, document.GetAllocator());

    // 将 Info 对象添加到 Infos 数组中
    infos.PushBack(info, document.GetAllocator());

    // 将 Infos 数组添加到 Data 对象中
    data.AddMember("Infos", infos, document.GetAllocator());

    // 将 Data 对象添加到 Document
    document.AddMember("Data", data, document.GetAllocator());

    // 将 Document 转为字符串
    Json::StringBuffer buffer;
    Json::PrettyWriter<Json::StringBuffer> writer(buffer);
    document.Accept(writer);

    // 输出 JSON 字符串
    std::cout << buffer.GetString() << std::endl;

    return 0;
}

int json_parse()
{
    const char *json = R"({
      "ActionCode": 1004,
      "DeviceName": "IPC",
      "UserName": "",
      "Return": 0,
      "Data": {
        "Total": 1,
        "Infos": [
          {
            "Id": 1,
            "Account": "admin",
            "Password": "admin@123",
            "LoginCnt": 2,
            "AccountStatus": 0,
            "AccountType": 0,
            "Name": "",
            "PhoneNumber": "",
            "LogoPath": "",
            "Permissions": {
              "LocalUpgradeFormat": 0,
              "LocalShutdownRestart": 0,
              "LocalSetParameters": 0,
              "LocalViewLogs": 0,
              "LocalChannelManagement": 0,
              "LocalPreview": 0,
              "LocalPlayback": 0,
              "LocalManualOperation": 0,
              "LocalPTZControl": 0,
              "LocalBackup": 0,
              "RemoteSetParameters": 0,
              "RemoteViewLogsStatus": 0,
              "RemoteUpgradeFormat": 0,
              "RemoteVoiceIntercom": 0,
              "RemoteShutdownRestart": 0,
              "RemoteAlarmRequestOutput": 0,
              "RemoteSerialControl": 0,
              "RemotePreview": 0,
              "RemoteManualRecording": 0,
              "RemotePTZControl": 0,
              "RemotePlayback": 0
            }
          }
        ]
      }
    })";

    // 创建一个 Document 对象
    rapidjson::Document document;

    // 解析 JSON 字符串
    if (document.Parse(json).HasParseError())
    {
        std::cerr << "JSON parse error: " << rapidjson::GetParseErrorFunc(document.GetParseError()) << std::endl;
        return 1;
    }

    // 访问和输出数据
    int actionCode = document["ActionCode"].GetInt();
    const char *deviceName = document["DeviceName"].GetString();
    const char *userName = document["UserName"].GetString();
    int returnValue = document["Return"].GetInt();

    std::cout << "ActionCode: " << actionCode << std::endl;
    std::cout << "DeviceName: " << deviceName << std::endl;
    std::cout << "UserName: " << userName << std::endl;
    std::cout << "Return: " << returnValue << std::endl;

    // 访问 Data
    const auto &data = document["Data"];
    int total = data["Total"].GetInt();
    std::cout << "Total: " << total << std::endl;

    // 访问 Infos 数组
    const auto &infos = data["Infos"];
    for (rapidjson::SizeType i = 0; i < infos.Size(); i++)
    {
        const auto &info = infos[i];
        int id = info["Id"].GetInt();
        const char *account = info["Account"].GetString();
        const char *password = info["Password"].GetString();
        int loginCnt = info["LoginCnt"].GetInt();
        std::cout << "Info " << i << " - Id: " << id
                  << ", Account: " << account
                  << ", Password: " << password
                  << ", LoginCnt: " << loginCnt << std::endl;
    }

    return 0;
}
