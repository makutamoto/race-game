#include<Windows.h>

#include "./include/sprite.h"
#include "./include/graphics.h"

Sprite initSprite(const char *id, Image image) {
  Sprite sprite;
	memcpy_s(sprite.id, sizeof(sprite.id), id, min(sizeof(sprite.id), strlen(id)));
	sprite.velocity[0] = 0.0;
	sprite.velocity[1] = 0.0;
  sprite.velocity[2] = 0.0;
	sprite.angVelocity = 0.0;
	sprite.position[0] = 0.0;
	sprite.position[1] = 0.0;
  sprite.position[2] = 0.0;
	sprite.angle[0] = 0.0;
  sprite.angle[1] = 0.0;
  sprite.angle[2] = 0.0;
	sprite.scale[0] = 1.0;
	sprite.scale[1] = 1.0;
  sprite.scale[2] = 1.0;
	sprite.shadowScale = 0.0;
	sprite.shadowOffset[0] = 0.0;
	sprite.shadowOffset[1] = 0.0;
	sprite.image = image;
	sprite.parent = NULL;
  sprite.behaviour = NULL;
	int i;
	for(i = 0;i < (int)(sizeof(sprite.children) / sizeof(Sprite*));i++) sprite.children[i] = NULL;
  return sprite;
}

int addChild(Sprite *parent, Sprite *child) {
	int i;
	for(i = 0;i < (int)(sizeof(parent->children) / sizeof(Sprite*));i++) {
		if(parent->children[i] == NULL) {
			parent->children[i] = child;
			child->parent = parent;
			return TRUE;
		}
	}
	return FALSE;
}

void drawSprite(Sprite *sprite) {
  if(sprite->behaviour != NULL) {
    if(!sprite->behaviour(sprite)) return;
  }
	pushTransformation();
  translateTransformation(sprite->position[0], sprite->position[1], sprite->position[2]);
  rotateTransformation(sprite->angle[0], sprite->angle[1], sprite->angle[2]);
	pushTransformation();
  translateTransformation(sprite->shadowOffset[0], sprite->shadowOffset[1], 0.0);
	scaleTransformation(sprite->shadowScale, sprite->shadowScale, 1.0);
	if(sprite->shadowOffset[0] != 0.0F || sprite->shadowOffset[1] != 0.0F) fillBuffer(sprite->image, TRUE);
	popTransformation();
	int i;
	for(i = 0;i < (int)(sizeof(sprite->children) / sizeof(Sprite*));i++) {
		if(sprite->children[i] != NULL) drawSprite(sprite->children[i]);
	}
  scaleTransformation(sprite->scale[0], sprite->scale[1], sprite->scale[2]);
	fillBuffer(sprite->image, FALSE);
	popTransformation();
}
