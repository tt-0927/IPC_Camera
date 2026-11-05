#pragma once

/*
 * BLE 配网对外标识和长期密钥文件必须由 GATT、认证模块及 ActionCode
 * 查询接口共用，避免二维码内容与设备实际服务配置不一致。
 */
#define BLE_PROVISIONING_PROTOCOL_VERSION 1
#define BLE_PROVISIONING_SERVICE_UUID "7A1E0001-7D8B-4F31-9E45-545650524F56"
#define BLE_SETUP_SECRET_FILE "/opt/course/config/ble_setup_secret"

