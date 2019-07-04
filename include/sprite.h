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
	Vector vertices;
	Vector indices;
	Image texture;
	struct _Sprite *parent;
	Vector children;
	int (*behaviour)(struct _Sprite*);
}	Sprite;

Sprite initSprite(const char *id, Image image);
void drawSprite(Sprite *sprite);

#endif
