#include<math.h>

#include "./include/matrix.h"

float (*copyMat4(float src[4][4], float dest[4][4]))[4] {
	int row, col;
	for(row = 0;row < 4;row++) {
		for(col = 0;col < 4;col++) dest[row][col] = src[row][col];
	}
	return dest;
}

float dot2(const float a[2], const float b[2]) {
	return a[0] * b[0] + a[1] * b[1];
}

float dot3(const float a[3], const float b[3]) {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

float dot4(const float a[4], const float b[4]) {
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2] + a[3] * b[3];
}

float length2(const float vector[2]) {
	return sqrtf(vector[0] * vector[0] + vector[1] * vector[1]);
}

float distance2(const float a[2], const float b[2]) {
	float position[2] = { b[0] - a[0], b[1] - a[1] };
	return length2(position);
}

float* addVec2(const float a[2], const float b[2], float out[2]) {
	out[0] = a[0] + b[0];
	out[1] = a[1] + b[1];
	return out;
}

float* subVec2(const float a[2], const float b[2], float out[2]) {
	out[0] = a[0] - b[0];
	out[1] = a[1] - b[1];
	return out;
}

float* mulVec2ByScalar(const float vector[2], float scalar, float out[2]) {
	out[0] = scalar * vector[0];
	out[1] = scalar * vector[1];
	return out;
}

float* normalize2(const float vector[2], float out[2]) {
	float length = length2(vector);
	if(length == 0.0F) {
		out[0] = 0.0F;
		out[1] = 0.0F;
	} else {
		out[0] = vector[0] / length;
		out[1] = vector[1] / length;
	}
	return out;
}

float* direction2(const float a[2], const float b[2], float out[2]) {
	float temp[2];
	subVec2(a, b, temp);
	normalize2(temp, out);
	return out;
}

float angleVec2(const float vector[2]) {
	float result;
	if(vector[0] == 0.0F) {
		result = (vector[1] == 0.0F || vector[1] < 0.0F) ? 3.0F * PI / 2.0F : PI / 2.0F;
	} else {
		if(vector[1] == 0.0F) {
			result = (vector[0] > 0.0F) ? 0.0F : PI;
		} else {
			result = atanf(vector[1] / vector[0]);
			if(vector[0] < 0.0F) {
				result += PI;
			} else {
				if(result < 0.0F) result += 2.0F * PI;
			}
		}
	}
	return result;
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

float	(*mulMat4(const float a[4][4], const float b[4][4], float out[4][4]))[4] {
	float b0[4] = { b[0][0], b[1][0], b[2][0], b[3][0] };
	float b1[4] = { b[0][1], b[1][1], b[2][1], b[3][1] };
	float b2[4] = { b[0][2], b[1][2], b[2][2], b[3][2] };
	float b3[4] = { b[0][3], b[1][3], b[2][3], b[3][3] };
	out[0][0] = dot4(a[0], b0);
	out[0][1] = dot4(a[0], b1);
	out[0][2] = dot4(a[0], b2);
	out[0][3] = dot4(a[0], b3);
	out[1][0] = dot4(a[1], b0);
	out[1][1] = dot4(a[1], b1);
	out[1][2] = dot4(a[1], b2);
	out[1][3] = dot4(a[1], b3);
	out[2][0] = dot4(a[2], b0);
	out[2][1] = dot4(a[2], b1);
	out[2][2] = dot4(a[2], b2);
	out[2][3] = dot4(a[2], b3);
	out[3][0] = dot4(a[3], b0);
	out[3][1] = dot4(a[3], b1);
	out[3][2] = dot4(a[3], b2);
	out[3][3] = dot4(a[3], b3);
	return out;
}

float* mulMat3Vec3(const float mat[3][3], const float vec[3], float out[3]) {
	out[0] = dot3(mat[0], vec);
	out[1] = dot3(mat[1], vec);
	out[2] = dot3(mat[2], vec);
	return out;
}

float* mulMat4Vec4(const float mat[4][4], const float vec[4], float out[4]) {
	out[0] = dot4(mat[0], vec);
	out[1] = dot4(mat[1], vec);
	out[2] = dot4(mat[2], vec);
	out[3] = dot4(mat[3], vec);
	return out;
}

float (*transposeMat3(const float mat[3][3], float out[3][3]))[3] {
	out[0][0] = mat[0][0];
	out[1][0] = mat[0][1];
	out[2][0] = mat[0][2];
	out[0][1] = mat[1][0];
	out[1][1] = mat[1][1];
	out[2][1] = mat[1][2];
	out[0][2] = mat[2][0];
	out[1][2] = mat[2][1];
	out[2][2] = mat[2][2];
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

float (*genIdentityMat4(float mat[4][4]))[4] {
  mat[0][0] = 1.0;
  mat[0][1] = 0.0;
	mat[0][2] = 0.0;
	mat[0][3] = 0.0;
  mat[1][0] = 0.0;
  mat[1][1] = 1.0;
	mat[1][2] = 0.0;
	mat[1][3] = 0.0;
	mat[2][0] = 0.0;
	mat[2][1] = 0.0;
	mat[2][2] = 1.0;
	mat[2][3] = 0.0;
	mat[3][0] = 0.0;
	mat[3][1] = 0.0;
	mat[3][2] = 0.0;
	mat[3][3] = 1.0;
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

float (*genTranslationMat4(float dx, float dy, float dz, float mat[4][4]))[4] {
	mat[0][0] = 1.0;
  mat[0][1] = 0.0;
	mat[0][2] = 0.0;
	mat[0][3] = dx;
  mat[1][0] = 0.0;
  mat[1][1] = 1.0;
	mat[1][2] = 0.0;
	mat[1][3] = dy;
	mat[2][0] = 0.0;
	mat[2][1] = 0.0;
	mat[2][2] = 1.0;
	mat[2][3] = dz;
	mat[3][0] = 0.0;
	mat[3][1] = 0.0;
	mat[3][2] = 0.0;
	mat[3][3] = 1.0;
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

float (*genScaleMat4(float sx, float sy, float sz, float mat[4][4]))[4] {
  mat[0][0] = sx;
  mat[0][1] = 0.0;
	mat[0][2] = 0.0;
	mat[0][3] = 0.0;
  mat[1][0] = 0.0;
  mat[1][1] = sy;
	mat[1][2] = 0.0;
	mat[1][3] = 0.0;
	mat[2][0] = 0.0;
	mat[2][1] = 0.0;
	mat[2][2] = sz;
	mat[2][3] = 0.0;
	mat[3][0] = 0.0;
	mat[3][1] = 0.0;
	mat[3][2] = 0.0;
	mat[3][3] = 1.0;
  return mat;
}

float (*genRotationMat3(float rotation, float mat[3][3]))[3] {
	mat[0][0] = cosf(rotation);
	mat[0][1] = -sinf(rotation);
	mat[0][2] = 0.0;
	mat[1][0] = sinf(rotation);
	mat[1][1] = cosf(rotation);
	mat[1][2] = 0.0;
	mat[2][0] = 0.0;
	mat[2][1] = 0.0;
	mat[2][2] = 1.0;
	return mat;
}

float (*genRotationXMat4(float rotation, float mat[4][4]))[4] {
	mat[0][0] = 1.0;
	mat[0][1] = 0.0;
	mat[0][2] = 0.0;
	mat[0][3] = 0.0;
	mat[1][0] = 0.0;
	mat[1][1] = cosf(rotation);
	mat[1][2] = -sinf(rotation);
	mat[1][3] = 0.0;
	mat[2][0] = 0.0;
	mat[2][1] = sinf(rotation);
	mat[2][2] = cosf(rotation);
	mat[2][3] = 0.0;
	mat[3][0] = 0.0;
	mat[3][1] = 0.0;
	mat[3][2] = 0.0;
	mat[3][3] = 1.0;
	return mat;
}

float (*genRotationYMat4(float rotation, float mat[4][4]))[4] {
	mat[0][0] = cosf(rotation);
	mat[0][1] = 0.0;
	mat[0][2] = sinf(rotation);
	mat[0][3] = 0.0;
	mat[1][0] = 0.0;
	mat[1][1] = 1.0;
	mat[1][2] = 0.0;
	mat[1][3] = 0.0;
	mat[2][0] = -sinf(rotation);
	mat[2][1] = 0.0;
	mat[2][2] = cosf(rotation);
	mat[2][3] = 0.0;
	mat[3][0] = 0.0;
	mat[3][1] = 0.0;
	mat[3][2] = 0.0;
	mat[3][3] = 1.0;
	return mat;
}

float (*genRotationZMat4(float rotation, float mat[4][4]))[4] {
	mat[0][0] = cosf(rotation);
	mat[0][1] = -sinf(rotation);
	mat[0][2] = 0.0;
	mat[0][3] = 0.0;
	mat[1][0] = sinf(rotation);
	mat[1][1] = cosf(rotation);
	mat[1][2] = 0.0;
	mat[1][3] = 0.0;
	mat[2][0] = 0.0;
	mat[2][1] = 0.0;
	mat[2][2] = 1.0;
	mat[2][3] = 0.0;
	mat[3][0] = 0.0;
	mat[3][1] = 0.0;
	mat[3][2] = 0.0;
	mat[3][3] = 1.0;
	return mat;
}

float (*genRotationMat4(float rx, float ry, float rz, float mat[4][4]))[4] {
	float xMat[4][4];
	float yMat[4][4];
	float zMat[4][4];
	float temp[4][4];
	genRotationXMat4(rx, xMat);
	genRotationYMat4(ry, yMat);
	genRotationZMat4(rz, zMat);
	return mulMat4(mulMat4(zMat, yMat, temp), xMat, mat);
}
