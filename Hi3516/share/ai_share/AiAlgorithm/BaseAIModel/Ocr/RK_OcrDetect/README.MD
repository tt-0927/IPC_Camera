# RK_OcrDetect

- 功能：文字检测
- 依赖：opencv库、librknn_api
- 模型推理入口：
  - 调用函数
    - 文字检测
      - OcrDetect/include/dbnet_ocr_detect.h  ---> DetectOcrRgb(xx)函数
  - 详细使用，查看demo/main.cpp
- 模型：(这个是文字检测+文字识别)
  - DBnetOcrDetect.rknn 模型宽高(w,h)-- (640x640)
