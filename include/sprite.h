#ifndef SPRITE_H
#define SPRITE_H

#include "./graphics.h"
#include "./vector.h"

typedef struct _Sprite {
	char id[10];
	float velocity[3];
	float angVelocity;
	float position[3];
	float angle[3];
	float scale[3];
	float shadowScale;
	float shadowOffset[2];
	Image texture;
	Vector indices;
	Vector vertices;
	Vector uv;
	Vector uvIndices;
	struct _Sprite *parent;
	Vector children;
	int (*behaviour)(struct _Sprite*);
	int isInterface;
}	Sprite;

Sprite initSprite(const char *id, Image image);
void discardSprite(Sprite sprite);

void drawSprite(Sprite *sprite);

void genPolygonsPlane(unsigned int width, unsigned int height, Vector *indices, Vector *vertices, Vector *uv, Vector *uvIndices);
void genPolygonsBox(unsigned int width, unsigned int height, unsigned int depth, Vector *indices, Vector *vertices, Vector *uv, Vector *uvIndices);
void readObj(char *filename, Vector *indices, Vector *vertices, Vector *uv, Vector *uvIndices);

#endif
