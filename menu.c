#include<math.h>

#include "./cnsglib/include/cnsg.h"

#include "./include/common.h"
#include "./include/menu.h"
#include "./include/title.h"
#include "./include/race.h"

static Window *previousWindow;
static Window menuWindow;
static View menuView;
static Scene menuScene;
static Node foregroundNode;
static Node menuNode;
static int isMenuOpened;
static MenuItem menuSelect;
static float width, height;
static float transitionRatio;
static int selected;

static void escapeEvent(void) {
  isMenuOpened = FALSE;
}

static void upArrowEvent(void) {
	if(isMenuOpened) menuSelect = menuSelect <= 0 ? 2 : menuSelect - 1;
}

static void downArrowEvent(void) {
	if(isMenuOpened) menuSelect = menuSelect >= 2 ? 0 : menuSelect + 1;
}

static void enterKeyEvent(void) {
	if(isMenuOpened) {
    if(menuSelect == QUIT) {
      deinitCNSG();
    } else {
      isMenuOpened = FALSE;
      selected = TRUE;
    }
	}
}

static int menuBehaviour(Node *node, float elapsed) {
	switch(menuSelect) {
		case 0:
			drawTextSJIS(&node->texture, &shnm12, 0, 0, ">タイトルへ\n 再走する\n ゲーム終了");
			break;
		case 1:
			drawTextSJIS(&node->texture, &shnm12, 0, 0, " タイトルへ\n>再走する\n ゲーム終了");
			break;
		case 2:
			drawTextSJIS(&node->texture, &shnm12, 0, 0, " タイトルへ\n 再走する\n>ゲーム終了");
			break;
	}
	return TRUE;
}

static void menuSceneBehaviour(Scene *scene, float elapsed) {
  if(transitionRatio <= 1.0F) {
    scene->camera.fov = PI / 2.0F + PI / 3.0F * transitionRatio;
    foregroundNode.position[0] = width * 0.75F * transitionRatio;
    foregroundNode.angle[1] = PI / 24.0F * transitionRatio;
    transitionRatio += 10.0F * (isMenuOpened - transitionRatio) * elapsed;
  }
  if(!isMenuOpened && transitionRatio < 0.01F) {
    if(selected) {
      if(menuSelect == GO_TO_TITLE) startTitle();
      else if(menuSelect == RETRY) restartRace();
    } else {
      setWindow(rootManager, previousWindow, NULL, 0.0F);
    }
  }
}

void initMenu(void) {
  menuWindow = initWindow();
  menuView = initView(&menuScene);
  push(&menuWindow.views, &menuView);

  menuScene = initScene();
  menuScene.camera.target[2] = 100.0F;
  menuScene.behaviour = menuSceneBehaviour;

  initControllerEvent(&menuScene.camera.controllerList, VK_ESCAPE, NULL, escapeEvent);
  initControllerEvent(&menuScene.camera.controllerList, VK_UP, NULL, upArrowEvent);
	initControllerEvent(&menuScene.camera.controllerList, VK_DOWN, NULL, downArrowEvent);
	initControllerEvent(&menuScene.camera.controllerList, VK_RETURN, NULL, enterKeyEvent);

  foregroundNode = initNode("foreground", NO_IMAGE);
  foregroundNode.useFakeZ = TRUE;
  foregroundNode.fakeZ = 0.0F;
  foregroundNode.position[2] = 100.0F;
  height = 2.0F * foregroundNode.position[2] * tanf(PI / 4.0F);
  width = height * ((float)SCREEN_WIDTH / SCREEN_HEIGHT);
  foregroundNode.shape = initShapePlaneV(width, height, BLACK);
  push(&menuScene.nodes, &foregroundNode);

  menuNode = initNodeText("menu", 6.0F, 0.0F, LEFT, CENTER, 66, 36, menuBehaviour);
  menuNode.useFakeZ = TRUE;
  menuNode.fakeZ = 10.0F;
	push(&menuScene.nodes, &menuNode);
}

void startMenu(void) {
  if(foregroundNode.texture.data) freeImage(&foregroundNode.texture);
  getScreenShot(&foregroundNode.texture);
  previousWindow = rootManager->currentWindow;
  transitionRatio = 0.0F;
  isMenuOpened = TRUE;
  menuSelect = 0;
  selected = FALSE;
  setWindow(rootManager, &menuWindow, NULL, 0.0F);
}
