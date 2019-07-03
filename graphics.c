#include<stdio.h>
#include<stdint.h>
#include<math.h>
#include<Windows.h>

#include "./include/matrix.h"
#include "./include/graphics.h"
#include "./include/bitmap.h"
#include "./include/colors.h"
#include "./include/vector.h"

static float transformation[4][4];

static Vector matrixStore;
static unsigned int currentStore = 0;
void pushTransformation(void) {
	float *store = malloc(16 * sizeof(float));
	memcpy_s(store, sizeof(transformation), transformation, sizeof(transformation));
	push(&matrixStore, store);
	currentStore += 1;
}

void popTransformation(void) {
	float *store;
	currentStore -= 1;
	store = (float*)pop(&matrixStore);
	memcpy_s(transformation, sizeof(transformation), store, sizeof(transformation));
	free(store);
}

void clearTransformation(void) {
	genIdentityMat4(transformation);
}

void mulTransformationL(float mat[4][4]) {
	float temp[4][4];
	memcpy_s(temp, sizeof(temp), transformation, sizeof(transformation));
	mulMat4(mat, temp, transformation);
}

void translateTransformation(float dx, float dy, float dz) {
	float temp1[4][4], temp2[4][4];
	memcpy_s(temp2, sizeof(temp2), transformation, sizeof(transformation));
	mulMat4(temp2, genTranslationMat4(dx, dy, dz, temp1), transformation);
}

void scaleTransformation(float sx, float sy, float sz) {
	float temp1[4][4], temp2[4][4];
	memcpy_s(temp2, sizeof(temp2), transformation, sizeof(transformation));
	mulMat4(temp2, genScaleMat4(sx, sy, sz, temp1), transformation);
}

void rotateTransformation(float rx, float ry, float rz) {
	float temp1[4][4], temp2[4][4];
	memcpy_s(temp2, sizeof(temp2), transformation, sizeof(transformation));
	mulMat4(temp2, genRotationMat4(rx, ry, rz, temp1), transformation);
}

void setBuffer(unsigned char color) {
	for(size_t i = 0;i < bufferLength;i++) buffer[i] = color;
}

void fillBuffer(Image image, int shadow) {
	float halfScreenWidth = bufferSizeCoord.X / 2.0F;
	float halfScreenHeight = bufferSizeCoord.Y / 2.0F;
	float halfWidth = image.width / 2.0F;
	float halfHeight = image.height / 2.0F;
	float leftTop[4] = { -halfWidth, -halfHeight, 0.0F, 1.0F };
	float leftBottom[4] = { -halfWidth, halfHeight, 0.0F, 1.0F };
	float rightTop[4] = { halfWidth, -halfHeight, 0.0F, 1.0F };
	float rightBottom[4] = { halfWidth, halfHeight, 0.0F, 1.0F };
	float transformedLeftTop[4];
	float transformedLeftBottom[4];
	float transformedRightTop[4];
	float transformedRightBottom[4];
	unsigned int maxCoord[2], minCoord[2];
	float axisX[2], axisY[2];
	float axisXLen, axisYLen;
	mulMat4Vec4(transformation, leftTop, transformedLeftTop);
	mulMat4Vec4(transformation, leftBottom, transformedLeftBottom);
	mulMat4Vec4(transformation, rightTop, transformedRightTop);
	mulMat4Vec4(transformation, rightBottom, transformedRightBottom);
	transformedLeftTop[0] = (transformedLeftTop[0] / transformedLeftTop[3]) * halfScreenWidth + halfScreenWidth;
	transformedLeftTop[1] = (transformedLeftTop[1] / transformedLeftTop[3]) * halfScreenHeight + halfScreenHeight;
	transformedLeftBottom[0] = (transformedLeftBottom[0] / transformedLeftBottom[3]) * halfScreenWidth + halfScreenWidth;
	transformedLeftBottom[1] = (transformedLeftBottom[1] / transformedLeftBottom[3]) * halfScreenHeight + halfScreenHeight;
	transformedRightTop[0] = (transformedRightTop[0] / transformedRightTop[3]) * halfScreenWidth + halfScreenWidth;
	transformedRightTop[1] = (transformedRightTop[1] / transformedRightTop[3])  * halfScreenHeight + halfScreenHeight;
	transformedRightBottom[0] = (transformedRightBottom[0] / transformedRightBottom[3]) * halfScreenWidth + halfScreenWidth;
	transformedRightBottom[1] = (transformedRightBottom[1] / transformedRightBottom[3]) * halfScreenHeight + halfScreenHeight;
	maxCoord[0] = (unsigned int)max(min(max(max(transformedLeftTop[0], transformedLeftBottom[0]), max(transformedRightTop[0], transformedRightBottom[0])), bufferSizeCoord.X), 0);
	maxCoord[1] = (unsigned int)max(min(max(max(transformedLeftTop[1], transformedLeftBottom[1]), max(transformedRightTop[1], transformedRightBottom[1])), bufferSizeCoord.Y), 0);
	minCoord[0] = (unsigned int)max(min(min(transformedLeftTop[0], transformedLeftBottom[0]), min(transformedRightTop[0], transformedRightBottom[0])), 0);
	minCoord[1] = (unsigned int)max(min(min(transformedLeftTop[1], transformedLeftBottom[1]), min(transformedRightTop[1], transformedRightBottom[1])), 0);
	axisX[0] = transformedRightTop[0] - transformedLeftTop[0];
	axisX[1] = transformedRightTop[1] - transformedLeftTop[1];
	axisY[0] = transformedLeftBottom[0] - transformedLeftTop[0];
	axisY[1] = transformedLeftBottom[1] - transformedLeftTop[1];
	axisXLen = length2(axisX);
	axisYLen = length2(axisY);
	unsigned int y;
	for(y = minCoord[1];y < maxCoord[1];y++) {
		unsigned int x;
		for(x = minCoord[0];x < maxCoord[0];x++) {
			float vector[2] = { (int)x - transformedLeftTop[0], (int)y - transformedLeftTop[1] };
			float dataCoords[2] = { dot2(vector, axisX) / (axisXLen * axisXLen), dot2(vector, axisY) / (axisYLen * axisYLen) };
			if(dataCoords[0] >= 0.0F && dataCoords[0] < 1.0F && dataCoords[1] >= 0.0F && dataCoords[1] < 1.0F) {
				unsigned char color = image.data[image.width * min((unsigned int)(roundf(image.height * dataCoords[1])), image.height - 1) + min((unsigned int)(roundf(image.width * dataCoords[0])), image.width - 1)];
				if(color != image.transparent) {
					size_t index = (size_t)bufferSizeCoord.X * y + x;
					if(shadow) {
						if((buffer[index] >> 3) & 1) {
							color = buffer[index] ^ 0x0E;
						} else {
							color = BLACK;
						}
					}
					buffer[index] = color;
				}
			}
		}
	}
}

Image loadBitmap(char *fileName, unsigned char transparent) {
	Image image = { 0, 0, NULL_COLOR, NULL };
	FILE *file;
	BitmapHeader header;
	BitmapInfoHeader infoHeader;
	uint32_t *img;
	if(fopen_s(&file, fileName, "rb")) {
		fprintf(stderr, "Failed to open the file '%s'\n", fileName);
		return image;
	}
	if(fread_s(&header, sizeof(BitmapHeader), 1, sizeof(BitmapHeader), file) != sizeof(BitmapHeader)) {
		fprintf(stderr, "BMP header does not exist in the file '%s'\n", fileName);
		return image;
	}
	if(header.magicNumber[0] != 'B' || header.magicNumber[1] != 'M') {
		fprintf(stderr, "Unknown magic number.\n");
		return image;
	}
	if(header.dibSize != 40) {
		fprintf(stderr, "Unknown DIB header type.\n");
		return  image;
	}
	if(fread_s(&infoHeader, sizeof(BitmapInfoHeader), 1, sizeof(BitmapInfoHeader), file) != sizeof(BitmapInfoHeader)) {
		fprintf(stderr, "Failed to load Bitmap info header.\n");
		return image;
	}
	if(infoHeader.width < 0 || infoHeader.height < 0) {
		fprintf(stderr, "Negative demensions are not supported.\n");
		return image;
	}
	if(infoHeader.compressionMethod != 0) {
		fprintf(stderr, "Compressions are not supported.\n");
		return image;
	}
	if(fseek(file, (long)header.offset, SEEK_SET)) {
		fprintf(stderr, "Image data does not exist.\n");
		return image;
	}
	img = (uint32_t*)malloc(infoHeader.imageSize);
	if(fread_s(img, infoHeader.imageSize, 1, infoHeader.imageSize, file) != infoHeader.imageSize) {
		fprintf(stderr, "Image data is corrupted.\n");
		return image;
	}
	image.width = (unsigned int)infoHeader.width;
	image.height = (unsigned int)infoHeader.height;
	image.transparent = transparent;
	image.data = (unsigned char*)malloc(image.width * image.height);
	unsigned int y;
	for(y = 0;y < image.height;y++) {
		unsigned int x;
		for(x = 0;x < image.width;x++) {
			size_t index = y * (unsigned int)(ceilf(image.width / 8.0F)) + x / 8;
			uint8_t location = x % 8;
			uint8_t highLow = location % 2;
			uint8_t color;
			if(highLow) {
				color = (img[index] >> ((location - 1) * 4)) & 0x0F;
			} else {
				color = (img[index] >> ((location + 1) * 4)) & 0x0F;
			}
			image.data[image.width * (image.height - 1 - y) + x] = color;
		}
	}
	fclose(file);
	return image;
}

Image genRect(unsigned int width, unsigned int height, unsigned char color) {
	Image image;
	image.data = (unsigned char*)malloc(width * height);
	if(image.data == NULL) {
		image.width = 0;
		image.height = 0;
		image.transparent = NULL_COLOR;
		image.data = NULL;
		return image;
	}
	image.width = width;
	image.height = height;
	image.transparent = NULL_COLOR;
  unsigned int y;
	for(y = 0;y < image.height;y++) {
    unsigned int x;
    for(x = 0;x < image.width;x++) image.data[image.width * y + x] = color;
	}
	return image;
}

Image genCircle(unsigned int radius, unsigned char color) {
	Image image;
	unsigned int edgeWidth = 2 * radius;
	image.data = malloc(edgeWidth * edgeWidth);
	if(image.data == NULL) {
		image.width = 0;
		image.height = 0;
		image.transparent = NULL_COLOR;
		image.data = NULL;
		return image;
	}
	image.width = edgeWidth;
	image.height = edgeWidth;
	image.transparent = color ^ 0x0F;
	unsigned int y;
	for(y = 0;y < edgeWidth;y++) {
		unsigned int x;
		for(x = 0;x < edgeWidth;x++) {
			int cx = (int)x - (int)radius;
			int cy = (int)y - (int)radius;
			size_t index = edgeWidth * y + x;
			if(radius * radius >= (unsigned int)(cx * cx + cy * cy)) {
				image.data[index] = color;
			} else {
				image.data[index] = image.transparent;
			}
		}
	}
	return image;
}

void freeImage(Image image) {
	free(image.data);
}
