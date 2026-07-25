#include <iostream>
#include <vector>
#include <numeric>
#include <cmath>

/* 定义一个结构体用于存储性能指标结果 */
typedef struct _METRICSRESULT_
{
    double label_pos_recall; /* 标签正类召回率 */
    double label_neg_recall; /* 标签负类召回率 */
    double label_prec;       /* 标签精确率 */
    double label_acc;        /* 标签准确率 */
    double label_f1;         /* 标签 F1 分数 */
    double ma;               /* 所有标签的整体平均准确率 */

    double instance_acc;    /* 实例准确率 */
    double instance_prec;   /* 实例精确率 */
    double instance_recall; /* 实例召回率 */
    double instance_f1;     /* 实例 F1 分数 */

    int error_num; /* 错误数量 (FP + FN) */
    int fn_num;    /* 假阴性数量 */
    int fp_num;    /* 假阳性数量 */
} MetricsResult_S;

/*计算行人检测指标的函数 */
void getPedestrianMetrics(
    MetricsResult_S& stResult,                            /* 返回結果 */
    const std::vector<std::vector<int>> &vGTLabels,      /* 真实标签 */
    const std::vector<std::vector<float>> &vPredsProbs, /* 预测概率 */
    float dThreshold = 0.5                              /* 预测概率阈值 */
);