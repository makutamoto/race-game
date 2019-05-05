#include<math.h>

#include "./include/matrix.h"

float dot2(const float a[2], const float b[2]) {
	return a[0] * b[0] + a[1] * b[1];
}

float length(const float vector[2]) {
	return sqrt(vector[0] * vector[0] + vector[1] * vector[1]);
}

float	(*mulMat2(const float a[2][2], const float b[2][2], float out[2][2]))[2] {
	float b0[3] = { b[0][0], b[1][0] };
	float b1[3] = { b[0][1], b[1][1] };
	out[0][0] = dot2(a[0], b0);
	out[0][1] = dot2(a[0], b1);
	out[1][0] = dot2(a[1], b0);
	out[1][1] = dot2(a[1], b1);
	return out;
}

float* mulMat2Vec2(const float mat[2][2], const float vec[2], float out[2]) {
	out[0] = dot2(mat[0], vec);
	out[1] = dot2(mat[1], vec);
	return out;
}

float (*genIdentityMat2(float mat[2][2]))[2] {
  mat[0][0] = 1.0;
  mat[0][1] = 0.0;
  mat[1][0] = 0.0;
  mat[1][1] = 1.0;
  return mat;
}

float (*genScaleMat2(float sx, float sy, float mat[2][2]))[2] {
  mat[0][0] = sx;
  mat[0][1] = 0.0;
  mat[1][0] = 0.0;
  mat[1][1] = sy;
  return mat;
}

float (*genRotationMat2(float rotation, float mat[2][2]))[2] {
	mat[0][0] = cos(rotation);
	mat[0][1] = -sin(rotation);
	mat[1][0] = sin(rotation);
	mat[1][1] = cos(rotation);
	return mat;
}
