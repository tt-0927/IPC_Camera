# CPU算法模块

| 序号 | 模型 | 位置 | 输入格式 | 输出 |
|:--------:|:--------:| :---------:|:--------:|:--------|
| 1 | Byte跟踪算法 | ByteTrack | 目标框 | 跟踪id、跟踪目标框|
| 2 | 高空抛物跟踪算法 | HighAltitudeTrack | 目标框 | 是否是下落物体、下落物体目标框 |
| 3 | 摄像头遮挡检测 | CameraObstruction | RGB | |
| 4 | 声音异常检测 | AudioAnomaly | 音频 |  |
| 5 | 色彩异常检测 | ImageAnomaly | RGB | |
| 6 | 视频卡顿检测 | ShapeDetect | RGB | |
| 7 | 摄像头移动检测 | MoveDetect | RGB | 目标框 |
