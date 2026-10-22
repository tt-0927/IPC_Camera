# ipc_rtspserver 第三方依赖

本目录随 `share/cam_share/media/ipc_rtspserver/` 一起归属 `cam_share` 仓库，提供
RTSP 静态库源码构建所需的离线依赖。产品 `stream` 不在这里编译第三方源码，而是
使用 `ipc_platform/lib/` 中已经生成的静态归档。

```text
third_party/
├── libevent/                    # OpenIPC libevent 迁移期离线镜像
├── libevent-openipc.lock.yaml   # libevent pin、License、目标配置
└── smolrtsp/                    # OpenIPC smolrtsp 家族离线镜像
    ├── PATCHES.md
    ├── smolrtsp/
    ├── smolrtsp-libevent/
    ├── slice99/
    ├── metalang99/
    ├── datatype99/
    └── interface99/
```

固定基线：

```text
smolrtsp       = 8ae3b5115f7a77c383080b51cb7fdba1046f862f
smolrtsp-libevent = b0329b4f8e72ff03f2b674ffdbfdc8553523ddf2
libevent       = 694decef35717d8955aa34ba4d2baaaf61c9e4a9
```

smolrtsp 系列的本地修改必须登记在 `smolrtsp/PATCHES.md`；不能直接修改 vendor
源码。libevent 的正式 source-of-truth 和 Hi3516 归档构建脚本位于
`cam_opensource_library/libevent-openipc/`。

## License 与升级纪律

| 组件 | 固定版本 | License |
|---|---|---|
| smolrtsp | `8ae3b5115f7a77c383080b51cb7fdba1046f862f` | MIT |
| smolrtsp-libevent | `b0329b4f8e72ff03f2b674ffdbfdc8553523ddf2` | MIT |
| slice99 / metalang99 / datatype99 / interface99 | v0.7.8 / v1.13.5 / v1.6.5 / v1.0.2 | MIT |
| libevent | `694decef35717d8955aa34ba4d2baaaf61c9e4a9` | BSD 3-Clause |

保留各上游 License 和 copyright notice。升级必须记录新基线、源码 diff、兼容性、
内存/Flash 及弱网回归结果；不得浮动跟踪上游 `master`。
