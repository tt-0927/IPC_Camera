# RK_SlideWindowTrack

- 功能：基于滑动窗口+特征匹配的跟踪算法
- 依赖：opencv库、librknn_api

- 模型推理入口：

  - Auxiliary/include/auxiliary_track.h -> 使用方式请看demo/main.cpp中，90、91、866（送char*帧识别）、914-915（获取人头坐标）行；模型配置等可以通过SLIDETRACKPARAM结构体进行初始化。
  - 调用说明：

    AUXILLARY_TRACK类中，GetDatas(x,y,w,h)函数可以获得被跟踪者的最终人头位置(x,y),和滑动窗口大小(w,h)
- 模型：(这个是人头检测+人体特征提取)
  - HeadDetect.rknn 模型宽高(w,h)-- (640x640)
  - Deepsort_facenet.rknn 模型宽高(w,h)-- (64x128)
