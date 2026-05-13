# RK_KeypointsDetect

- 功能：人体关键点检测（17个）（使用可以参考demo/main.cpp中的AI()、*videoFrameHandlThr(xx)这两个函数）
- 依赖：opencv库、librknn_api
- 模型推理入口：include/rk_human_detect_kpt.h -> DetectKptRgb(xxx)函数
- 模型：
	- Yolov5Kpt.rknn 模型宽高(w,h) -- (640x640)
