#include "Accuracy.hpp"

void getPedestrianMetrics(
    MetricsResult_S &stResult,                          /* 返回結果 */
    const std::vector<std::vector<int>> &vGTLabels,     /* 真实标签 */
    const std::vector<std::vector<float>> &vPredsProbs, /* 预测概率 */
    float dThreshold                                    /* 预测概率阈值 */
)
{
    size_t n = vGTLabels.size();             /* 样本数量 */
    size_t num_labels = vGTLabels[0].size(); /* 标签数量 */
    // MetricsResult stResult = {};               /* 初始化结果 */

    const double eps = 1e-20; /* 避免除零错误 */

    /* 初始化标签级和实例级的 TP, TN, FP, FN */
    std::vector<double> true_pos(num_labels, 0);  /* 真实正类数量 */
    std::vector<double> true_neg(num_labels, 0);  /* 真实负类数量 */
    std::vector<double> false_pos(num_labels, 0); /* 假阳性数量 */
    std::vector<double> false_neg(num_labels, 0); /* 假阴性数量 */

    std::vector<double> gt_pos(num_labels, 0); /* 正类数量 */
    std::vector<double> gt_neg(num_labels, 0); /* 负类数量 */

    /* 计算 TP, TN, FP, FN */
    for (size_t i = 0; i < n; ++i)
    { /* 遍历每个样本 */
        for (size_t j = 0; j < num_labels; ++j)
        {                                                /* 遍历每个标签 */
            int gt = vGTLabels[i][j];                    /* 真实标签 */
            float pred_prob = vPredsProbs[i][j];         /* 预测概率 */
            int pred = (pred_prob > dThreshold) ? 1 : 0; /* 根据阈值判断预测结果 */

            /* 统计 TP, TN, FP, FN */
            if (gt == 1)
            {
                gt_pos[j]++; /* 正类数量加一 */
                if (pred == 1)
                {
                    true_pos[j]++; /* 真实正类和预测正类 */
                }
                else
                {
                    false_neg[j]++; /* 真实正类但预测为负类 */
                }
            }
            else
            {
                gt_neg[j]++; /* 负类数量加一 */
                if (pred == 1)
                {
                    false_pos[j]++; /* 真实负类但预测为正类 */
                }
                else
                {
                    true_neg[j]++; /* 真实负类和预测负类 */
                }
            }
        }
    }

    /* 计算标签级指标 */
    double total_label_pos_recall = 0; /* 总正类召回率 */
    double total_label_neg_recall = 0; /* 总负类召回率 */
    double total_label_prec = 0;       /* 总精确率 */
    double total_label_acc = 0;        /* 总准确率 */
    double total_label_f1 = 0;         /* 总 F1 分数 */
    double total_label_ma = 0;         /* 总均值准确率 */

    for (size_t j = 0; j < num_labels; ++j)
    {                                                                              /* 遍历每个标签 */
        double label_pos_recall = (gt_pos[j] > 0) ? (true_pos[j] / gt_pos[j]) : 0; /* 正类召回率 */
        double label_neg_recall = (gt_neg[j] > 0) ? (true_neg[j] / gt_neg[j]) : 0; /* 负类召回率 */

        total_label_pos_recall += label_pos_recall; /* 累加正类召回率 */
        total_label_neg_recall += label_neg_recall; /* 累加负类召回率 */

        double label_prec = (true_pos[j] + false_pos[j] > 0) ? (true_pos[j] / (true_pos[j] + false_pos[j])) : 0; /* 精确率 */
        total_label_prec += label_prec;                                                                          /* 累加精确率 */

        double label_acc = (true_pos[j] + false_pos[j] + false_neg[j] > 0) ? (true_pos[j] / (true_pos[j] + false_pos[j] + false_neg[j])) : 0; /* 精确率 */
        total_label_acc += label_acc;                                                                                                         /* 累加精确率 */

        double label_f1 = (label_prec + label_pos_recall > 0) ? (2 * label_prec * label_pos_recall / (label_prec + label_pos_recall)) : 0; /* F1 分数 */
        total_label_f1 += label_f1;
        /* 累加 F1 分数 */
        /* 计算当前标签的均值准确率 (label_ma)，并累加 */
        double label_ma = (label_pos_recall + label_neg_recall) / 2;
        total_label_ma += label_ma;
    }

    /* 计算平均值 */
    stResult.label_pos_recall = total_label_pos_recall / num_labels; /* 平均正类召回率 */
    stResult.label_neg_recall = total_label_neg_recall / num_labels; /* 平均负类召回率 */
    stResult.label_prec = total_label_prec / num_labels;             /* 平均精确率 */
    stResult.label_acc = total_label_acc / num_labels;               /* 标签准确率 */
    stResult.label_f1 = total_label_f1 / num_labels;                 /* 平均 F1 分数 */
    stResult.ma = total_label_ma / num_labels;                       /* 所有标签的整体平均准确率 */

    /* 计算实例级指标 */
    double instance_acc = 0, instance_prec = 0, instance_recall = 0, instance_f1 = 0;

    for (size_t i = 0; i < n; ++i)
    { /* 遍历每个样本 */
        double inst_true_pos = 0, inst_true_neg = 0, inst_false_pos = 0, inst_false_neg = 0;

        for (size_t j = 0; j < num_labels; ++j)
        {                                                /* 遍历每个标签 */
            float pred_prob = vPredsProbs[i][j];         /* 预测概率 */
            int pred = (pred_prob > dThreshold) ? 1 : 0; /* 根据阈值判断预测结果 */

            /* 统计实例级 TP, TN, FP, FN */
            if (pred == 1)
            {
                if (vGTLabels[i][j] == 1)
                {
                    inst_true_pos++; /* 真实正类和预测正类 */
                }
                else
                {
                    inst_false_pos++; /* 真实负类但预测为正类 */
                }
            }
            else
            {
                if (vGTLabels[i][j] == 1)
                {
                    inst_false_neg++; /* 真实正类但预测为负类 */
                }
                else
                {
                    inst_true_neg++; /* 真实负类和预测负类 */
                }
            }
        }

        // 计算实例准确率、精确率和召回率
        double union_pos = inst_true_pos + inst_true_neg + inst_false_pos + inst_false_neg + eps; /* 当前样本的总预测数量 */
        instance_acc += (inst_true_pos + inst_true_neg) / union_pos;                              /* 实例准确率 */
        instance_prec += (inst_true_pos) / (inst_true_pos + inst_false_pos + eps);                /* 实例精确率 */
        instance_recall += (inst_true_pos) / (inst_true_pos + inst_false_neg + eps);              /* 实例召回率 */
    }

    /* 计算平均实例级指标 */
    stResult.instance_acc = instance_acc / n;       /* 实例平均准确率 */
    stResult.instance_prec = instance_prec / n;     /* 实例平均精确率 */
    stResult.instance_recall = instance_recall / n; /* 实例平均召回率 */

    stResult.instance_f1 = (stResult.instance_prec + stResult.instance_recall > 0) ? (2 * stResult.instance_prec * stResult.instance_recall /
                                                                                      (stResult.instance_prec + stResult.instance_recall))
                                                                                   : 0; /* 实例 F1 分数 */

    /* 统计错误数量、假阴性和假阳性数量 */
    stResult.error_num = std::accumulate(false_pos.begin(), false_pos.end(), 0) +
                         std::accumulate(false_neg.begin(), false_neg.end(), 0);
    stResult.fn_num = std::accumulate(false_neg.begin(), false_neg.end(), 0);
    stResult.fp_num = std::accumulate(false_pos.begin(), false_pos.end(), 0);
}

/*
int main()
{
    // 示例数据
    std::vector<std::vector<int>> vGTLabels = {
        {1, 0, 1},
        {0, 1, 0},
        {1, 1, 1}};

    std::vector<std::vector<double>> vPredsProbs = {
        {0.1, 0.2, 0.3},
        {0.4, 0.6, 0.1},
        {0.95, 0.2, 0.8}}; // 预测概率

    MetricsResult stResult = get_pedestrian_metrics(vGTLabels, vPredsProbs, 0.5);

    // 输出结果
    std::cout << "Label Positive Recall: " << stResult.label_pos_recall << std::endl;
    std::cout << "Label Negative Recall: " << stResult.label_neg_recall << std::endl;
    std::cout << "Label Precision: " << stResult.label_prec << std::endl;
    std::cout << "Label ACC: " << stResult.label_acc << std::endl;
    std::cout << "Label F1 Score: " << stResult.label_f1 << std::endl;
    std::cout << "Label Mean Accuracy: " << stResult.ma << std::endl;
    std::cout << "Instance Accuracy: " << stResult.instance_acc << std::endl;
    std::cout << "Instance Precision: " << stResult.instance_prec << std::endl;
    std::cout << "Instance Recall: " << stResult.instance_recall << std::endl;
    std::cout << "Instance F1 Score: " << stResult.instance_f1 << std::endl;
    std::cout << "Error Count: " << stResult.error_num << std::endl;
    std::cout << "False Negative Count: " << stResult.fn_num << std::endl;
    std::cout << "False Positive Count: " << stResult.fp_num << std::endl;

    return 0;
}
*/