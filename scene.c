#include<Windows.h>

#include "./include/scene.h"
#include "./include/node.h"
#include "./include/vector.h"
#include "./include/matrix.h"
#include "./include/graphics.h"

Camera initCamera(float x, float y, float z, float aspect) {
  Camera camera;
  memset(&camera, 0, sizeof(Camera));
  camera.position[0] = x;
  camera.position[1] = y;
  camera.position[2] = z;
  camera.worldUp[0] = 0.0F;
  camera.worldUp[1] = 1.0F;
  camera.worldUp[2] = 0.0F;
  camera.fov = PI / 3.0F * 2.0F;
  camera.nearLimit = 10.0F;
  camera.farLimit = 500.0F;
  camera.aspect = aspect;
  return camera;
}

Scene initScene(void) {
  Scene scene;
  memset(&scene, 0, sizeof(Scene));
  scene.nodes = initVector();
  scene.camera = initCamera(0.0F, 0.0F, 0.0F, 1.0F);
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
  clearBuffer(scene->background);
  clearZBuffer();
  resetIteration(&scene->nodes);
  node = nextData(&scene->nodes);
  while(node) {
    setCameraMat4(camera);
    drawNode(node);
    clearVector(&node->collisionTargets);
    node->collisionFlags = 0;
    node = nextData(&scene->nodes);
  }
  resetIteration(&scene->nodes);
  node = nextData(&scene->nodes);
  while(node) {
    addVec3(node->position, node->velocity, node->position);
    if(node->collisionMaskActive || node->collisionMaskPassive) {
      Node *collisionTarget;
      VectorItem *item = scene->nodes.currentItem;
      collisionTarget = (Node*)nextData(&scene->nodes);
      while(collisionTarget) {
        if(testCollision(*node, *collisionTarget)) {
          unsigned int flagsA = node->collisionMaskPassive & collisionTarget->collisionMaskActive;
          unsigned int flagsB = node->collisionMaskActive & collisionTarget->collisionMaskPassive;
          unsigned int flags = flagsA | flagsB;
          if(flags) {
            push(&node->collisionTargets, collisionTarget);
            push(&collisionTarget->collisionTargets, node);
            node->collisionFlags |= flags;
            collisionTarget->collisionFlags |= flags;
          }
        }
        collisionTarget = (Node*)nextData(&scene->nodes);
      }
      scene->nodes.currentItem = item;
    }
    node = nextData(&scene->nodes);
  }
  resetIteration(&scene->nodes);
  node = previousData(&scene->nodes);
  while(node) {
    IntervalEvent *interval;
    if(node->behaviour != NULL) {
      if(!node->behaviour(node)) {
        node = previousData(&scene->nodes);
        continue;
      }
    }
    resetIteration(&node->intervalEvents);
    interval = nextData(&node->intervalEvents);
    while(interval) {
      clock_t current = clock();
      clock_t diff = current - interval->begin;
      if(diff < 0) {
        interval->begin = current;
      } else {
        if(interval->interval < (unsigned int)diff) {
          interval->begin = current;
          interval->callback(node);
        }
      }
      interval = nextData(&node->intervalEvents);
    }
    node = previousData(&scene->nodes);
  }
  flushBuffer(screen);
}

void discardScene(Scene *scene) {
  clearVector(&scene->nodes);
}
