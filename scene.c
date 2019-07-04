#include<Windows.h>

#include "./include/scene.h"
#include "./include/sprite.h"
#include "./include/vector.h"
#include "./include/matrix.h"
#include "./include/graphics.h"

Scene initScene(void) {
  Scene scene = {
    .objects = initVector(),
    .interfaces = initVector(),
    .camera = {
      .position = { 0.0F, 50.0F, 100.0F },
      .target = { 0.0F, 0.0F, 0.0F },
      .worldUp = { 0.0F, 1.0F, 0.0F },
      .fov = PI / 2.0F,
      .nearLimit = 0.01F,
      .farLimit = 1000.0F,
      .aspect = 1.0F,
    },
  };
  return scene;
}

void drawScene(Scene *scene, HANDLE screen) {
  Sprite *sprite;
  float lookAt[4][4];
  float projection[4][4];
  float camera[4][4];
  clearTransformation();
  genLookAtMat4(scene->camera.position, scene->camera.target, scene->camera.worldUp, lookAt);
  genPerspectiveMat4(scene->camera.fov, scene->camera.nearLimit, scene->camera.farLimit, scene->camera.aspect, projection);
  mulMat4(projection, lookAt, camera);
  mulTransformationL(camera);
  clearBuffer(scene->background);
  resetIteration(&scene->objects);
  while((sprite = previousData(&scene->objects))) {
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
  clearTransformation();
  resetIteration(&scene->interfaces);
  while((sprite = previousData(&scene->interfaces))) drawSprite(sprite);
  flushBuffer(screen);
}

void discardScene(Scene *scene) {
  clearVector(&scene->objects);
  clearVector(&scene->interfaces);
}
