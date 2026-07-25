# RK_PlayPhone

- 功能：手机口罩检测（使用可以参考demo/main.cpp中的AI()、*videoFrameHandlThr(xx)这两个函数）
- 依赖：opencv库、librknn_api
- 模型推理入口：include/rk_phone_detect.h -> DetectPhoneRgb(xxx)函数
- 模型：
	- PhoneRelu.rknn 模型宽高(w,h) -- (640x640)

