# RK_CameraTrack

- 功能：人头检测+人体特征匹配+摄像头移动的跟踪算法
- 依赖：opencv库、librknn_api
- 模型推理入口：
  - Auxiliary/include/auxiliary_track.h 中的， --》 AI(xx)函数
  - demo/main.cpp中，90、91、966（送char*帧识别）、获取人头坐标给全局变量vRtPoints；模型配置等可以通过SLIDETRACKPARAM结构体进行初始化。

- 模型：(这个是人头检测+人体特征提取)
  - HeadDetect.rknn 模型宽高(w,h)-- (640x640)
  - Deepsort_facenet.rknn 模型宽高(w,h)-- (64x128)
