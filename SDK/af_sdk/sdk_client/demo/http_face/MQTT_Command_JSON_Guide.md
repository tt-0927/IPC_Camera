# MQTT 命令 JSON 对接说明

本文档说明平台通过 MQTT 下发命令时的 Topic、JSON 外层格式、字段含义，以及人脸库、目标库、垃圾识别配置相关命令示例。

## 1. Topic 约定

平台向设备下发通用命令：

```text
device/{设备SN}/command
```

设备向平台返回命令响应：

```text
device/{设备SN}/response
```

示例：设备 SN 为 `2` 时：

```text
下发：device/2/command
响应：device/2/response
```

建议 QoS：

| 方向 | Topic | QoS |
|---|---|---|
| 平台 -> 设备 | `device/{设备SN}/command` | 1 |
| 设备 -> 平台 | `device/{设备SN}/response` | 1 |

## 2. 通用请求格式

所有命令统一使用如下外层 JSON：

```json
{
  "Command": "NET_TV_ADD_TARGET_LIB",
  "RequestId": "req-20260525-0001",
  "Data": {}
}
```

字段说明：

| 字段 | 类型 | 必填 | 说明 |
|---|---:|---:|---|
| `Command` | string | 是 | 命令名，必须使用本文档列出的命令字符串 |
| `RequestId` | string | 建议填写 | 平台生成的请求 ID；设备响应会原样带回，便于平台匹配请求和响应 |
| `Data` | object | 否 | 命令参数对象；无参数 GET 命令可填 `{}` |

## 3. 通用响应格式

设备处理完成后发布到 `device/{设备SN}/response`：

```json
{
  "Command": "NET_TV_ADD_TARGET_LIB",
  "RequestId": "req-20260525-0001",
  "Return": 0,
  "Data": {}
}
```

字段说明：

| 字段 | 类型 | 说明 |
|---|---:|---|
| `Command` | string | 对应请求里的命令名 |
| `RequestId` | string | 对应请求里的请求 ID |
| `Return` | int | 返回码，`0` 表示成功，非 `0` 表示失败 |
| `Data` | object | GET 命令返回配置/列表；SET/ADD/DEL 成功时通常为 `{}` |

## 4. 公共业务字段

### 4.1 人脸字段

| 字段 | 类型 | 说明 |
|---|---:|---|
| `Id` | int | 人脸 ID；修改/删除时使用 |
| `LibId` | string | 目标库 ID/名称；当前设备侧字段名为 `LibId` |
| `Name` | string | 人员名称 |
| `PhoneNum` | string | 联系方式或证件号字段，当前设备侧按字符串保存 |
| `PicPath` | string | 设备本地图片完整路径或文件名；当前不支持平台直接上传图片文件 |
| `BinPath` | string | 图片二进制/特征文件路径，可选 |
| `PicType` | string | 图片类型，如 `jpg` |
| `PicSize` | int | 图片大小，单位字节 |
| `PicDate` | string | 图片日期/创建时间 |
| `ModelState` | int | 模型状态，通常 `0` 未处理，`1` 成功，`-1` 失败 |
| `RatingLevel` | int | 质量评级，通常 `0` 全部/默认，`1` 未知，`2` 低，`3` 高 |

注意：如平台只有图片 URL，需要设备侧先扩展“下载 URL 图片并保存本地”的逻辑，再把本地路径写入 `PicPath`。当前直接把 URL 填到 `PicPath` 不保证成功。

### 4.2 目标库字段

| 字段 | 类型 | 说明 |
|---|---:|---|
| `LibId` | string | 目标库 ID/名称 |
| `TotalFace` | int | 目标库总人数；添加/修改时可填 `0` |
| `NormalNum` | int | 正常人脸数量；添加/修改时可填 `0` |
| `AbnormalNum` | int | 异常人脸数量；添加/修改时可填 `0` |

### 4.3 区域字段

垃圾识别配置中的 `Region` 使用多边形区域：

```json
{
  "PointNum": 4,
  "Points": [
    { "X": 0.1, "Y": 0.1 },
    { "X": 0.9, "Y": 0.1 },
    { "X": 0.9, "Y": 0.9 },
    { "X": 0.1, "Y": 0.9 }
  ]
}
```

字段说明：

| 字段 | 类型 | 说明 |
|---|---:|---|
| `PointNum` | int | 点数量，建议 3 到 32 |
| `Points` | array | 点数组 |
| `Points[].X` | float | 横坐标，归一化坐标，建议范围 `0.0` 到 `1.0` |
| `Points[].Y` | float | 纵坐标，归一化坐标，建议范围 `0.0` 到 `1.0` |

### 4.4 布防时间字段

布防时间按一周 7 天拆成 `AlarmTime1` 到 `AlarmTime7`：

```json
{
  "AlarmTime1": [
    {
      "StartTime": { "Hour": 0, "Min": 0, "Sec": 0, "MSec": 0 },
      "StopTime": { "Hour": 24, "Min": 0, "Sec": 0, "MSec": 0 }
    }
  ]
}
```

字段说明：

| 字段 | 类型 | 说明 |
|---|---:|---|
| `AlarmTime1` | array | 第 1 天布防时间段 |
| `AlarmTime2` | array | 第 2 天布防时间段 |
| `AlarmTime3` | array | 第 3 天布防时间段 |
| `AlarmTime4` | array | 第 4 天布防时间段 |
| `AlarmTime5` | array | 第 5 天布防时间段 |
| `AlarmTime6` | array | 第 6 天布防时间段 |
| `AlarmTime7` | array | 第 7 天布防时间段 |
| `StartTime.Hour` | int | 开始小时 |
| `StartTime.Min` | int | 开始分钟 |
| `StartTime.Sec` | int | 开始秒 |
| `StartTime.MSec` | int | 开始毫秒 |
| `StopTime.Hour` | int | 结束小时 |
| `StopTime.Min` | int | 结束分钟 |
| `StopTime.Sec` | int | 结束秒 |
| `StopTime.MSec` | int | 结束毫秒 |

### 4.5 联动字段

垃圾识别配置中的联动字段为 `LinkageMode`：

```json
{
  "LinkageMode": {
    "Tradition": [],
    "AlarmLinkage": [],
    "RecordChn": []
  }
}
```

字段说明：

| 字段 | 类型 | 说明 |
|---|---:|---|
| `Tradition` | int array | 常规联动类型列表 |
| `AlarmLinkage` | int array | 报警输出联动列表 |
| `RecordChn` | int array | 录像通道联动列表 |

## 5. 人脸命令

### 5.1 添加人脸

命令：`NET_TV_ADD_FACE_INFO`

```json
{
  "Command": "NET_TV_ADD_FACE_INFO",
  "RequestId": "face-add-001",
  "Data": {
    "Id": 0,
    "LibId": "test_lib_001",
    "Name": "张三",
    "PhoneNum": "13800000000",
    "PicPath": "/userdata/face/zhangsan.jpg",
    "BinPath": "",
    "PicType": "jpg",
    "PicSize": 123456,
    "PicDate": "2026-05-25 10:00:00",
    "ModelState": 0,
    "RatingLevel": 0
  }
}
```

### 5.2 删除人脸

命令：`NET_TV_DEL_FACE_INFO`

```json
{
  "Command": "NET_TV_DEL_FACE_INFO",
  "RequestId": "face-del-001",
  "Data": {
    "Ids": [1001, 1002]
  }
}
```

### 5.3 修改人脸

命令：`NET_TV_SET_FACE_INFO`

```json
{
  "Command": "NET_TV_SET_FACE_INFO",
  "RequestId": "face-set-001",
  "Data": {
    "Id": 1001,
    "LibId": "test_lib_001",
    "Name": "张三-修改",
    "PhoneNum": "13900000000",
    "PicPath": "/userdata/face/zhangsan_new.jpg",
    "BinPath": "",
    "PicType": "jpg",
    "PicSize": 123456,
    "PicDate": "2026-05-25 11:00:00",
    "ModelState": 0,
    "RatingLevel": 0
  }
}
```

### 5.4 获取人脸

命令：`NET_TV_GET_FACE_INFO`

```json
{
  "Command": "NET_TV_GET_FACE_INFO",
  "RequestId": "face-get-001",
  "Data": {
    "LibId": "test_lib_001",
    "Name": "",
    "PhoneNum": "",
    "ModelState": 0,
    "RatingLevel": 0
  }
}
```

响应 `Data` 示例：

```json
{
  "FaceInfos": [
    {
      "Id": 1001,
      "LibId": "test_lib_001",
      "Name": "张三",
      "PhoneNum": "13800000000",
      "PicPath": "/userdata/face/zhangsan.jpg",
      "PicType": "jpg",
      "PicSize": 123456,
      "PicDate": "2026-05-25 10:00:00",
      "ModelState": 1,
      "RatingLevel": 3,
      "BinPath": ""
    }
  ]
}
```

## 6. 目标库命令

### 6.1 添加目标库

命令：`NET_TV_ADD_TARGET_LIB`

```json
{
  "Command": "NET_TV_ADD_TARGET_LIB",
  "RequestId": "lib-add-001",
  "Data": {
    "LibId": "test_lib_001",
    "TotalFace": 0,
    "NormalNum": 0,
    "AbnormalNum": 0
  }
}
```

### 6.2 删除目标库

命令：`NET_TV_DEL_TARGET_LIB`

```json
{
  "Command": "NET_TV_DEL_TARGET_LIB",
  "RequestId": "lib-del-001",
  "Data": {
    "LibId": "test_lib_001",
    "TotalFace": 0,
    "NormalNum": 0,
    "AbnormalNum": 0
  }
}
```

### 6.3 修改目标库

命令：`NET_TV_SET_TARGET_LIB`

```json
{
  "Command": "NET_TV_SET_TARGET_LIB",
  "RequestId": "lib-set-001",
  "Data": {
    "LibId": "test_lib_001_new",
    "TotalFace": 0,
    "NormalNum": 0,
    "AbnormalNum": 0
  }
}
```

### 6.4 获取目标库

命令：`NET_TV_GET_TARGET_LIB`

```json
{
  "Command": "NET_TV_GET_TARGET_LIB",
  "RequestId": "lib-get-001",
  "Data": {}
}
```

响应 `Data` 示例：

```json
{
  "TargetLibInfos": [
    {
      "LibId": "test_lib_001",
      "TotalFace": 10,
      "NormalNum": 9,
      "AbnormalNum": 1
    }
  ]
}
```

## 7. 垃圾暴露识别配置

### 7.1 获取垃圾暴露识别配置

命令：`NET_TV_GET_GARBAGE_EXPOSURE_CFG`

```json
{
  "Command": "NET_TV_GET_GARBAGE_EXPOSURE_CFG",
  "RequestId": "garbage-exposure-get-001",
  "Data": {}
}
```

### 7.2 设置垃圾暴露识别配置

命令：`NET_TV_SET_GARBAGE_EXPOSURE_CFG`

```json
{
  "Command": "NET_TV_SET_GARBAGE_EXPOSURE_CFG",
  "RequestId": "garbage-exposure-set-001",
  "Data": {
    "Enable": true,
    "Rule": {
      "Sensitivity": 50,
      "Region": {
        "PointNum": 4,
        "Points": [
          { "X": 0.1, "Y": 0.1 },
          { "X": 0.9, "Y": 0.1 },
          { "X": 0.9, "Y": 0.9 },
          { "X": 0.1, "Y": 0.9 }
        ]
      }
    },
    "AlarmTime1": [
      {
        "StartTime": { "Hour": 0, "Min": 0, "Sec": 0, "MSec": 0 },
        "StopTime": { "Hour": 24, "Min": 0, "Sec": 0, "MSec": 0 }
      }
    ],
    "AlarmTime2": [
      {
        "StartTime": { "Hour": 0, "Min": 0, "Sec": 0, "MSec": 0 },
        "StopTime": { "Hour": 24, "Min": 0, "Sec": 0, "MSec": 0 }
      }
    ],
    "AlarmTime3": [
      {
        "StartTime": { "Hour": 0, "Min": 0, "Sec": 0, "MSec": 0 },
        "StopTime": { "Hour": 24, "Min": 0, "Sec": 0, "MSec": 0 }
      }
    ],
    "AlarmTime4": [
      {
        "StartTime": { "Hour": 0, "Min": 0, "Sec": 0, "MSec": 0 },
        "StopTime": { "Hour": 24, "Min": 0, "Sec": 0, "MSec": 0 }
      }
    ],
    "AlarmTime5": [
      {
        "StartTime": { "Hour": 0, "Min": 0, "Sec": 0, "MSec": 0 },
        "StopTime": { "Hour": 24, "Min": 0, "Sec": 0, "MSec": 0 }
      }
    ],
    "AlarmTime6": [
      {
        "StartTime": { "Hour": 0, "Min": 0, "Sec": 0, "MSec": 0 },
        "StopTime": { "Hour": 24, "Min": 0, "Sec": 0, "MSec": 0 }
      }
    ],
    "AlarmTime7": [
      {
        "StartTime": { "Hour": 0, "Min": 0, "Sec": 0, "MSec": 0 },
        "StopTime": { "Hour": 24, "Min": 0, "Sec": 0, "MSec": 0 }
      }
    ],
    "LinkageMode": {
      "Tradition": [],
      "AlarmLinkage": [],
      "RecordChn": []
    }
  }
}
```

字段说明：

| 字段 | 类型 | 说明 |
|---|---:|---|
| `Enable` | bool | 是否启用垃圾暴露识别 |
| `Rule.Sensitivity` | int | 灵敏度，范围 `1` 到 `100` |
| `Rule.Region` | object | 检测区域，详见区域字段 |
| `AlarmTime1` - `AlarmTime7` | array | 一周布防时间 |
| `LinkageMode` | object | 联动方式 |

## 8. 垃圾满溢识别配置

### 8.1 获取垃圾满溢识别配置

命令：`NET_TV_GET_GARBAGE_OVERFLOW_CFG`

```json
{
  "Command": "NET_TV_GET_GARBAGE_OVERFLOW_CFG",
  "RequestId": "garbage-overflow-get-001",
  "Data": {}
}
```

### 8.2 设置垃圾满溢识别配置

命令：`NET_TV_SET_GARBAGE_OVERFLOW_CFG`

```json
{
  "Command": "NET_TV_SET_GARBAGE_OVERFLOW_CFG",
  "RequestId": "garbage-overflow-set-001",
  "Data": {
    "Enable": true,
    "Rule": {
      "Sensitivity": 50,
      "Region": {
        "PointNum": 4,
        "Points": [
          { "X": 0.1, "Y": 0.1 },
          { "X": 0.9, "Y": 0.1 },
          { "X": 0.9, "Y": 0.9 },
          { "X": 0.1, "Y": 0.9 }
        ]
      }
    },
    "AlarmTime1": [
      {
        "StartTime": { "Hour": 0, "Min": 0, "Sec": 0, "MSec": 0 },
        "StopTime": { "Hour": 24, "Min": 0, "Sec": 0, "MSec": 0 }
      }
    ],
    "AlarmTime2": [
      {
        "StartTime": { "Hour": 0, "Min": 0, "Sec": 0, "MSec": 0 },
        "StopTime": { "Hour": 24, "Min": 0, "Sec": 0, "MSec": 0 }
      }
    ],
    "AlarmTime3": [
      {
        "StartTime": { "Hour": 0, "Min": 0, "Sec": 0, "MSec": 0 },
        "StopTime": { "Hour": 24, "Min": 0, "Sec": 0, "MSec": 0 }
      }
    ],
    "AlarmTime4": [
      {
        "StartTime": { "Hour": 0, "Min": 0, "Sec": 0, "MSec": 0 },
        "StopTime": { "Hour": 24, "Min": 0, "Sec": 0, "MSec": 0 }
      }
    ],
    "AlarmTime5": [
      {
        "StartTime": { "Hour": 0, "Min": 0, "Sec": 0, "MSec": 0 },
        "StopTime": { "Hour": 24, "Min": 0, "Sec": 0, "MSec": 0 }
      }
    ],
    "AlarmTime6": [
      {
        "StartTime": { "Hour": 0, "Min": 0, "Sec": 0, "MSec": 0 },
        "StopTime": { "Hour": 24, "Min": 0, "Sec": 0, "MSec": 0 }
      }
    ],
    "AlarmTime7": [
      {
        "StartTime": { "Hour": 0, "Min": 0, "Sec": 0, "MSec": 0 },
        "StopTime": { "Hour": 24, "Min": 0, "Sec": 0, "MSec": 0 }
      }
    ],
    "LinkageMode": {
      "Tradition": [],
      "AlarmLinkage": [],
      "RecordChn": []
    }
  }
}
```

字段说明：

| 字段 | 类型 | 说明 |
|---|---:|---|
| `Enable` | bool | 是否启用垃圾满溢识别 |
| `Rule.Sensitivity` | int | 灵敏度，范围 `1` 到 `100` |
| `Rule.Region` | object | 检测区域，详见区域字段 |
| `AlarmTime1` - `AlarmTime7` | array | 一周布防时间 |
| `LinkageMode` | object | 联动方式 |

## 9. 支持命令汇总

| 功能 | Command |
|---|---|
| 添加人脸 | `NET_TV_ADD_FACE_INFO` |
| 删除人脸 | `NET_TV_DEL_FACE_INFO` |
| 修改人脸 | `NET_TV_SET_FACE_INFO` |
| 获取人脸 | `NET_TV_GET_FACE_INFO` |
| 添加目标库 | `NET_TV_ADD_TARGET_LIB` |
| 删除目标库 | `NET_TV_DEL_TARGET_LIB` |
| 修改目标库 | `NET_TV_SET_TARGET_LIB` |
| 获取目标库 | `NET_TV_GET_TARGET_LIB` |
| 获取垃圾暴露识别配置 | `NET_TV_GET_GARBAGE_EXPOSURE_CFG` |
| 设置垃圾暴露识别配置 | `NET_TV_SET_GARBAGE_EXPOSURE_CFG` |
| 获取垃圾满溢识别配置 | `NET_TV_GET_GARBAGE_OVERFLOW_CFG` |
| 设置垃圾满溢识别配置 | `NET_TV_SET_GARBAGE_OVERFLOW_CFG` |

