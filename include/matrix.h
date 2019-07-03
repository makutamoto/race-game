#ifndef MATRIX_H
#define MATRIX_H

#define PI  3.14159265359F

float (*copyMat4(float src[4][4], float dest[4][4]))[4];

float dot2(const float a[2], const float b[2]);
float dot3(const float a[3], const float b[3]);
float dot4(const float a[4], const float b[4]);
float* cross(float a[3], float b[3], float out[3]);
float length2(const float vector[2]);
float length3(const float vector[3]);
float distance2(const float a[2], const float b[2]);
float* addVec2(const float a[2], const float b[2], float out[2]);
float* addVec3(const float a[3], const float b[3], float out[3]);
float* subVec2(const float a[2], const float b[2], float out[2]);
float* subVec3(const float a[3], const float b[3], float out[3]);
float* mulVec2ByScalar(const float vector[2], float scalar, float out[2]);
float* normalize2(const float vector[2], float out[2]);
float* normalize3(const float vector[3], float out[3]);
float* direction2(const float a[2], const float b[2], float out[2]);
float angleVec2(const float vector[2]);
float	(*mulMat3(const float a[3][3], const float b[3][3], float out[3][3]))[3];
float	(*mulMat4(const float a[4][4], const float b[4][4], float out[4][4]))[4];
float* mulMat3Vec3(const float mat[3][3], const float vec[3], float out[3]);
float* mulMat4Vec4(const float mat[4][4], const float vec[4], float out[4]);
float (*transposeMat3(const float mat[3][3], float out[3][3]))[3];

float (*genTranslationMat3(float dx, float dy, float mat[3][3]))[3];
float (*genTranslationMat4(float dx, float dy, float dz, float mat[4][4]))[4];
float (*genIdentityMat3(float mat[3][3]))[3];
float (*genIdentityMat4(float mat[4][4]))[4];
float (*genScaleMat3(float sx, float sy, float mat[3][3]))[3];
float (*genScaleMat4(float sx, float sy, float sz, float mat[4][4]))[4];
float (*genRotationMat3(float rotation, float mat[3][3]))[3];
float (*genRotationXMat4(float rotation, float mat[4][4]))[4];
float (*genRotationYMat4(float rotation, float mat[4][4]))[4];
float (*genRotationZMat4(float rotation, float mat[4][4]))[4];
float (*genRotationMat4(float rx, float ry, float rz, float mat[4][4]))[4];

float (*genLookAtMat4(float position[3], float target[3], float worldUp[3], float mat[4][4]))[4];
float (*genPerspectiveMat4(float fov, float near, float far, float aspect, float mat[4][4]))[4];

void printVec4(float vec[4]);
void printMat4(float mat[4][4]);

#endif
