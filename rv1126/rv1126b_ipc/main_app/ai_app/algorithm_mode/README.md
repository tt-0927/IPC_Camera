# algorithm_mode 目录说明

本目录存放**不继承 `CAlgorithm`、不接入取流信号槽、由调用方驱动**的纯算法组件（`init()/unInit()/process()` 形态），组件本身不含事件业务逻辑。

与 ai_app 相邻目录的边界：

| 目录 | 形态 | 接入方式 |
|---|---|---|
| detect_mode | 事件算法实例（继承 CAlgorithm） | algo_stream_deal 接线取流 + 事件联动 |
| analysis_mode | 大模型/场景分析（仅 TV-3882TI） | CAlgoStreamDeal 管理 |
| algorithm_mode（本目录） | 纯算法组件，无业务逻辑 | 调用方自行构造并调用 process() |
| common | 基础设施（队列/数据库/结果管理等） | 被各处引用 |

## 目录内容

- `common/`：组件公共定义（模型路径宏、公共数据契约等，见 `common/algorithm_define.hpp`；各组件类别 ID 常量定义在各自组件头文件）
- `channel_occupation_detect_model/`：通道占用检测组件（纯 CPU 背景差分）
- `climb_detect_model/`：攀爬检测组件
- `face_attribute_model/`：人脸属性分析组件
- `face_detect_model/`：人脸检测组件
- `face_expression/`：人脸表情识别组件
- `face_quality_model/`：人脸质量评估组件
- `fire_person_detect/`：烟火行人检测组件
- `kitchen_detect/`：厨房14类检测组件
- `pmnm_detect/`：人车非检测组件

## face_attribute_model

人脸属性（性别/年龄/眼镜/胡子/口罩/表情）分析组件，推理复用 Inference 层 `CAttribute`。
仅做分类：单张人脸 RGB 图（尺寸须等于模型输入，组件不缩放）+ 推理 + 阈值判定，输出专有
`FaceAttributeTarget_S`（定义在组件头文件），不含事件业务。移植自
`share/ai_share/AiModules/Modules/FaceAttribute`（`CFaceAttributeV1_0`）。

## face_detect_model

人脸（框 + 5 关键点）检测组件，推理复用 Inference 层 `CYoloUltralyticsPoint`。
仅做检测：贴边缩放 + 推理 + 坐标还原，输出专有 `FaceDetectedTarget_S`（框、置信度、5 关键点，
定义在组件头文件），瞳距过滤、区域过滤等业务由调用方处理；贴边行为对齐旧 `FaceDetectV1_0`，
仅超限时缩小，小图原尺寸居中。

## face_expression

人脸表情（生气/厌恶/恐惧/高兴/难过/惊讶/中立）识别组件，推理复用 Inference 层 `CImageFeature`。
仅做分类：人脸图直接缩放到模型输入尺寸 + 推理 + 取最大概率类别，输出复用 `DetectedTarget_S`
（仅类别与置信度有效，坐标置 0），不含事件业务。移植自 `share/ai_share/AiModules/Modules/FaceExpression`。

## face_quality_model

人脸质量评估组件，推理复用 Inference 层 `CImageFeature`。
仅做评估：等比贴边 + 推理 + 评分（特征向量前 10 维均值），输出质量分（float），
质量阈值过滤由调用方处理。移植自 `share/ai_share/AiModules/Modules/FaceQualityAssessment`
（`CFaceQualityAssessmentV1_0`）。

## fire_person_detect

烟火行人（烟雾/火焰/行人）检测组件，推理复用 Inference 层 `CYoloUltralytics`。
仅做检测：letterbox + 推理 + 坐标还原，输出全部目标框（`DetectedTarget_S`），类别开关与事件业务由调用方处理。

## kitchen_detect

厨房 14 类场景（穿戴物/未穿戴/卫生/违规目标）检测组件，推理复用 Inference 层 `CYoloUltralytics`。
仅做检测：letterbox + 推理 + 坐标还原，输出全部目标框（`DetectedTarget_S`），合规判定与事件业务由调用方处理。

## pmnm_detect

人车非（人/机动车/非机动车）检测组件，推理复用 Inference 层 `CYoloUltralytics`。

## climb_detect_model

攀爬检测组件，推理复用 Inference 层 `CYoloUltralytics`。
仅做检测：letterbox + 推理 + 坐标还原，输出全部目标框（`DetectedTarget_S`），类别 0-攀爬，事件业务由调用方处理。

## channel_occupation_detect_model

通道占用检测组件（纯 CPU 背景差分），复用 Inference 层 `CChannelOccupationDetect`。
仅做单帧分析：输入 RGB 图 + 灵敏度档位（低/中/高），输出是否占用、目标框、面积占比与运动标志；
使能判断、区域换算、报警状态机等业务由调用方处理。移植自
`share/ai_share/AiModules/Modules/ChannelOccupationDetect`（`CChannelOccupationDetectV1_0`），不依赖 AiModules/Modules。
