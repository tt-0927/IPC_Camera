### 人数统计模块

#### 说明
1. 使用这个模块时在该程序的CMakeLists.txt中添加下面代码
   ```cmake
   add_subdirectory(Path/CountingAlgorithm 生成的二进制临时文件路径)
   target_link_libraries (${PROJECT_NAME} CountingAlgorithm)
   ```

#### 注意点
