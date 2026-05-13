#ifndef KALMAN_FILTER_HPP
#define KALMAN_FILTER_HPP

#include <Eigen/Dense>
#include <stdexcept>

class KalmanFilter {
public:
    KalmanFilter(int dim_x, int dim_z, int dim_u = 0);

    void predict(const Eigen::VectorXd& u = Eigen::VectorXd(), 
                 const Eigen::MatrixXd& B = Eigen::MatrixXd(), 
                 const Eigen::MatrixXd& F = Eigen::MatrixXd(), 
                 const Eigen::MatrixXd& Q = Eigen::MatrixXd());

    void update(const Eigen::VectorXd& z, 
                const Eigen::MatrixXd& R = Eigen::MatrixXd(), 
                const Eigen::MatrixXd& H = Eigen::MatrixXd());

    // Getters for state and covariance
    Eigen::VectorXd getState() const { return x; }
    Eigen::MatrixXd getCovariance() const { return P; }

public:
    int dim_x, dim_z, dim_u;
    Eigen::VectorXd x;            // State vector
    Eigen::MatrixXd P;            // State covariance
    Eigen::MatrixXd Q;            // Process noise covariance
    Eigen::MatrixXd B;            // Control transition matrix
    Eigen::MatrixXd F;            // State transition matrix
    Eigen::MatrixXd H;            // Measurement function
    Eigen::MatrixXd R;            // Measurement noise covariance
    Eigen::MatrixXd K;            // Kalman gain
    Eigen::VectorXd y;            // Measurement residual
    Eigen::MatrixXd S;            // System uncertainty in measurement space
    Eigen::MatrixXd SI;           // Inverse of system uncertainty

    Eigen::MatrixXd I;            // Identity matrix

    Eigen::VectorXd x_prior;      // Prior state
    Eigen::MatrixXd P_prior;      // Prior covariance
    Eigen::VectorXd x_post;       // Posterior state
    Eigen::MatrixXd P_post;       // Posterior covariance

private:
    Eigen::MatrixXd invertMatrix(const Eigen::MatrixXd& matrix);
};

#endif // KALMAN_FILTER_HPP
