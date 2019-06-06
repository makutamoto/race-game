#ifndef MATRIX_H
#define MATRIX_H

#define PI  3.14159265359

float dot2(const float a[2], const float b[2]);
float dot3(const float a[3], const float b[3]);
float length2(const float vector[2]);
float distance2(const float a[2], const float b[2]);
float* addVec2(const float a[2], const float b[2], float out[2]);
float* subVec2(const float a[2], const float b[2], float out[2]);
float* mulVec2ByScalar(const float vector[2], float scalar, float out[2]);
float* normalize2(const float vector[2], float out[2]);
float* direction2(const float a[2], const float b[2], float out[2]);
float angleVec2(const float vector[2]);
float	(*mulMat3(const float a[3][3], const float b[3][3], float out[3][3]))[3];
float* mulMat3Vec3(const float mat[3][3], const float vec[3], float out[3]);
float (*transposeMat3(const float mat[3][3], float out[3][3]))[3];

float (*genTranslationMat3(float dx, float dy, float mat[3][3]))[3];
float (*genIdentityMat3(float mat[3][3]))[3];
float (*genScaleMat3(float sx, float sy, float mat[3][3]))[3];
float (*genRotationMat3(float rotation, float mat[3][3]))[3];

#endif
