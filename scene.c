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
  node = previousData(&scene->nodes);
  while(node) {
    setCameraMat4(camera);
    if(!drawNode(node)) {
      node = previousData(&scene->nodes);
      continue;
    }
    addVec3(node->position, node->velocity, node->position);
    clearVector(&node->collisionTargets);
    if(node->collisionMaskActive || node->collisionMaskPassive) {
      Node *collisionTarget;
      VectorItem *item = scene->nodes.currentItem;
      collisionTarget = (Node*)previousData(&scene->nodes);
      while(collisionTarget) {
        if(testCollision(*node, *collisionTarget)) {
          if(node->collisionMaskPassive & collisionTarget->collisionMaskActive || node->collisionMaskActive & collisionTarget->collisionMaskPassive) {
            push(&node->collisionTargets, collisionTarget);
            push(&collisionTarget->collisionTargets, node);
          }
        }
        collisionTarget = (Node*)previousData(&scene->nodes);
      }
      scene->nodes.currentItem = item;
    }
    node = previousData(&scene->nodes);
  }
  flushBuffer(screen);
}

void discardScene(Scene *scene) {
  clearVector(&scene->nodes);
}
