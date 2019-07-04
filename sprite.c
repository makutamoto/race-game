#include<Windows.h>

#include "./include/sprite.h"
#include "./include/graphics.h"
#include "./include/vector.h"

Sprite initSprite(const char *id, Image image) {
  Sprite sprite;
  memset(&sprite, 0, sizeof(sprite));
	memcpy_s(sprite.id, sizeof(sprite.id), id, min(sizeof(sprite.id), strlen(id)));
	sprite.scale[0] = 1.0;
	sprite.scale[1] = 1.0;
  sprite.scale[2] = 1.0;
	sprite.texture = image;
  sprite.children = initVector();
	return sprite;
}

void drawSprite(Sprite *sprite) {
  Sprite *child;
  if(sprite->behaviour != NULL) {
    if(!sprite->behaviour(sprite)) return;
  }
	pushTransformation();
  translateTransformation(sprite->position[0], sprite->position[1], sprite->position[2]);
  rotateTransformation(sprite->angle[0], sprite->angle[1], sprite->angle[2]);
	pushTransformation();
  translateTransformation(sprite->shadowOffset[0], sprite->shadowOffset[1], 0.0);
	scaleTransformation(sprite->shadowScale, sprite->shadowScale, 1.0);
	if(sprite->shadowOffset[0] != 0.0F || sprite->shadowOffset[1] != 0.0F) fillBuffer(sprite->texture, TRUE);
	popTransformation();
  resetIteration(&sprite->children);
  while((child = previousData(&sprite->children))) drawSprite(child);
  scaleTransformation(sprite->scale[0], sprite->scale[1], sprite->scale[2]);
	fillBuffer(sprite->texture, FALSE);
	popTransformation();
}
