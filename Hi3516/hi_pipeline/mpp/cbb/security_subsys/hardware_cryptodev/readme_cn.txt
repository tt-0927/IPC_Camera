#### Description

- 本内核模块用于对接cipher硬件能力到linux crypto内核框架，并借助开源cryptodev_linux向用户态提供统一的内核加解密接口
  使用硬件加密能力前请先编译开源框架cryptodev_linux并将模块载入内核
- 目前以内核ko的形式实现，由于依赖cipher硬件驱动，使用前应先加载 `insmod ot_cipher.ko`、 `insmod ot_km.ko`、'insmod ot_km.ko', 
  并同时加载cipher、km和otp的依赖模块，然后执行 `insmod ot_hardware_cryptodev.ko` 将本模块载入内核。

#### Compile

1.单独编译kernel：
  需先整编内核，给模块编译提供环境

  cd smp/a7_linux/source/bsp
  执行编译 make kernel

2.编译适配硬件cipher的内核模块：
    1.cd smp/a7_linux/source/mpp/cbb/security_subsys
    3.security_subsys目录下执行 make -j 可以同时生成  ot_cipher.ko insmod ot_km.ko insmod ot_km.ko 以及 ot_hardware_cryptodev.ko
    也可 cd hardware_cryptodev 目录 make -j 单独编译 ot_hardware_cryptodev.ko
    以上ko均生成在 smp/a7_linux/source/out/ko 目录下
