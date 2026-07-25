# RK_HeadSort

- 功能：人头检测+竖直跟踪算法
- 依赖：opencv库、librknn_api
- 模型推理入口：
  - 调用函数
    - 人头检测
      - include/rk_head_sort_detect.h  ---> HeadSortDetectFaceRgb(xx)函数
    - 竖直跟踪算法（Sort）
      - sort/include/sort.h  ---> track_process(xx)函数
  - 详细使用，查看demo/main.cpp
- 模型：(这个是人头检测+人体特征提取)
  - HeadDetect.rknn 模型宽高(w,h)-- (640x640)
