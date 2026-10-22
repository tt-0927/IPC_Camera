# PATCHES

对 smolrtsp 基线 `8ae3b5115f7a77c383080b51cb7fdba1046f862f`（2026-09-03 master）的
全部本地修改必须逐条记录在此，不得静默改源码。

当前状态：基线原始源码，无任何本地修改。

## 补丁记录格式

每条补丁包含：

- 目的
- 影响文件
- 上游基线 commit
- 是否计划回 upstream
- 测试项

## 预期补丁方向（来自 03 文档 3.7，仅作提示，落地时逐条登记）

- 自定义 epoll Writer
- TCP send buffer 优化
- Digest auth
- RTCP engine
- Controller method 扩展
- 去除 Interface99/Datatype99 宏依赖
- 内存分配器适配
