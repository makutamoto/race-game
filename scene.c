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
      .position = { 0.0F, -100.0F, -100.0F },
      .target = { 0.0F, 0.0F, 0.0F },
      .worldUp = { 0.0F, 1.0F, 0.0F },
      .fov = PI / 3.0F * 2.0F,
      .nearLimit = 10.0F,
      .farLimit = 500.0F,
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
  setCameraMat4(camera);
  clearBuffer(scene->background);
  clearZBuffer();
  resetIteration(&scene->objects);

  while((sprite = previousData(&scene->objects))) {
    addVec3(sprite->position, sprite->velocity, sprite->position);
    clearVector(&sprite->collisionTargets);
    if(sprite->collisionMask) {
      Sprite *collisionTarget;
      VectorItem *item = scene->objects.currentItem;
      while((collisionTarget = (Sprite*)previousData(&scene->objects))) {
        if(sprite->collisionMask & collisionTarget->collisionMask) {
          if(testCollision(*sprite, *collisionTarget)) {
            push(&sprite->collisionTargets, collisionTarget);
            push(&collisionTarget->collisionTargets, sprite);
          }
        }
      }
      scene->objects.currentItem = item;
    }
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
