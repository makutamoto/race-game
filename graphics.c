#include<stdio.h>
#include<stdint.h>
#include<math.h>
#include<windows.h>

#include "./include/matrix.h"
#include "./include/graphics.h"
#include "./include/bitmap.h"
#include "./include/colors.h"

extern char *buffer;
extern COORD bufferSizeCoord;

float transformation[3][3];

float matrixStore[64][3][3];
unsigned int currentStore = 0;
void pushTransformation() {
	matrixStore[currentStore][0][0] = transformation[0][0];
	matrixStore[currentStore][0][1] = transformation[0][1];
	matrixStore[currentStore][0][2] = transformation[0][2];
	matrixStore[currentStore][1][0] = transformation[1][0];
	matrixStore[currentStore][1][1] = transformation[1][1];
	matrixStore[currentStore][1][2] = transformation[1][2];
	matrixStore[currentStore][2][0] = transformation[2][0];
	matrixStore[currentStore][2][1] = transformation[2][1];
	matrixStore[currentStore][2][2] = transformation[2][2];
	currentStore += 1;
}

void popTransformation() {
	currentStore -= 1;
	transformation[0][0] = matrixStore[currentStore][0][0];
	transformation[0][1] = matrixStore[currentStore][0][1];
	transformation[0][2] = matrixStore[currentStore][0][2];
	transformation[1][0] = matrixStore[currentStore][1][0];
	transformation[1][1] = matrixStore[currentStore][1][1];
	transformation[1][2] = matrixStore[currentStore][1][2];
	transformation[2][0] = matrixStore[currentStore][2][0];
	transformation[2][1] = matrixStore[currentStore][2][1];
	transformation[2][2] = matrixStore[currentStore][2][2];
}

void clearTransformation() {
	genIdentityMat3(transformation);
}

void translateTransformation(float dx, float dy) {
	float temp1[3][3], temp2[3][3];
	memcpy_s(temp2, sizeof(temp2), transformation, sizeof(temp2));
	mulMat3(temp2, genTranslationMat3(dx, dy, temp1), transformation);
}

void scaleTransformation(float sx, float sy) {
	float temp1[3][3], temp2[3][3];
	memcpy_s(temp2, sizeof(temp2), transformation, sizeof(temp2));
	mulMat3(temp2, genScaleMat3(sx, sy, temp1), transformation);
}

void rotateTransformation(float rotation) {
	float temp1[3][3], temp2[3][3];
	memcpy_s(temp2, sizeof(temp2), transformation, sizeof(temp2));
	mulMat3(temp2, genRotationMat3(rotation, temp1), transformation);
}

void setPixelColor(int x, int y, char color) {
	if(x >= 0 && x < bufferSizeCoord.X && y >= 0 && y < bufferSizeCoord.Y) {
		buffer[bufferSizeCoord.X * y + x] = color;
	}
}

void fillBuffer(Image image, int shadow) {
	float halfWidth = image.width / 2.0;
	float halfHeight = image.height / 2.0;
	float leftTop[3] = { -halfWidth, -halfHeight, 1.0 };
	float leftBottom[3] = { -halfWidth, halfHeight, 1.0 };
	float rightTop[3] = { halfWidth, -halfHeight, 1.0 };
	float rightBottom[3] = { halfWidth, halfHeight, 1.0 };
	float transformedLeftTop[3];
	float transformedLeftBottom[3];
	float transformedRightTop[3];
	float transformedRightBottom[3];
	int maxCoord[2], minCoord[2];
	float axisX[2], axisY[2];
	float axisXLen, axisYLen;
	mulMat3Vec3(transformation, leftTop, transformedLeftTop);
	mulMat3Vec3(transformation, leftBottom, transformedLeftBottom);
	mulMat3Vec3(transformation, rightTop, transformedRightTop);
	mulMat3Vec3(transformation, rightBottom, transformedRightBottom);
	maxCoord[0] = min(max(max(transformedLeftTop[0], transformedLeftBottom[0]), max(transformedRightTop[0], transformedRightBottom[0])), bufferSizeCoord.X);
	maxCoord[1] = min(max(max(transformedLeftTop[1], transformedLeftBottom[1]), max(transformedRightTop[1], transformedRightBottom[1])), bufferSizeCoord.Y);
	minCoord[0] = max(min(min(transformedLeftTop[0], transformedLeftBottom[0]), min(transformedRightTop[0], transformedRightBottom[0])), 0);
	minCoord[1] = max(min(min(transformedLeftTop[1], transformedLeftBottom[1]), min(transformedRightTop[1], transformedRightBottom[1])), 0);
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
			float vector[2] = { x - transformedLeftTop[0], y - transformedLeftTop[1] };
			float dataCoords[2] = { dot2(vector, axisX) / (axisXLen * axisXLen), dot2(vector, axisY) / (axisYLen * axisYLen) };
			if(dataCoords[0] >= 0.0 && dataCoords[0] < 1.0 && dataCoords[1] >= 0.0 && dataCoords[1] < 1.0) {
				char color = image.data[image.width * min((unsigned int)round(image.height * dataCoords[1]), image.height - 1) + min((unsigned int)round(image.width * dataCoords[0]), image.width - 1)];
				if(color != image.transparent) {
					size_t index = bufferSizeCoord.X * y + x;
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

Image loadBitmap(char *fileName, char transparent) {
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
	if(fseek(file, header.offset, SEEK_SET)) {
		fprintf(stderr, "Image data does not exist.\n");
		return image;
	}
	img = (uint32_t*)malloc(infoHeader.imageSize);
	if(fread_s(img, infoHeader.imageSize, 1, infoHeader.imageSize, file) != infoHeader.imageSize) {
		fprintf(stderr, "Image data is corrupted.\n");
		return image;
	}
	image.width = infoHeader.width;
	image.height = infoHeader.height;
	image.transparent = transparent;
	image.data = (char*)malloc(image.width * image.height);
	int32_t y;
	for(y = 0;y < image.height;y++) {
		int32_t x;
		for(x = 0;x < image.width;x++) {
			size_t index = y * (int32_t)ceil(image.width / 8.0) + x / 8;
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

Image genRect(unsigned int width, unsigned int height, char color) {
	Image image;
	image.data = (char*)malloc(width * height);
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

Image genCircle(unsigned int radius, char color) {
	Image image;
	int edgeWidth = 2 * radius;
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
	int y;
	for(y = 0;y < edgeWidth;y++) {
		int x;
		for(x = 0;x < edgeWidth;x++) {
			int cx = x - radius;
			int cy = y - radius;
			size_t index = edgeWidth * y + x;
			if(radius * radius >= cx * cx + cy * cy) {
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
