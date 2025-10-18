#ifndef KMPE_COST_FUNCTION_H
#define KMPE_COST_FUNCTION_H

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <Eigen/Dense>
#include <cmath>

class KMPECostFunction {
public:
    KMPECostFunction(double sigma = 2.0, double p = 1.0, 
                     double error_threshold = 0.1, double eps = 1e-6)
        : sigma_sq(sigma * sigma), p(p),
          error_threshold_sq(error_threshold * error_threshold),
          eps(eps) {}

    double compute(const Eigen::Vector3d& angles, const Eigen::Vector3d& translation,
                   const pcl::PointCloud<pcl::PointXYZ>::Ptr& target,
                   const pcl::PointCloud<pcl::PointXYZ>::Ptr& source) const {
        if (target->size() != source->size()) {
            throw std::runtime_error("Point clouds must have the same size");
        }

        Eigen::Matrix3d R = (Eigen::AngleAxisd(angles.z(), Eigen::Vector3d::UnitZ()) *
                             Eigen::AngleAxisd(angles.y(), Eigen::Vector3d::UnitY()) *
                             Eigen::AngleAxisd(angles.x(), Eigen::Vector3d::UnitX())).matrix();
        
        double total_cost = 0.0;
        size_t N = source->size();

        for (size_t i = 0; i < N; ++i) {
            Eigen::Vector3d src_pt(source->points[i].x, 
                                  source->points[i].y, 
                                  source->points[i].z);
            Eigen::Vector3d tgt_pt(target->points[i].x, 
                                  target->points[i].y, 
                                  target->points[i].z);
            
            Eigen::Vector3d transformed = R * src_pt + translation;
            Eigen::Vector3d delta = tgt_pt - transformed;
            
            double delta_sq_sum = delta.squaredNorm();
            double fi_ij = 0.0;

            if (delta_sq_sum <= error_threshold_sq) {
                fi_ij = std::pow(1.0 - std::exp(-delta_sq_sum/(2.0*sigma_sq)), p/2.0);
            } else {
                double penalty_factor = std::pow(delta_sq_sum / error_threshold_sq, 2);
                fi_ij = std::abs(delta_sq_sum) * penalty_factor + delta_sq_sum;
            }
            total_cost += fi_ij;
        }

        return total_cost / (N + eps);
    }

private:
    const double sigma_sq;           
    const double p;                  
    const double error_threshold_sq; 
    const double eps;                
};
#endif // KMPE_COST_FUNCTION_H