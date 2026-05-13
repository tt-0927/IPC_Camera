### 航道分析模块

#### 模块结构
1. ChannelAlgorithmV1: 航道分析算法V1

#### 使用说明
1. 使用这个模块时在该程序的CMakeLists.txt中添加下面代码
   ```
   add_subdirectory(Path/ChannelAlgorithm 生成的二进制临时文件路径)
   target_link_libraries (${PROJECT_NAME} ChannelAlgorithm)
   ```

#### 注意点
