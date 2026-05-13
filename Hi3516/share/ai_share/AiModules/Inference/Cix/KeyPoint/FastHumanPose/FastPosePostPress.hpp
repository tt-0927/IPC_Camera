#include <vector>
#include <algorithm>
#include <numeric>
#include <cmath>
#include "OutputDataEXT.hpp"

namespace Inference_NS
{
    void heatmap2Keypoint(std::vector<float *> vInput, std::vector<Inference_NS::PointData_S> &vPointDatas)
    {
        Inference_NS::PointData_S stOnmeData;

        const int N = 1, C = 26, H = 64, W = 48;
        const int stride = 256 / H; // 4
        const int plane = H * W;    // 64*48

        // 预先生成坐标表
        std::vector<float> coordX(W), coordY(H);
        for (int x = 0; x < W; ++x)
            coordX[x] = static_cast<float>(x);
        for (int y = 0; y < H; ++y)
            coordY[y] = static_cast<float>(y);

        for (int c = 0; c < C; ++c) // 每个关键点
        {
            const float *p = vInput[0] + c * plane;

            // 1. 求 confidence = max(p)
            float conf = *std::max_element(p, p + plane);

            // 2. 归一化：先求和，再逐元素除
            float sum = std::accumulate(p, p + plane, 0.0f);
            sum = 1.0f / (sum + 1e-12f); // 避免除 0

            // 3. 求期望坐标
            float Ex = 0.0f, Ey = 0.0f;

            // 3.1 对 y 方向积分 -> 得到 1×W 的 marginal X
            std::vector<float> mx(W, 0.0f);
            for (int y = 0; y < H; ++y)
                for (int x = 0; x < W; ++x)
                    mx[x] += p[y * W + x] * sum;

            Ex = std::inner_product(mx.begin(), mx.end(), coordX.begin(), 0.0f);

            // 3.2 对 x 方向积分 -> 得到 1×H 的 marginal Y
            std::vector<float> my(H, 0.0f);
            for (int y = 0; y < H; ++y)
                for (int x = 0; x < W; ++x)
                    my[y] += p[y * W + x] * sum;

            Ey = std::inner_product(my.begin(), my.end(), coordY.begin(), 0.0f);

            /* 4. 乘 stride 并写回 */
            Point_S stdData;
            stdData.nX = (int)Ex * stride;
            stdData.nY = (int)Ey * stride;
            stdData.nShow = conf;
            stOnmeData.vPoints.push_back(stdData);
        }
        vPointDatas.push_back(stOnmeData);
    }
}