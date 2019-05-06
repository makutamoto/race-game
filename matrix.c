#include<math.h>

#include "./include/matrix.h"

float dot2(const float a[2], const float b[2]) {
	return a[0] * b[0] + a[1] * b[1];
}

float dot3(const float a[3], const float b[3]) {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float length2(const float vector[2]) {
	return sqrt(vector[0] * vector[0] + vector[1] * vector[1]);
}

float	(*mulMat3(const float a[3][3], const float b[3][3], float out[3][3]))[3] {
	float b0[3] = { b[0][0], b[1][0], b[2][0] };
	float b1[3] = { b[0][1], b[1][1], b[2][1] };
	float b2[3] = { b[0][2], b[1][2], b[2][2] };
	out[0][0] = dot3(a[0], b0);
	out[0][1] = dot3(a[0], b1);
	out[0][2] = dot3(a[0], b2);
	out[1][0] = dot3(a[1], b0);
	out[1][1] = dot3(a[1], b1);
	out[1][2] = dot3(a[1], b2);
	out[2][0] = dot3(a[2], b0);
	out[2][1] = dot3(a[2], b1);
	out[2][2] = dot3(a[2], b2);
	return out;
}

float* mulMat3Vec3(const float mat[3][3], const float vec[3], float out[3]) {
	out[0] = dot3(mat[0], vec);
	out[1] = dot3(mat[1], vec);
	out[2] = dot3(mat[2], vec);
	return out;
}

float (*genIdentityMat3(float mat[3][3]))[3] {
  mat[0][0] = 1.0;
  mat[0][1] = 0.0;
	mat[0][2] = 0.0;
  mat[1][0] = 0.0;
  mat[1][1] = 1.0;
	mat[1][2] = 0.0;
	mat[2][0] = 0.0;
	mat[2][1] = 0.0;
	mat[2][2] = 1.0;
  return mat;
}

float (*genTranslationMat3(float dx, float dy, float mat[3][3]))[3] {
	mat[0][0] = 1.0;
  mat[0][1] = 0.0;
	mat[0][2] = dx;
  mat[1][0] = 0.0;
  mat[1][1] = 1.0;
	mat[1][2] = dy;
	mat[2][0] = 0.0;
	mat[2][1] = 0.0;
	mat[2][2] = 1.0;
	return mat;
}

float (*genScaleMat3(float sx, float sy, float mat[3][3]))[3] {
  mat[0][0] = sx;
  mat[0][1] = 0.0;
	mat[0][2] = 0.0;
  mat[1][0] = 0.0;
  mat[1][1] = sy;
	mat[1][2] = 0.0;
	mat[2][0] = 0.0;
	mat[2][1] = 0.0;
	mat[2][2] = 1.0;
  return mat;
}

float (*genRotationMat3(float rotation, float mat[3][3]))[3] {
	mat[0][0] = cos(rotation);
	mat[0][1] = -sin(rotation);
	mat[0][2] = 0.0;
	mat[1][0] = sin(rotation);
	mat[1][1] = cos(rotation);
	mat[1][2] = 0.0;
	mat[2][0] = 0.0;
	mat[2][1] = 0.0;
	mat[2][2] = 1.0;
	return mat;
}
