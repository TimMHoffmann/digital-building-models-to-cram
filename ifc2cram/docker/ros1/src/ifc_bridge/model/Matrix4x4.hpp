#ifndef MATRIX4X4_HPP
#define MATRIX4X4_HPP

#include <array>

struct Matrix4x4 {
    double m[4][4]; 
    
     Matrix4x4();
    
    void setTranslation(double x, double y, double z);

    Matrix4x4 operator*(const Matrix4x4& other) const;
    std::array<double, 3> transformPoint(const std::array<double, 3>& point) const;
    Matrix4x4 inverse() const;
};

#endif // MATRIX4X4_HPP