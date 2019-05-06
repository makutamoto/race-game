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

float transformation[2][2];

float matrixStore[64][2][2];
unsigned int currentStore = 0;
void pushTransformation() {
	matrixStore[currentStore][0][0] = transformation[0][0];
	matrixStore[currentStore][0][1] = transformation[0][1];
	matrixStore[currentStore][1][0] = transformation[1][0];
	matrixStore[currentStore][1][1] = transformation[1][1];
	currentStore += 1;
}

void popTransformation() {
	currentStore -= 1;
	transformation[0][0] = matrixStore[currentStore][0][0];
	transformation[0][1] = matrixStore[currentStore][0][1];
	transformation[1][0] = matrixStore[currentStore][1][0];
	transformation[1][1] = matrixStore[currentStore][1][1];
}

void clearTransformation() {
	genIdentityMat2(transformation);
}

void scaleTransformation(float sx, float sy) {
	float temp1[2][2], temp2[2][2];
	memcpy_s(temp2, sizeof(temp2), transformation, sizeof(temp2));
	mulMat2(genScaleMat2(sx, sy, temp1), temp2, transformation);
}

void rotateTransformation(float rotation) {
	float temp1[2][2], temp2[2][2];
	memcpy_s(temp2, sizeof(temp2), transformation, sizeof(temp2));
	mulMat2(genRotationMat2(rotation, temp1), temp2, transformation);
}

void setPixelColor(int x, int y, char color) {
	if(x >= 0 && x < bufferSizeCoord.X && y >= 0 && y < bufferSizeCoord.Y) {
		buffer[bufferSizeCoord.X * y + x] = color;
	}
}

void fillBuffer(int ox, int oy, unsigned int width, unsigned int height, char *data, char transparent, int shadow) {
	float halfWidth = width / 2.0;
	float halfHeight = height / 2.0;
	float leftTop[2] = { -halfWidth, -halfHeight };
	float leftBottom[2] = { -halfWidth, halfHeight };
	float rightTop[2] = { halfWidth, -halfHeight };
	float rightBottom[2] = { halfWidth, halfHeight };
	float transformedLeftTop[2];
	float transformedLeftBottom[2];
	float transformedRightTop[2];
	float transformedRightBottom[2];
	int maxCoord[2], minCoord[2];
	float axisX[2], axisY[2];
	float axisXLen, axisYLen;
	mulMat2Vec2(transformation, leftTop, transformedLeftTop);
	mulMat2Vec2(transformation, leftBottom, transformedLeftBottom);
	mulMat2Vec2(transformation, rightTop, transformedRightTop);
	mulMat2Vec2(transformation, rightBottom, transformedRightBottom);
	transformedLeftTop[0] += ox;
	transformedLeftTop[1] += oy;
	transformedLeftBottom[0] += ox;
	transformedLeftBottom[1] += oy;
	transformedRightTop[0] += ox;
	transformedRightTop[1] += oy;
	transformedRightBottom[0] += ox;
	transformedRightBottom[1] += oy;
	maxCoord[0] = min(max(max(transformedLeftTop[0], transformedLeftBottom[0]), max(transformedRightTop[0], transformedRightBottom[0])), bufferSizeCoord.X);
	maxCoord[1] = min(max(max(transformedLeftTop[1], transformedLeftBottom[1]), max(transformedRightTop[1], transformedRightBottom[1])), bufferSizeCoord.Y);
	minCoord[0] = max(min(min(transformedLeftTop[0], transformedLeftBottom[0]), min(transformedRightTop[0], transformedRightBottom[0])), 0);
	minCoord[1] = max(min(min(transformedLeftTop[1], transformedLeftBottom[1]), min(transformedRightTop[1], transformedRightBottom[1])), 0);
	axisX[0] = transformedRightTop[0] - transformedLeftTop[0];
	axisX[1] = transformedRightTop[1] - transformedLeftTop[1];
	axisY[0] = transformedLeftBottom[0] - transformedLeftTop[0];
	axisY[1] = transformedLeftBottom[1] - transformedLeftTop[1];
	axisXLen = length(axisX);
	axisYLen = length(axisY);
	unsigned int y;
	for(y = minCoord[1];y < maxCoord[1];y++) {
		unsigned int x;
		for(x = minCoord[0];x < maxCoord[0];x++) {
			float vector[2] = { x - transformedLeftTop[0], y - transformedLeftTop[1] };
			float dataCoords[2] = { dot2(vector, axisX) / (axisXLen * axisXLen), dot2(vector, axisY) / (axisYLen * axisYLen) };
			if(dataCoords[0] >= 0.0 && dataCoords[0] < 1.0 && dataCoords[1] >= 0.0 && dataCoords[1] < 1.0) {
				char color = data[width * min((unsigned int)round(height * dataCoords[1]), height - 1) + min((unsigned int)round(width * dataCoords[0]), width - 1)];
				if(color != transparent) {
					size_t index = bufferSizeCoord.X * y + x;
					if(shadow) {
						if((buffer[index] >> 3) & 1) {
							color = buffer[index] ^ 0x0F;
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

BOOL loadBitmap(char *fileName, Image *image, char transparent) {
	FILE *file;
	BitmapHeader header;
	BitmapInfoHeader infoHeader;
	uint32_t *img;
	char *result;
	if(fopen_s(&file, fileName, "rb")) {
		fprintf(stderr, "Failed to open the file '%s'\n", fileName);
		return FALSE;
	}
	if(fread_s(&header, sizeof(BitmapHeader), 1, sizeof(BitmapHeader), file) != sizeof(BitmapHeader)) {
		fprintf(stderr, "BMP header does not exist in the file '%s'\n", fileName);
		return FALSE;
	}
	if(header.magicNumber[0] != 'B' || header.magicNumber[1] != 'M') {
		fprintf(stderr, "Unknown magic number.\n");
		return FALSE;
	}
	if(header.dibSize != 40) {
		fprintf(stderr, "Unknown DIB header type.\n");
		return  FALSE;
	}
	if(fread_s(&infoHeader, sizeof(BitmapInfoHeader), 1, sizeof(BitmapInfoHeader), file) != sizeof(BitmapInfoHeader)) {
		fprintf(stderr, "Failed to load Bitmap info header.\n");
		return FALSE;
	}
	if(infoHeader.width < 0 || infoHeader.height < 0) {
		fprintf(stderr, "Negative demensions are not supported.\n");
		return FALSE;
	}
	if(infoHeader.compressionMethod != 0) {
		fprintf(stderr, "Compressions are not supported.\n");
		return FALSE;
	}
	if(fseek(file, header.offset, SEEK_SET)) {
		fprintf(stderr, "Image data does not exist.\n");
		return FALSE;
	}
	img = (uint32_t*)malloc(infoHeader.imageSize);
	if(fread_s(img, infoHeader.imageSize, 1, infoHeader.imageSize, file) != infoHeader.imageSize) {
		fprintf(stderr, "Image data is corrupted.\n");
		return FALSE;
	}
	image->width = infoHeader.width;
	image->height = infoHeader.height;
	image->transparent = transparent;
	result = (char*)malloc(image->width * image->height);
	int32_t y;
	for(y = 0;y < image->height;y++) {
		int32_t x;
		for(x = 0;x < image->width;x++) {
			size_t index = y * (int32_t)ceil(image->width / 8.0) + x / 8;
			uint8_t location = x % 8;
			uint8_t highLow = location % 2;
			uint8_t color;
			if(highLow) {
				color = (img[index] >> ((location - 1) * 4)) & 0x0F;
			} else {
				color = (img[index] >> ((location + 1) * 4)) & 0x0F;
			}
			result[image->width * (image->height - 1 - y) + x] = color;
		}
	}
	image->data = result;
	fclose(file);
	return TRUE;
}

void discardImage(Image image) {
	free(image.data);
}

void drawImage(int ox, int oy, Image image, char transparent) {
	unsigned int y;
	for(y = 0;y < image.height;y++) {
		unsigned int x;
		for(x = 0;x < image.width;x++) {
			char color = image.data[image.width * y + x];
			if(color != transparent) setPixelColor(ox + x, oy + y, color);
		}
	}
}

void drawRect(int ox, int oy, unsigned int width, unsigned int height, char color) {
  unsigned int y;
	for(y = 0;y < height;y++) {
    unsigned int x;
    for(x = 0;x < width;x++) setPixelColor(x + ox, y + oy, color);
	}
}

void drawCircle(int ox, int oy, unsigned int radius, char color) {
	int y;
	for(y = 0;y < 2 * radius;y++) {
		int x;
		for(x = 0;x < 2 * radius;x++) {
			int cx = x - radius;
			int cy = y - radius;
			if(radius * radius >= cx * cx + cy * cy) setPixelColor(x + ox, y + oy, color);
		}
	}
}
