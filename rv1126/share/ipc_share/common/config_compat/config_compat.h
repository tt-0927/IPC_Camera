/**
 * @FilePath     : config_compat.h
 * @Author       : zhouzr@kfb.cn
 * @Date         : 2026-01-13
 * @LastEditors  : zhouzr@kfb.cn
 * @LastEditTime : 2026-01-13 14:47:03
 * @Description  : 配置版本兼容性框架统一头文件
 */

#pragma once

/* 核心模块 */
#include "config_version.h"
#include "config_validator.h"
#include "config_migrator.h"
#include "versioned_config_storage.h"

/* 具体配置类型的兼容性实现 */
#include "video_config_compat.h"
#include "record_config_compat.h"
