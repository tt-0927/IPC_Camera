## 语音唤醒

### 一、使用说明

1、CMakeLists.txt导入kws相关的文件

```
add subdirectory(当前路径)
# 编译CMakeLists.txt,添加kws
target_link_libraries(xx PRIVATE CpuCAInferenceModel_KWS)
```

2、第三方的依赖(onnxruntime)
- 下载地址：https://github.com/csukuangfj/onnxruntime-libs/releases/download/v1.17.1/onnxruntime-linux-aarch64-glibc2_17-Release-1.17.1.zip
	- 	将kws_source/onnxruntime 替换即可



