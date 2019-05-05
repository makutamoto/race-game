#ifndef MATRIX_H
#define MATRIX_H

float dot2(const float a[2], const float b[2]);
float length(const float vector[2]);
float	(*mulMat2(const float a[2][2], const float b[2][2], float out[2][2]))[2];
float* mulMat2Vec2(const float mat[2][2], const float vec[2], float out[2]);

float (*genIdentityMat2(float mat[2][2]))[2];
float (*genScaleMat2(float sx, float sy, float mat[2][2]))[2];
float (*genRotationMat2(float rotation, float mat[2][2]))[2];

#endif
