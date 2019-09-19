#include<Windows.h>

#include "./include/scene.h"
#include "./include/node.h"
#include "./include/vector.h"
#include "./include/matrix.h"
#include "./include/graphics.h"

Scene initScene(void) {
  Scene scene = {
    .nodes = initVector(),
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
  Node *node;
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
  resetIteration(&scene->nodes);
  while((node = previousData(&scene->nodes))) {
    if(node->isInterface) {
      clearCameraMat4();
      drawNode(node);
      setCameraMat4(camera);
    } else {
      drawNode(node);
    }
    addVec3(node->position, node->velocity, node->position);
    clearVector(&node->collisionTargets);
    if(node->collisionMaskActive || node->collisionMaskPassive) {
      Node *collisionTarget;
      VectorItem *item = scene->nodes.currentItem;
      while((collisionTarget = (Node*)previousData(&scene->nodes))) {
        if(testCollision(*node, *collisionTarget)) {
          if(node->collisionMaskPassive & collisionTarget->collisionMaskActive || node->collisionMaskActive & collisionTarget->collisionMaskPassive) {
            push(&node->collisionTargets, collisionTarget);
            push(&collisionTarget->collisionTargets, node);
          }
        }
      }
      scene->nodes.currentItem = item;
    }
  }
  flushBuffer(screen);
}

void discardScene(Scene *scene) {
  clearVector(&scene->nodes);
}
