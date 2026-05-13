# RK_OcrRep

- 功能：文字识别
- 依赖：opencv库、librknn_api
- 模型推理入口：
  - 调用函数
    - 文字识别
      - OcrRep/include/ocr_repvgg.h  ---> DetectOcrRgb(xx)函数
  - 详细使用，查看demo/main.cpp
- 模型：(这个是文字检测+文字识别)
  - VggOcrREPt.rknn 模型宽高(w,h)-- (448,32)
  - TextDict.txt  语料文件，使用必要
