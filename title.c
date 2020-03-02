#include "./cnsglib/include/cnsg.h"

#include "./include/common.h"
#include "./include/title.h"
#include "./include/race.h"
#include "./include/record.h"

static Window titleWindow;
static View titleView;
static Scene titleScene;
static Node titleNode, titleMenuNode;
static Node titleCarNode;

static int titleMenuEntered;
static TitleMenuItem titleMenuSelect;
static Vector titleRecords;

static void escapeEvent(void) {
  titleMenuEntered = FALSE;
}

static void upEvent(void) {
	titleMenuSelect = titleMenuSelect <= 0 ? 2 : titleMenuSelect - 1;
}

static void downEvent(void) {
	titleMenuSelect = titleMenuSelect >= 2 ? 0 : titleMenuSelect + 1;
}

static void enterEvent(void) {
  if(titleMenuEntered) {
    switch(titleMenuSelect) {
      case ONEP_PLAY:
        startRace(FALSE);
        break;
      case TWOP_PLAY:
        startRace(TRUE);
        break;
      case TITLE_QUIT:
        deinitCNSG();
        break;
    }
  } else {
    titleMenuEntered = TRUE;
    titleMenuSelect = 0;
  }
}

static int titleMenuBehaviour(Node *node, float elapsed) {
	if(titleMenuEntered) {
		switch(titleMenuSelect) {
			case 0:
				drawTextSJIS(&node->texture, &shnm12, 0, 0, " >1P PLAY  \n  2P PLAY  \n  QUIT     ");
				break;
			case 1:
				drawTextSJIS(&node->texture, &shnm12, 0, 0, "  1P PLAY  \n >2P PLAY  \n  QUIT     ");
				break;
			case 2:
				drawTextSJIS(&node->texture, &shnm12, 0, 0, "  1P PLAY  \n  2P PLAY  \n >QUIT     ");
				break;
		}
	} else {
		drawTextSJIS(&node->texture, &shnm12, 0, 0, "PRESS ENTER\n           \n           ");
	}
	return TRUE;
}

static int titleCarBehaviour(Node *node, float elapsed) {
  if(playCar(&titleRecords, node, titleScene.clock)) titleScene.clock = 0.0F;
  return TRUE;
}

void initTitle(void) {
  titleWindow = initWindow();
  titleView = initView(&titleScene);
  push(&titleWindow.views, &titleView);

  titleScene = initScene();
  titleScene.camera = initCamera(0.0F, 20.0F, 15.0F);
  titleScene.camera.parent = &titleCarNode;
  titleScene.camera.positionMask[1] = TRUE;
  titleScene.camera.farLimit = 3000.0F;
  titleScene.background = BLUE;

  initControllerEvent(&titleScene.camera.controllerList, VK_ESCAPE, NULL, escapeEvent);
  initControllerEvent(&titleScene.camera.controllerList, VK_UP, NULL, upEvent);
  initControllerEvent(&titleScene.camera.controllerList, VK_DOWN, NULL, downEvent);
  initControllerEvent(&titleScene.camera.controllerList, VK_RETURN, NULL, enterEvent);

  titleNode =	initNodeText("title", 0.0F, -16.0F, CENTER, CENTER, 104, 16, NULL);
	drawTextSJIS(&titleNode.texture, &shnm16b, 0, 0, "COARSE RACING");
  titleMenuNode =	initNodeText("titleMenu", 0.0F, 22.0F, CENTER, CENTER, 66, 36, titleMenuBehaviour);
	pushUntilNull(&titleScene.nodes, &titleNode, &titleMenuNode, &titleCarNode, &courseNode, &stageNode, NULL);

  titleCarNode = initNode("titleCar", carImage);
	titleCarNode.shape = carShape;
	setVec3(titleCarNode.scale, 4.0F, XYZ_MASK);
	titleCarNode.behaviour = titleCarBehaviour;

  loadRecord(&titleRecords, "./carRecords/title.crd");
}

void startTitle(void) {
  StopAllSound();
	titleMenuEntered = FALSE;
	resetIteration(&titleRecords);
	titleCarNode.position[0] = 600.0F;
	titleCarNode.position[1] = 75.0F;
	titleCarNode.position[2] = 0.0F;
	clearVec3(titleCarNode.velocity);
	clearVec3(titleCarNode.angle);
	clearVec3(titleCarNode.angVelocity);
	clearVec3(titleCarNode.angMomentum);
	titleScene.clock = 0.0F;
  setWindow(rootManager, &titleWindow, revoluteTransition, 1.0F);
}
