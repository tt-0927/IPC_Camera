# RK_Retinaface

- 功能：基于深度学习的人脸检测
- 依赖：opencv库、librknn_api
- 模型推理入口：include/rk_yoloface_detect.h -> DetecFaceRgb(xxx)函数
  - 均值滤波实现马赛克：include/rk_yoloface_detect.h -> MosaicFace(xxx)函数
- 模型：
  - YoloFace.rknn 模型宽高(w,h)-- (640*640) (*默认使用)
