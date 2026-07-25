# RK_Retinaface

- 功能：基于深度学习的人脸检测
- 依赖：opencv库、librknn_api
- 模型推理入口：include/rk_faces_detect.h -> DetectFaceBgr(xxx)函数
- 模型：
	- MB_FaceDetector1920x1024.rknn 模型宽高(w,h)-- (1920x1024) (*默认使用)
	- RT50_FaceDetector1920x1024.rknn 模型宽高(w,h)-- (1920x1024)
	- FaceDetector768x640.rknn 模型宽高(w,h)-- (768x640)
