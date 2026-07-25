# RK_HumanCount

- 功能：基于深度学习的人数统计（统计每一帧图片的人数）
- 依赖：opencv库、librknn_api
- 模型推理入口：include/rk_human_count_detect.h -> DetectHumanBgr(xxx)函数
- 模型：
	- HR_human768x640_FDST.rknn 模型宽高(w,h)-- (768x640)
	- HR_human768x640_NWPU.rknn 模型宽高(w,h)-- (768x640)**(*默认使用)**
	- HR_human768x640_QNRF.rknn 模型宽高(w,h)-- (768x640)
	- human768x640.rknn 模型宽高(w,h)-- (768x640)
