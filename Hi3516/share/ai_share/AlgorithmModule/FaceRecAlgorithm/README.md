### 人脸识别模块

#### 模块结构
1. FaceDataDB: 人脸特征数据库类
2. FaceRecAlgorithmV1: 人脸识别算法v1

#### 使用说明
1. 使用这个模块时在该程序的CMakeLists.txt中添加下面代码
   ```
   add_subdirectory(Path/FaceRecAlgorithm 生成的二进制临时文件路径)
   target_link_libraries (${PROJECT_NAME} FaceRecAlgorithm)
   ```

#### 注意点
