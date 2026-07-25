#include "KalmanFilter.hpp"
#include <iostream>

// 构造函数
KalmanFilter::KalmanFilter(int dim_x, int dim_z, int dim_u)
    : dim_x(dim_x), dim_z(dim_z), dim_u(dim_u),
      x(Eigen::VectorXd::Zero(dim_x)), 
      P(Eigen::MatrixXd::Identity(dim_x, dim_x)), 
      Q(Eigen::MatrixXd::Identity(dim_x, dim_x)), 
      B(Eigen::MatrixXd::Zero(dim_x, dim_u)),
      F(Eigen::MatrixXd::Identity(dim_x, dim_x)), 
      H(Eigen::MatrixXd::Zero(dim_z, dim_x)),
      R(Eigen::MatrixXd::Identity(dim_z, dim_z)),
      K(Eigen::MatrixXd::Zero(dim_x, dim_z)),
      y(Eigen::VectorXd::Zero(dim_z)), 
      S(Eigen::MatrixXd::Zero(dim_z, dim_z)), 
      SI(Eigen::MatrixXd::Zero(dim_z, dim_z)), 
      I(Eigen::MatrixXd::Identity(dim_x, dim_x)),
      x_prior(Eigen::VectorXd::Zero(dim_x)), 
      P_prior(Eigen::MatrixXd::Identity(dim_x, dim_x)), 
      x_post(Eigen::VectorXd::Zero(dim_x)), 
      P_post(Eigen::MatrixXd::Identity(dim_x, dim_x)) {}

// 预测函数
void KalmanFilter::predict(const Eigen::VectorXd& u, 
                           const Eigen::MatrixXd& B, 
                           const Eigen::MatrixXd& F, 
                           const Eigen::MatrixXd& Q) {
    if (F.size() > 0) this->F = F;
    if (Q.size() > 0) this->Q = Q;
    if (B.size() > 0) this->B = B;

    // x = F * x + B * u
    if (this->B.size() > 0 && u.size() > 0) {
        x = this->F * x + this->B * u;
    } else {
        x = this->F * x;
    }

    // P = F * P * F.transpose() + Q
    P = this->F * P * this->F.transpose() + this->Q;

    // 保存预测后的状态和协方差
    x_prior = x;
    P_prior = P;
}

// 更新函数
void KalmanFilter::update(const Eigen::VectorXd& z, 
                          const Eigen::MatrixXd& R, 
                          const Eigen::MatrixXd& H) {
    if (z.size() == 0) return;  // 无观测时不更新

    if (R.size() > 0) this->R = R;
    if (H.size() > 0) this->H = H;

    // y = z - H * x
    y = z - this->H * x;

    // PHT = P * H.transpose()
    Eigen::MatrixXd PHT = P * this->H.transpose();

    // S = H * PHT + R
    S = this->H * PHT + this->R;
    SI = invertMatrix(S);

    // K = PHT * SI
    K = PHT * SI;

    // x = x + K * y
    x = x + K * y;

    // P = (I - K * H) * P
    P = (I - K * this->H) * P;

    // 保存更新后的状态和协方差
    x_post = x;
    P_post = P;
}

// 辅助函数：计算矩阵的逆
Eigen::MatrixXd KalmanFilter::invertMatrix(const Eigen::MatrixXd& matrix) {
    return matrix.inverse();
}
