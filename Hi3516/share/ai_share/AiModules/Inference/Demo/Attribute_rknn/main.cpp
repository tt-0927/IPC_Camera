/**
 * @file main.cpp
 * @author wucp (wucp@kfb.cn)
 * @date 2024-11-05
 *
 * @brief 车辆属性性能测试
 */
#include <chrono>
#include <cmath>
#include "opencv2/core/core.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "CVInferenceRK.hpp"

#include "Attribute.hpp"
#include "Accuracy.hpp"
#include "ReadData.hpp"

using namespace Inference_NS;

/* 进度条 */
void showProgress(int current, int total)
{
    int bar_width = 50; /* 进度条的宽度 */
    float progress = static_cast<float>(current) / total;

    std::cout << "[";
    int pos = bar_width * progress;
    for (int i = 0; i < bar_width; ++i)
    {
        if (i < pos)
            std::cout << "=";
        else if (i == pos)
            std::cout << ">";
        else
            std::cout << " ";
    }
    std::cout << "] " << int(progress * 100.0) << "% " << "["<< current << "/" << total << "]\r";
    std::cout.flush();
}

/* 获取标签，以及图片的地址 */
bool getLabel(std::string sFilename, std::vector<std::vector<int>> &vLabels, std::vector<std::string> &vImgPaths)
{
    bool bFlag = true;
    std::vector<std::vector<std::string>> data = readFile(sFilename);
    splitHead(data, vImgPaths);
    bFlag = convertInt(data, vLabels);
    return bFlag;
}

/* 按批次进行性能总结 */
bool getBatchSizeMetric(CAttribute *pDetect, std::vector<std::vector<int>> &vLabels, std::vector<std::string> &vImgPaths, int nEpochNum, int nBatchSize, MetricsResult_S &stResult)
{
    if (vLabels.size() != vImgPaths.size())
    {
        printf("输入的标签长度[%d]!=图片的长度[%d]\n", vLabels.size(), vImgPaths.size());
        return false;
    }

    int nStart = nEpochNum * nBatchSize;
    int nEnd = (nEpochNum + 1) * nBatchSize;
    if (nStart >= vImgPaths.size())
    {
        printf("图片数据遍历完毕\n");
        return true;
    }
    if (nEnd >= vImgPaths.size())
    {
        nEnd = vImgPaths.size();
    }

    std::vector<std::vector<float>> vPredsProbs;
    for (int i = nStart; i < nEnd; i++)
    {
        std::string sImgPath = "images/" + vImgPaths[i];
        /* 推理 */
        cv::Mat aInput = cv::imread(sImgPath);
        // resizeAndPadImage(aInput, cv::Size(256, 256), aInput);
        cv::resize(aInput, aInput, cv::Size(256, 256));

        /* 模型推理 */
        Inference_NS::InputData_S stInputData;
        stInputData.pData = (float*)aInput.data;
        stInputData.nDataSize = static_cast<int>(aInput.total() * aInput.elemSize())* sizeof(float);    
        std::vector<Inference_NS::ClsData_S> vClsDatas;
        pDetect->inference(stInputData, vClsDatas);
        /* 模型输出 */
        std::vector<float> vResult;
        vResult = pDetect->vTestResult; /* 神经网络的输出 */
        vPredsProbs.push_back(vResult);
        showProgress(i,nBatchSize);
    }

    /* 性能对比 */
    const std::vector<std::vector<int>> vGTLabels(vLabels.begin() + nStart, vLabels.begin() + nEnd);
    if (vPredsProbs.size() != vGTLabels.size())
    {
        printf("推理输出大小[%d] != 标签大小[%d]\n", vPredsProbs.size(), vGTLabels.size());
        return false;
    }

    getPedestrianMetrics(
        stResult,    /* 返回結果 */
        vGTLabels,   /* 真实标签 */
        vPredsProbs, /* 预测概率 */
        0.5          /* 预测概率阈值 */
    );
    return true;
}

int main(int argc, char *argv[])
{
    // 检查是否有足够的参数
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << "<aiConfigPath>  <LabelPath>" << std::endl;
        return 1;
    }

    /* 初始化 */
    CAttribute *demo = new CAttribute(argv[1]);
    bool bT = demo->init();
    if (!bT)
    {
        printf("初始化参数识别\n");
        exit(0);
    }

#if 1
    std::string sImgPath = argv[2];
    cv::Mat aInput = cv::imread(sImgPath);
    cv::resize(aInput, aInput, cv::Size(256, 256));
    cv::cvtColor(aInput, aInput, cv::COLOR_BGR2RGB);

    /* 模型推理 */
    Inference_NS::InputData_S stInputData;
    stInputData.pData = (float*)aInput.data;
    stInputData.nDataSize = static_cast<int>(aInput.total() * aInput.elemSize())* sizeof(float);    
    std::vector<Inference_NS::ClsData_S> vClsDatas;
    demo->inference(stInputData, vClsDatas);
    for(int i=0;i<vClsDatas.size();i++)
    {
        for(int j=0; j<vClsDatas[i].vCls.size();j++)
        {
            printf("nLabel[%d] - fConfidence[%f]\n",vClsDatas[i].vCls[j].nLabel, vClsDatas[i].vCls[j].fConfidence);
        }
    }

#else
    /* 启用模型性能测试 */
    demo->bPerformanceTest = true;

    /* 获取标签信息 */
    std::vector<std::vector<int>> vLabels;
    std::vector<std::string> vImgPaths;
    bool bFlag = getLabel(argv[2], vLabels, vImgPaths);
    if (!bFlag)
    {
        printf("----------------- 读取标签数据失败 ---------------\n");
        return -1;
    }
    else if (vLabels.size() != vImgPaths.size())
    {
        printf("标签数量%d] != 图片的数量[%d]\n", vLabels.size(), vImgPaths.size());
        return -1;
    }

    /* 性能测试 */
    int nBatchSize = vImgPaths.size();  /* 批次需要设置大 */
    int nEnpoch = std::ceil(vImgPaths.size() * 1.0 / nBatchSize);

    /* 批量数据性能测试 */
    float ma = 0.0;
    float label_f1 = 0.0;
    float pos_recall = 0.0;
    float neg_recall = 0.0;
    float Acc = 0.0;
    float Prec = 0.0;
    float Rec = 0.0;
    float F1 = 0.0;
    for (int nEpochN = 0; nEpochN < nEnpoch; nEpochN++)
    {
        MetricsResult_S stResult;
        bFlag = getBatchSizeMetric(demo, vLabels, vImgPaths, nEpochN, nBatchSize, stResult);
        if (!bFlag)
        {
            printf("标签与模型输出性能计算报错\n");
            return -1;
        }

        ma += stResult.ma;
        label_f1 += stResult.label_f1;
        pos_recall += stResult.label_pos_recall;
        neg_recall += stResult.label_neg_recall;
        Acc += stResult.instance_acc;
        Prec += stResult.instance_prec;
        Rec += stResult.instance_recall;
        F1 += stResult.instance_f1;

        printf("[epoch:%d] ma: %.4f, label_f1: %.4f, pos_recall: %.4f, neg_recall: %.4f, Acc: %.4f, Prec: %.4f, Rec: %.4f, F1:%.4f\n",
               nEpochN, stResult.ma, stResult.label_f1, stResult.label_pos_recall, stResult.label_neg_recall, stResult.instance_acc, stResult.instance_prec, stResult.instance_recall, stResult.instance_f1);
    }

    ma /= nEnpoch;
    label_f1 /= nEnpoch;
    pos_recall /= nEnpoch;
    neg_recall /= nEnpoch;
    Acc /= nEnpoch;
    Prec /= nEnpoch;
    Rec /= nEnpoch;
    F1 /= nEnpoch;
    printf("-------------------------------- 数据集的总体性能如下: ---------------------------------\n");
    printf("ma: %.4f, label_f1: %.4f, pos_recall: %.4f, neg_recall: %.4f, Acc: %.4f, Prec: %.4f, Rec: %.4f, F1:%.4f\n",
           ma, label_f1, pos_recall, neg_recall, Acc, Prec, Rec, F1);

#endif

    delete demo;
    return 0;
}
