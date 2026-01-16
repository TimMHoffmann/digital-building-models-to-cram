#include "Matrix4x4.hpp"
#include <cmath>
#include <stdexcept>
#include <iostream>

Matrix4x4::Matrix4x4() {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            m[i][j] = (i == j) ? 1.0 : 0.0;
        }
    }
}

void Matrix4x4::setTranslation(double x, double y, double z) {
    m[0][3] = x;
    m[1][3] = y;
    m[2][3] = z;
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& other) const {
    Matrix4x4 result;

    for (int i = 0; i < 4; ++i) { 
        for (int j = 0; j < 4; ++j) {
            result.m[i][j] = 0.0;
            for (int k = 0; k < 4; ++k) {
                result.m[i][j] += m[i][k] * other.m[k][j];
            }
        }
    }
    return result;
}

std::array<double, 3> Matrix4x4::transformPoint(const std::array<double, 3>& point) const {
    double x = point[0];
    double y = point[1];
    double z = point[2];

    std::array<double, 3> result;

    result[0] = m[0][0] * x + m[0][1] * y + m[0][2] * z + m[0][3];
    result[1] = m[1][0] * x + m[1][1] * y + m[1][2] * z + m[1][3];
    result[2] = m[2][0] * x + m[2][1] * y + m[2][2] * z + m[2][3];
    
    return result;
}