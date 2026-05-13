# RK_PlayPhone

- 功能：船只行驶状态跟踪（使用可以参考demo/main.cpp中的AI()、*videoFrameHandlThr(xx)这两个函数）
- 依赖：opencv库、librknn_api
- 模型推理入口：include/rk_ship_detect.h -> DetectShipRgb()函数
- 模型：
	- PhoneRelu.rknn 模型宽高(w,h) -- (640x640)