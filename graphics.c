#include<stdio.h>
#include<float.h>
#include<math.h>
#include<Windows.h>

#ifndef __BORLANDC__
#include<stdint.h>
#endif

#include "./include/borland.h"
#include "./include/matrix.h"
#include "./include/graphics.h"
#include "./include/bitmap.h"
#include "./include/colors.h"
#include "./include/vector.h"

static unsigned char *buffer;
static float *zBuffer;
static unsigned int bufferSize[2];
static unsigned long bufferLength;
static unsigned long zBufferLength;
static unsigned int screenSize[2];
static unsigned long screenLength;
static unsigned int halfScreenSize[2];

static float camera[4][4];
static float transformation[4][4];
static Vector matrixStore;
static unsigned int currentStore = 0;

int aabbClear = TRUE;
static float aabbTemp[3][2];

void initGraphics(unsigned int width, unsigned int height) {
	bufferSize[0] = 2 * width;
	bufferSize[1] = height;
	bufferLength = bufferSize[0] * bufferSize[1];
	zBufferLength = sizeof(float) * bufferLength;
	if(buffer) free(buffer);
	if(zBuffer) free(zBuffer);
	buffer = malloc(bufferLength);
	zBuffer = malloc(zBufferLength);
	screenSize[0] = width;
	screenSize[1] = height;
	screenLength = screenSize[0] * screenSize[1];
	halfScreenSize[0] = screenSize[0] / 2;
	halfScreenSize[1] = screenSize[1] / 2;
}

void deinitGraphics(void) {
	if(buffer) free(buffer);
}

void clearBuffer(unsigned char color) {
	memset(buffer, color, bufferLength);
}

void clearZBuffer(void) {
	unsigned long i;
	for(i = 0;i < bufferLength;i++) zBuffer[i] = FLT_MAX;
}

void flushBuffer(HANDLE screen) {
	DWORD nofWritten;
	static COORD cursor = { 0, 0 };
	WORD *data = (WORD*)malloc(bufferLength * sizeof(WORD));
	size_t index;
	for(index = 0;index < screenLength;index++) {
		WORD attribute = (WORD)(BACKGROUND_BLUE - 1 + (((buffer[index] & 8) | ((buffer[index] & 1) << 2) | (buffer[index] & 2) | ((buffer[index] & 4) >> 2)) << 4));
		data[2 * index] = attribute;
		data[2 * index + 1] = attribute;
	}
	WriteConsoleOutputAttribute(screen, data, bufferLength, cursor, &nofWritten);
	free(data);
}

Vertex initVertex(float x, float y, float z, unsigned char color) {
	Vertex vertex;
	vertex.components[0] = x;
	vertex.components[1] = y;
	vertex.components[2] = z;
	vertex.components[3] = 1.0F;
	vertex.color = color;

	return vertex;
}

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

void setCameraMat4(float mat[4][4]) {
	memcpy_s(camera, sizeof(camera), mat, sizeof(camera));
}

void clearCameraMat4(void) {
	genIdentityMat4(camera);
}

void translateTransformation(float dx, float dy, float dz) {
	float temp1[4][4], temp2[4][4];
	memcpy_s(temp2, sizeof(transformation), transformation, sizeof(transformation));
	mulMat4(temp2, genTranslationMat4(dx, dy, dz, temp1), transformation);
}

void scaleTransformation(float sx, float sy, float sz) {
	float temp1[4][4], temp2[4][4];
	memcpy_s(temp2, sizeof(transformation), transformation, sizeof(transformation));
	mulMat4(temp2, genScaleMat4(sx, sy, sz, temp1), transformation);
}

void rotateTransformation(float rx, float ry, float rz) {
	float temp1[4][4], temp2[4][4];
	memcpy_s(temp2, sizeof(transformation), transformation, sizeof(transformation));
	mulMat4(temp2, genRotationMat4(rx, ry, rz, temp1), transformation);
}

void clearAABB(void) {
	aabbClear = TRUE;
}

float (*getAABB(float out[3][2]))[2] {
	out[0][0] = aabbTemp[0][0];
	out[0][1] = aabbTemp[0][1];
	out[1][0] = aabbTemp[1][0];
	out[1][1] = aabbTemp[1][1];
	out[2][0] = aabbTemp[2][0];
	out[2][1] = aabbTemp[2][1];

	return out;
}

static float edgeFunction(float x, float y, const float a[2], const float b[2]) {
	return (a[0] - b[0]) * (y - a[1]) - (a[1] - b[1]) * (x - a[0]);
}

void fillTriangle(Vertex vertices[3], Image *image, float *uv[3]) {
	float transformed[3][4], transformedTemp[3][4];
	float textures[3][2];
	float vertexColors[3];
	unsigned int maxCoord[2], minCoord[2];
	float area;
	float aspect = (float)screenSize[0] / screenSize[1];
	unsigned int y, x;
	mulMat4Vec4(transformation, vertices[0].components, transformedTemp[0]);
	mulMat4Vec4(transformation, vertices[1].components, transformedTemp[1]);
	mulMat4Vec4(transformation, vertices[2].components, transformedTemp[2]);
	// printVec4(vertices[0].components);
	// printVec4(vertices[1].components);
	// printVec4(vertices[2].components);
	// printVec4(transformedTemp[0]);
	// printVec4(transformedTemp[1]);
	// printVec4(transformedTemp[2]);
	// puts("");
	if(aabbClear) {
		aabbTemp[0][0] = transformedTemp[0][0];
		aabbTemp[0][1] = transformedTemp[0][0];
		aabbTemp[1][0] = transformedTemp[0][1];
		aabbTemp[1][1] = transformedTemp[0][1];
		aabbTemp[2][0] = transformedTemp[0][2];
		aabbTemp[2][1] = transformedTemp[0][2];
		aabbClear = FALSE;
	} else {
		if(aabbTemp[0][0] > transformedTemp[0][0]) aabbTemp[0][0] = transformedTemp[0][0];
		if(aabbTemp[0][1] < transformedTemp[0][0]) aabbTemp[0][1] = transformedTemp[0][0];
		if(aabbTemp[1][0] > transformedTemp[0][1]) aabbTemp[1][0] = transformedTemp[0][1];
		if(aabbTemp[1][1] < transformedTemp[0][1]) aabbTemp[1][1] = transformedTemp[0][1];
		if(aabbTemp[2][0] > transformedTemp[0][2]) aabbTemp[2][0] = transformedTemp[0][2];
		if(aabbTemp[2][1] < transformedTemp[0][2]) aabbTemp[2][1] = transformedTemp[0][2];
 	}
	if(aabbTemp[0][0] > transformedTemp[1][0]) aabbTemp[0][0] = transformedTemp[1][0];
	if(aabbTemp[0][1] < transformedTemp[1][0]) aabbTemp[0][1] = transformedTemp[1][0];
	if(aabbTemp[1][0] > transformedTemp[1][1]) aabbTemp[1][0] = transformedTemp[1][1];
	if(aabbTemp[1][1] < transformedTemp[1][1]) aabbTemp[1][1] = transformedTemp[1][1];
	if(aabbTemp[2][0] > transformedTemp[1][2]) aabbTemp[2][0] = transformedTemp[1][2];
	if(aabbTemp[2][1] < transformedTemp[1][2]) aabbTemp[2][1] = transformedTemp[1][2];
	if(aabbTemp[0][0] > transformedTemp[2][0]) aabbTemp[0][0] = transformedTemp[2][0];
	if(aabbTemp[0][1] < transformedTemp[2][0]) aabbTemp[0][1] = transformedTemp[2][0];
	if(aabbTemp[1][0] > transformedTemp[2][1]) aabbTemp[1][0] = transformedTemp[2][1];
	if(aabbTemp[1][1] < transformedTemp[2][1]) aabbTemp[1][1] = transformedTemp[2][1];
	if(aabbTemp[2][0] > transformedTemp[2][2]) aabbTemp[2][0] = transformedTemp[2][2];
	if(aabbTemp[2][1] < transformedTemp[2][2]) aabbTemp[2][1] = transformedTemp[2][2];
	mulMat4Vec4Proj(camera, transformedTemp[0], transformed[0]);
	mulMat4Vec4Proj(camera, transformedTemp[1], transformed[1]);
	mulMat4Vec4Proj(camera, transformedTemp[2], transformed[2]);
	if(transformed[0][2] < 0.0F || transformed[1][2] < 0.0F || transformed[2][2] < 0.0F) return;
	if(transformed[0][2] > 1.0F || transformed[1][2] > 1.0F || transformed[2][2] > 1.0F) return;
	transformed[0][0] = roundf(transformed[0][0] * halfScreenSize[0] / aspect + halfScreenSize[0]);
	transformed[0][1] = roundf(transformed[0][1] * halfScreenSize[1] + halfScreenSize[1]);
	transformed[1][0] = roundf(transformed[1][0] * halfScreenSize[0] / aspect + halfScreenSize[0]);
	transformed[1][1] = roundf(transformed[1][1] * halfScreenSize[1] + halfScreenSize[1]);
	transformed[2][0] = roundf(transformed[2][0] * halfScreenSize[0] / aspect + halfScreenSize[0]);
	transformed[2][1] = roundf(transformed[2][1]  * halfScreenSize[1] + halfScreenSize[1]);
	maxCoord[0] = (unsigned int)max(min(max(max(transformed[0][0], transformed[1][0]), transformed[2][0]), (int)screenSize[0]), 0);
	maxCoord[1] = (unsigned int)max(min(max(max(transformed[0][1], transformed[1][1]), transformed[2][1]), (int)screenSize[1]), 0);
	minCoord[0] = (unsigned int)max(min(min(transformed[0][0], transformed[1][0]), transformed[2][0]), 0);
	minCoord[1] = (unsigned int)max(min(min(transformed[0][1], transformed[1][1]), transformed[2][1]), 0);
	area = edgeFunction(transformed[0][0], transformed[0][1], transformed[1], transformed[2]);
	if(area == 0.0F) return;
	if(image == NULL) {
		vertexColors[0] = vertices[0].color * transformed[0][3];
		vertexColors[1] = vertices[1].color * transformed[1][3];
		vertexColors[2] = vertices[2].color * transformed[2][3];
	} else {
		textures[0][0] = uv[0][0] * transformed[0][3];
		textures[0][1] = uv[0][1] * transformed[0][3];
		textures[1][0] = uv[1][0] * transformed[1][3];
		textures[1][1] = uv[1][1] * transformed[1][3];
		textures[2][0] = uv[2][0] * transformed[2][3];
		textures[2][1] = uv[2][1] * transformed[2][3];
	}
	for(y = minCoord[1];y < maxCoord[1];y++) {
		for(x = minCoord[0];x < maxCoord[0];x++) {
			float weights[3];
			weights[0] = edgeFunction(x + 0.5F, y + 0.5F, transformed[1], transformed[2]);
			weights[1] = edgeFunction(x + 0.5F, y + 0.5F, transformed[2], transformed[0]);
			weights[2] = edgeFunction(x + 0.5F, y + 0.5F, transformed[0], transformed[1]);
			if(weights[0] >= 0.0F && weights[1] >= 0.0F && weights[2] >= 0.0F) {
				size_t index = (size_t)screenSize[0] * y + x;
				float depth;
				float dataCoords[2];
				unsigned char color;
				weights[0] /= area;
				weights[1] /= area;
				weights[2] /= area;
				depth = 1.0F / (transformed[0][3] * weights[0] + transformed[1][3] * weights[1] + transformed[2][3] * weights[2]);
				if(depth < zBuffer[index]) {
					if(image == NULL) {
						color = (unsigned char)roundf(depth * (vertexColors[0] * weights[0] + vertexColors[1] * weights[1] + vertexColors[2] * weights[2]));
						if(color != NULL_COLOR) {
							buffer[index] = color;
							zBuffer[index] = depth;
						}
					} else {
						dataCoords[0] = depth * (textures[0][0] * weights[0] + textures[1][0] * weights[1] + textures[2][0] * weights[2]);
						dataCoords[1] =	depth * (textures[0][1] * weights[0] + textures[1][1] * weights[1] + textures[2][1] * weights[2]);
						color = image->data[image->width * min((unsigned int)(roundf(image->height * dataCoords[1])), image->height - 1) + min((unsigned int)(roundf(image->width * dataCoords[0])), image->width - 1)];
						if(color != image->transparent) {
							buffer[index] = color;
							zBuffer[index] = depth;
						}
					}
				}
			}
		}
	}
}

void fillPolygons(Vector vertices, Vector indices, Image *image, Vector uv, Vector uvIndices) {
	unsigned long i;
	static float defaultUV[2] = { 0.0F, 0.0F };
	resetIteration(&indices);
	resetIteration(&uvIndices);
	for(i = 0;i < indices.length / 3;i++) {
		unsigned long index[3], uvIndex[3];
		Vertex triangle[3];
		float *triangleUV[3] = { defaultUV, defaultUV, defaultUV };
		index[0] = *(unsigned long*)nextData(&indices);
		index[1] = *(unsigned long*)nextData(&indices);
		index[2] = *(unsigned long*)nextData(&indices);
		uvIndex[0] = *(unsigned long*)nextData(&uvIndices);
		uvIndex[1] = *(unsigned long*)nextData(&uvIndices);
		uvIndex[2] = *(unsigned long*)nextData(&uvIndices);
		triangle[0] = *(Vertex*)dataAt(vertices, index[0]);
		triangle[1] = *(Vertex*)dataAt(vertices, index[1]);
		triangle[2] = *(Vertex*)dataAt(vertices, index[2]);
		if(uv.length != 0) {
			triangleUV[0] = (float*)dataAt(uv, uvIndex[0]);
			triangleUV[1] = (float*)dataAt(uv, uvIndex[1]);
			triangleUV[2] = (float*)dataAt(uv, uvIndex[2]);
		}
		fillTriangle(triangle, image, triangleUV);
	}
}

Image loadBitmap(char *fileName, unsigned char transparent) {
	Image image = { 0, 0, NULL_COLOR, NULL };
	FILE *file;
	BitmapHeader header;
	BitmapInfoHeader infoHeader;
	uint32_t *img;
	unsigned int y;
	if(fopen_s(&file, fileName, "rb")) {
		fprintf(stderr, "Failed to open the file '%s'\n", fileName);
		fclose(file);
		return image;
	}
	if(fread_s(&header, sizeof(BitmapHeader), 1, sizeof(BitmapHeader), file) != sizeof(BitmapHeader)) {
		fprintf(stderr, "BMP header does not exist in the file '%s'\n", fileName);
		fclose(file);
		return image;
	}
	if(header.magicNumber[0] != 'B' || header.magicNumber[1] != 'M') {
		fprintf(stderr, "Unknown magic number.\n");
		fclose(file);
		return image;
	}
	if(header.dibSize != 40) {
		fprintf(stderr, "Unknown DIB header type.\n");
		fclose(file);
		return  image;
	}
	if(fread_s(&infoHeader, sizeof(BitmapInfoHeader), 1, sizeof(BitmapInfoHeader), file) != sizeof(BitmapInfoHeader)) {
		fprintf(stderr, "Failed to load Bitmap info header.\n");
		fclose(file);
		return image;
	}
	if(infoHeader.width < 0 || infoHeader.height < 0) {
		fprintf(stderr, "Negative demensions are not supported.\n");
		fclose(file);
		return image;
	}
	if(infoHeader.compressionMethod != 0) {
		fprintf(stderr, "Compressions are not supported.\n");
		fclose(file);
		return image;
	}
	if(fseek(file, (long)header.offset, SEEK_SET)) {
		fprintf(stderr, "Image data does not exist.\n");
		fclose(file);
		return image;
	}
	img = (uint32_t*)malloc(infoHeader.imageSize);
	if(fread_s(img, infoHeader.imageSize, 1, infoHeader.imageSize, file) != infoHeader.imageSize) {
		fprintf(stderr, "Image data is corrupted.\n");
		fclose(file);
		return image;
	}
	image.width = (unsigned int)infoHeader.width;
	image.height = (unsigned int)infoHeader.height;
	image.transparent = transparent;
	image.data = (unsigned char*)malloc(image.width * image.height);
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
	unsigned int y;
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
	for(y = 0;y < image.height;y++) {
    unsigned int x;
    for(x = 0;x < image.width;x++) image.data[image.width * y + x] = color;
	}
	return image;
}

Image genCircle(unsigned int radius, unsigned char color) {
	Image image;
	unsigned int edgeWidth = 2 * radius;
	unsigned int y;
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
