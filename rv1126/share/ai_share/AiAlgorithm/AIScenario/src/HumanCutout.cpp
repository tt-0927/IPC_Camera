/*
*  File Name:HumanCutout.cpp
*  Created on: 2024年5月17日
*  Author: liaoet
*  description :人像抠图方法	
*  Modify date: 
*/

# include "HumanCutout.hpp"

/* 模型预加载 */
HumanCutout::HumanCutout()
{
    m_model = new Model(m_strModelPath);
}

/* 对图片人像抠图 */
void HumanCutout::detection(cv::Mat image) {
    /* 获取原始图片的宽高 */
    m_nImgHeight = image.rows;
    m_nImgWidth = image.cols;
    /* 图片缩放大小计算 */
    if (std::max(m_nImgHeight, m_nImgWidth) < m_nRefSize || std::min(m_nImgHeight, m_nImgWidth) > m_nRefSize) {
        if (m_nImgWidth >= m_nImgHeight) {
            m_nImgRh = m_nRefSize;
            m_nImgRw = static_cast<int>(m_nImgWidth * m_nRefSize / m_nImgHeight);
        } else {
            m_nImgRw = m_nRefSize;
            m_nImgRh = static_cast<int>(m_nImgHeight * m_nRefSize / m_nImgWidth);
        }
    } else {
        m_nImgRh = m_nImgHeight;
        m_nImgRw = m_nImgWidth;
    }
    m_nImgRw -= m_nImgRw % 32;
    m_nImgRh -= m_nImgRh % 32;
    /* 进行缩放 */
    cv::Mat dst, dst_float;
    cv::resize(image, dst, cv::Size(m_nImgRw, m_nImgRh), 0, 0);
    dst.convertTo(dst_float, CV_32F);
    std::cerr << "原始图片大小：" << m_nImgWidth << "x" << m_nImgHeight << "，缩放后图片大小：" << m_nImgRw << "x" << m_nImgRh << std::endl;
    /* 模型归一化 */
    std::vector<cv::Mat> channels, normalized_image;
    cv::split(image, channels);
    cv::Mat r, g, b;
    b = channels.at(0);
    g = channels.at(1);
    r = channels.at(2);
    b = (b / 255. - 0.5) / 0.5;
    g = (g / 255. - 0.5) / 0.5;
    r = (r / 255. - 0.5) / 0.5;
    normalized_image.push_back(r);
    normalized_image.push_back(g);
    normalized_image.push_back(b);
    cv::Mat out = cv::Mat(image.rows, image.cols, CV_32F);
    cv::merge(normalized_image, out);
    cv::Mat blob = cv::dnn::blobFromImage(out, 1, cv::Size(m_nImgRw, m_nImgRh), cv::Scalar(0, 0, 0), false, true);
    std::cerr << "原始图片大小：" << blob.total() << std::endl;

    /* 模型推理 */
    std::vector<int64_t> input_node_dims = { 1, 3, m_nImgRh, m_nImgRw };
    cv::Mat mask = m_model->Inference(blob, input_node_dims);

    /* 恢复mask到原图大小 */
    cv::resize(mask, mask, cv::Size(m_nImgWidth, m_nImgHeight), 0, 0);
    /* 根据检测结果截取图片 */
    cv::Mat result_image;
    cv::bitwise_and(image, image, result_image, mask = mask);

    /* 展示图片 */
    cv::imshow("Image", result_image);
    cv::waitKey(0);

}

