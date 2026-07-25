# RK_StudentBehaviorV2

- 功能：基于YOLOV5的学生行为分析算法，可以识别4类(低头、抬头、转头、举手)
- 依赖：opencv库、librknn_api
- 模型推理入口：include/rk_student_behavior.h -> StudentBehaviorRgb(xxx)函数
- 模型：
  - StudentBehaviorV2.rknn 模型宽高(w,h)-- (640x640)

