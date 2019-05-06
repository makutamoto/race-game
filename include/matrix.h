#ifndef MATRIX_H
#define MATRIX_H

float dot2(const float a[2], const float b[2]);
float dot3(const float a[3], const float b[3]);
float length2(const float vector[2]);
float	(*mulMat3(const float a[3][3], const float b[3][3], float out[3][3]))[3];
float* mulMat3Vec3(const float mat[3][3], const float vec[3], float out[3]);

float (*genTranslationMat3(float dx, float dy, float mat[3][3]))[3];
float (*genIdentityMat3(float mat[3][3]))[3];
float (*genScaleMat3(float sx, float sy, float mat[3][3]))[3];
float (*genRotationMat3(float rotation, float mat[3][3]))[3];

#endif
