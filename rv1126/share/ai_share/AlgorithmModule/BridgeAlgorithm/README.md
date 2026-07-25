### 桥梁分析模块

#### 模块结构
1. BridgeCollapseAlgorithm: 桥梁坍塌分析算法
2. BridgeFractureAlgorithm: 桥梁裂缝分析算法

#### 使用说明
1. 使用这个模块时在该程序的CMakeLists.txt中添加下面代码
   ```
   add_subdirectory(Path/BridgeAlgorithm 生成的二进制临时文件路径)
   target_link_libraries (${PROJECT_NAME} BridgeAlgorithm)
   ```

#### 注意点
