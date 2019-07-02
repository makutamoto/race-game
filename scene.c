#include "./include/scene.h"
#include "./include/sprite.h"
#include "./include/vector.h"
#include "./include/matrix.h"

Scene initScene(void) {
  Scene scene = {
    .children = initVector(),
    .background = 0,
  };
  return scene;
}

void drawScene(Scene *scene) {
  Sprite *sprite;
  setBuffer(scene->background);
  resetIteration(&scene->children);
  while((sprite = (Sprite*)previousData(&scene->children))) {
    // Sprite *collisionTarget;
    // VectorItem *item = scene->children.currentItem;
    // while((collisionTarget = (Sprite*)previousData(&scene->children))) {
    //   int distance = distance2(sprite->position, collisionTarget->position);
    //   if(distance - sprite->collisionRadius - collisionTarget->collisionRadius < 0) {
    //     sprite->collisionTarget = collisionTarget;
    //     collisionTarget->collisionTarget = sprite;
    //   }
    // }
    // scene->children.currentItem = item;
    drawSprite(sprite);
  }
}

void discardScene(Scene *scene) {
  clearVector(&scene->children);
}
