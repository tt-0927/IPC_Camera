
初始化
#include "llm_analysis.hpp"

// 创建LLM推理任务实例
CLLmInference llmInferenceTask;
//  开始运行LLM模型
llmInferenceTask.StartLLMInferenceTask();



使用：

// 1. 纯文本查询
llmInferenceTask.sendTextQuery("你好，请介绍一下你自己", 0);

// 2. 直接传入图像的查询
cv::Mat image = cv::imread("test.jpg");
llmInferenceTask.sendImageQuery(image, "请描述这张图片的内容", 1);

// 3. 从VPSS通道获取图像的查询
llmInferenceTask.sendVpssQuery(2, "请分析这个通道的视频内容", 2);

// 4. 从本地文件加载图像的查询
llmInferenceTask.sendLocalImageQuery("/sdcard/images/photo.jpg", "这是什么场景", 3);


查询记录：

// 获取所有记录
auto records = llmInferenceTask.getAllRecords();
  for (const auto& record : records) {
  std::cout << record.getSummary() << std::endl;
}

// 导出到CSV
llmInferenceTask.exportRecordsToCSV("/mnt/demo_Linux_armhf/rkllm/llm_records/export.csv");

// 根据标签搜索记录
auto imageRecords = llmInferenceTask.getRecordsByTag("图像分析");
std::cout << "找到 " << imageRecords.size() << " 条图像分析记录:" << std::endl;
for (const auto& record : imageRecords) {
std::cout << record.getSummary() << std::endl;