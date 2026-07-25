# RK_BridgeFracture

- 桥路面断裂检测（目前是一个测试版本-只在样版上有效果）（使用可以参考demo/main.cpp中的AI()、*videoFrameHandlThr(xx)这两个函数）
- 依赖：opencv库、librknn_api
- 模型推理入口：include/rk_bridgefracture_detect.h -> DetectRkBridgeRgb(xxx)函数
- 模型：
	- FireDetect.rknn 模型宽高(w,h) -- (640x640)
