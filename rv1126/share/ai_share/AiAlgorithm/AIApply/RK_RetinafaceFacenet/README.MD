# RK_RetinafaceFacenet

- 功能：人脸识别
- 依赖：opencv库、librknn_api
- 模型推理入口：FaceRecognition/include -> RetinafaceFacenetBgr(xxx)函数（提取人脸关键点），CosineSimilarity(xx)函数（人脸关键点的匹配）
- 模型：
  - 默认组合
    - MB_FaceDetector1920x1024.rknn 模型宽高(w,h)-- (1920x1024)
    - MB_facenet160x160.rknn 模型宽高(w,h)-- (160x160)
  - 其他组合1
    - RT50_FaceDetector1920x1024.rknn 模型宽高(w,h)-- (1920x1024) 
    -  MB_facenet160x160.rknn 模型宽高(w,h)-- (160x160)
