
### 船只检测模块

#### 说明
1. 使用这个模块时在该程序的CMakeLists.txt中添加下面代码
   ```cmake
   add_subdirectory(Path/ShipAnalyzer 生成的二进制临时文件路径)
   target_link_libraries (${PROJECT_NAME} ShipAnalyzer)
   ```

#### 注意点
