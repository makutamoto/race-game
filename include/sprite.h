#ifndef SPRITE_H
#define SPRITE_H

#include<time.h>

#include "./graphics.h"
#include "./vector.h"

typedef struct {
	Vector indices;
	Vector vertices;
	Vector uv;
	Vector uvIndices;
} Shape;

typedef struct _Sprite {
	char id[10];
	float velocity[3];
	float angVelocity;
	float position[3];
	float angle[3];
	float scale[3];
	float aabb[3][2];
	Image *texture;
	Shape shape;
	unsigned int collisionMask;
	Vector collisionTargets;
	Vector intervalEvents;
	struct _Sprite *parent;
	Vector children;
	int (*behaviour)(struct _Sprite*);
	int isInterface;
}	Sprite;

typedef struct {
	clock_t begin;
	unsigned int interval;
	int (*callback)(Sprite*);
} IntervalEvent;

Sprite initSprite(const char *id, Image *image);
void discardSprite(Sprite sprite);

void drawSprite(Sprite *sprite);
int testCollision(Sprite a, Sprite b);
void addIntervalEvent(Sprite *sprite, unsigned int milliseconds, int (*callback)(Sprite*));

Shape initShapePlane(unsigned int width, unsigned int height, unsigned char color);
Shape initShapeBox(unsigned int width, unsigned int height, unsigned int depth, unsigned char color);
int initShapeFromObj(Shape *shape, char *filename);
void discardShape(Shape shape);

#endif
