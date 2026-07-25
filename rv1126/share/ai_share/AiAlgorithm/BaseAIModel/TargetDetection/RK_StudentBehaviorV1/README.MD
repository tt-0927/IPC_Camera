# RK_StudentBehaviorV1

- 功能：基于YOLOV7的学生行为分析算法，可以识别8类(低头写字，低头看书，抬头听课，转头，举手，站立，小组讨论，教师指导)
- 依赖：opencv库、librknn_api
- 模型推理入口：include/rk_student_behavior.h -> StudentBehaviorRgb(xxx)函数
- 模型：
	- student_behavior.rknn 模型宽高(w,h)-- (640x640)
