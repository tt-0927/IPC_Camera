# RK_FireDetect

- 功能：白烟和火焰检测（使用可以参考demo/main.cpp中的AI()、*videoFrameHandlThr(xx)这两个函数）
- 依赖：opencv库、librknn_api
- 模型推理入口：include/rk_fire_detect.h -> DetectFireRgb(xxx)函数
- 模型：
	- FireDetect.rknn 模型宽高(w,h) -- (640x640)
