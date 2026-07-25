# rk_pipeline

RK 应用平台API  


## 目录结构
### 1. `rockit` 多媒体处理平台 RK MPI
- **描述** 基于RK MPI API 进行的封装。
- **子模块**：
  - `adec`：⾳频解码。
  - `aenc`：⾳频编码。
  - `ai`：⾳频输⼊。
  - `ao`：⾳频输出。
  - `common`：公共功能，如媒体模块绑定接口。
  - `drm`：DRM渲染框架。
  - `example`：例程。
  - `example_new`：新例程。
  - `im2d_api`：图像转二维数组。
  - `include`：RK MPI API头文件。
  - `lib`：RK MPI API相关动态库。
  - `rgn`：区域管理。
  - `tde`：RGA快速的图形处理。
  - `vdec`：视频解码。
  - `venc`：视频编码。
  - `vgs`：视频图形⼦系统，对输⼊的图像进⾏缩放、旋转、打OSD、COVER、画线等操作。
  - `vi`：视频输入。
  - `vo`：视频输出。
  - `vpss`：视频处理⼦系统，CROP、Scale、像素格式转换、固定⻆度旋转、Cover/Coverex、Mirror/Flip、压缩解压等。

### 2. `rkaiq` Rk自动图像质量优化和处理模块 (Rk Auto Image Quality)  
- **描述** 存放相关头文件等。
- **子模块**：
  - `include`：RK AIQ 头文件。