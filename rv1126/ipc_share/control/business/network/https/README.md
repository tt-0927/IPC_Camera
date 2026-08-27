# CA证书相关模块

导入/导出自签CA证书

## certs说明
├── device                  # 用于存放自签名证书和导入的证书
│   └── device.key          # 自签名证书和导入的证书
│
├── request                 # 存放证书请求相关文件
│   └── request.key         # 证书请求的私钥文件
│
└── trust                   # 存放受信任的证书
    ├── middle              # 存放中间证书相关文件
    │   ├── middleCA.cer    # 中级证书文件
    │   ├── middleCA.csr    # 中级证书请求文件
    │   ├── middleCA.key    # 中级证书私钥文件
    │   └── openssl.cnf     # OpenSSL配置文件,根证书签发中间证书使用
    │
    └── root                # 存放根证书相关文件
        ├── keypasswd.txt   # 根证书私钥的密码文件
        ├── rootCA.cer      # 根证书文件
        ├── rootCA.key      # 根证书私钥文件
        └── rootCA.srl      # 根证书序列号文件
需要将ca_dir拷贝到系统对应的 "/opt/rk/security/cert/" 目录下

## 依赖库
libcrypto.so、libssl.so
