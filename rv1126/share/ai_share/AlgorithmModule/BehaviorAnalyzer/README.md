### 行为分析模块

#### 模块结构
1. BehaviorAnalyzerV1: 行为分析算法V1

#### 使用说明
1. 使用这个模块时在该程序的CMakeLists.txt中添加下面代码
   ```
   add_subdirectory(Path/BehaviorAnalyzer 生成的二进制临时文件路径)
   target_link_libraries (${PROJECT_NAME} BehaviorAnalyzer)
   ```

#### 注意点
