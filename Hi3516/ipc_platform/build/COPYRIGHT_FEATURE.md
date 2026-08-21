# 软著打包功能实现说明

## 概述

为打包脚本添加软著打包功能，支持生成符合软著登记要求的文件名格式。

## 功能特性

### 1. 软著文件名格式

**标准格式：**
```
AI网络摄像机安全管理软件V3.293(设备型号-芯片型号-传感器型号-焦距-版本号-语言-包类型-项目类型)
```

**示例：**
- 升级包：`AI网络摄像机安全管理软件V3.293(TV-3852T-Hi3516CV610-20S-sc533hai-f4mm-V1.04-中文-升级包-itc).bin`
- 固件包：`AI网络摄像机安全管理软件V3.293(TV-3852T-Hi3516CV610-20S-sc533hai-f4mm-V1.04-中文-固件包-itc).zip`

### 2. 版本号转换

自动将标准版本号转换为软著格式：
- `V1.0.4` → `V1.04`（移除第二个点号）
- `V1.1.5` → `V1.15`
- `V1.0.7` → `V1.07`

转换规则：`sed 's/\(V[0-9]*\.[0-9]*\)\.\([0-9]*\)/\1\2/'`

### 3. 设备焦距映射表

根据设备型号自动展开支持的焦距列表：

| 设备型号 | 传感器 | 支持焦距 |
|---------|--------|---------|
| TV-3852T | sc533hai | f2_8mm, f4mm, f6mm, f8mm |
| TV-3852H | sc533hai | f2_8mm, f4mm |
| TV-3852HZT | sc533hai | f2_8mm, f4mm |
| TV-3852TL | sc533hai | f4mm |
| TV-3852HL | sc533hai | f4mm |
| TV-3852TL4G | sc533hai | f4mm |
| TV-3852TLW | sc533hai | f4mm |

## 使用方法

### 基本用法

**升级包软著打包：**
```bash
./build.sh -d TV-3852T -s sc533hai-f4mm -p --copyright
```

**固件包软著打包：**
```bash
./build.sh -d TV-3852T -s sc533hai-f4mm -i --copyright
```

### 组合使用

**软著 + 全焦距：**
```bash
./build.sh -d TV-3852T -s sc533hai-f4mm -p --copyright --all-focal
# 生成 4 个升级包（f2_8mm, f4mm, f6mm, f8mm）
```

**软著 + 全焦距 + 全项目类型：**
```bash
./build.sh -d TV-3852T -s sc533hai-f4mm -i --copyright --all-focal --all-project
# 生成 8 个固件包（4 焦距 × 2 项目类型）
```

### 参数说明

- `--copyright`: 启用软著打包模式，文件名用软著名称包裹，版本号转换为软著格式
- `--all-focal`: 打包当前设备型号支持的所有焦距
- `--all-project`: 打包所有项目类型（itc, 中性）
- `--daily`: 追加日期序号后缀（与 `--copyright` 同时使用时，软著模式优先级更高，不追加日期）

## 实现细节

### 修改的文件

1. **path.conf**
   - 添加软著前缀常量：`COPYRIGHT_PREFIX="AI网络摄像机安全管理软件V3.293"`
   - 添加版本号转换函数：`convert_to_copyright_version()`
   - 修改版本号解析函数：`resolve_version()` 支持软著模式参数
   - 添加设备焦距映射表：`DEVICE_FOCAL_LENGTHS`
   - 添加设备传感器映射表：`DEVICE_SENSOR_MODEL`
   - 添加焦距查询函数：`get_device_focal_lengths()`
   - 添加传感器查询函数：`get_device_sensor_model()`

2. **build.sh**
   - 添加 `--copyright` 选项解析
   - 添加 `COPYRIGHT_MODE` 变量
   - 修改 `resolve_version()` 调用，传递软著模式参数
   - 修改 `run_make_packet()` 和 `run_make_image()`，向子脚本传递 `--copyright`
   - 修改 `record_packet_outputs()` 和 `record_image_outputs()`，在软著模式下包裹文件名
   - 重构 `prepare_package_matrix()`，使用设备焦距映射表替代硬编码

3. **help.conf**
   - 添加 `--copyright` 帮助文本
   - 更新 `--all-focal` 帮助文本
   - 添加软著模式使用示例

4. **pack/make_packet.sh**
   - 添加 `--copyright` 选项解析
   - 添加 `COPYRIGHT_MODE` 变量
   - 修改 `generate_software_name()`，在软著模式下包裹文件名

5. **image/make_image.sh**
   - 添加 `--copyright` 选项解析
   - 添加 `COPYRIGHT_MODE` 变量
   - 修改 `SOFTWARE_NAME` 赋值，在软著模式下包裹文件名

### 数据流

```
build.sh --copyright
  ↓
COPYRIGHT_MODE=true
  ↓
resolve_version(daily, copyright=true)
  ↓
VERSION_NUM="V1.04"（转换后，无日期）
  ↓
run_make_packet(... --copyright)
  ↓
make_packet.sh --copyright
  ↓
generate_software_name()
  ↓
SOFTWARE_NAME 被 COPYRIGHT_PREFIX 包裹
  ↓
run_make_image(... --copyright)
  ↓
make_image.sh --copyright
  ↓
SOFTWARE_NAME 被 COPYRIGHT_PREFIX 包裹
  ↓
record_*_outputs() 查找输出文件时同步包裹
```

### 兼容性

- **向后兼容**：不使用 `--copyright` 参数时，行为与原有版本完全一致
- **Bash 版本要求**：需要 Bash 4+（支持 `declare -A` 关联数组）
- **依赖**：需要 `sed`、`getopt` 等标准工具

## 测试建议

### 1. 基础功能测试

```bash
# 测试软著升级包
./build.sh -d TV-3852T -s sc533hai-f4mm -p --copyright
# 预期：生成 AI网络摄像机安全管理软件V3.293(TV-3852T-Hi3516CV610-20S-sc533hai-f4mm-V1.04-中文-升级包-itc).bin

# 测试软著固件包
./build.sh -d TV-3852T -s sc533hai-f4mm -i --copyright
# 预期：生成 AI网络摄像机安全管理软件V3.293(TV-3852T-Hi3516CV610-20S-sc533hai-f4mm-V1.04-中文-固件包-itc).zip
```

### 2. 组合功能测试

```bash
# 测试软著 + 全焦距（TV-3852T 支持 4 个焦距）
./build.sh -d TV-3852T -s sc533hai-f4mm -p --copyright --all-focal
# 预期：生成 4 个升级包

# 测试软著 + 全焦距 + 全项目类型
./build.sh -d TV-3852H -s sc533hai-f4mm -i --copyright --all-focal --all-project
# 预期：生成 4 个固件包（2 焦距 × 2 项目类型）
```

### 3. 兼容性测试

```bash
# 测试不使用 --copyright 参数（向后兼容）
./build.sh -d TV-3852T -s sc533hai-f4mm -p
# 预期：生成 TV-3852T-Hi3516CV610-20S-sc533hai-f4mm-V1.0.4-中文-升级包-itc.bin

# 测试 --daily 参数
./build.sh -d TV-3852T -s sc533hai-f4mm -p --daily
# 预期：生成带日期后缀的版本号

# 测试 --copyright + --daily（软著模式优先）
./build.sh -d TV-3852T -s sc533hai-f4mm -p --copyright --daily
# 预期：生成软著格式文件名，不追加日期后缀
```

### 4. 错误处理测试

```bash
# 测试传感器型号与设备不匹配
./build.sh -d TV-3852T -s sc500ai-f4mm -p --copyright --all-focal
# 预期：报错提示传感器型号与设备不匹配

# 测试未配置焦距映射的设备
./build.sh -d TV-3852X -s sc533hai-f4mm -p --copyright --all-focal
# 预期：报错提示设备未配置焦距映射表
```

## 注意事项

1. **软著前缀**：当前所有型号统一使用 `AI网络摄像机安全管理软件V3.293`，如需按型号区分，需修改 `COPYRIGHT_PREFIX` 常量
2. **版本号来源**：基础版本号从 `share_define.h` 自动提取，转换仅影响文件名，不修改源文件
3. **Bash 版本**：确保构建环境使用 Bash 4+（Linux 环境通常满足）
4. **Windows 环境**：当前为编辑环境，不执行编译测试

## 后续优化建议

1. **软著前缀配置化**：可考虑从配置文件读取，支持按设备型号配置不同的软著名称
2. **版本号转换规则**：当前规则固定，如需更复杂的转换（如 V2.0.0 → V2.0），可优化正则表达式
3. **映射表扩展**：如需支持其他传感器型号（如 sc500ai），可添加更多映射条目
