#include<stdio.h>
#include<math.h>
#include<time.h>
#include<Windows.h>

#include "./cnsglib/include/cnsg.h"

#include "./include/record.h"

#define SCREEN_HEIGHT 128
#define SCREEN_WIDTH 180 // 1.414 * SCREEN_HEIGHT
#define SCREEN_ASPECT ((float)SCREEN_WIDTH / SCREEN_HEIGHT)
#define FRAME_PER_SECOND 60

#define CAR_COLLISIONMASK 0x01
#define COURSE_COLLISIONMASK 0x02
#define LAP_COLLISIONMASK (LAPA_COLLISIONMASK | LAPB_COLLISIONMASK | LAPC_COLLISIONMASK)
#define LAPA_COLLISIONMASK 0x04
#define LAPB_COLLISIONMASK 0x08
#define LAPC_COLLISIONMASK 0x10
#define DIRT_COLLISIONMASK 0x20

#define CENTER_X(size) ((SCREEN_WIDTH - (size)) / 2.0F)
#define CENTER_Y(size) ((SCREEN_HEIGHT - (size)) / 2.0F)

typedef enum {
	ONEP_PLAY, TWOP_PLAY, TITLE_QUIT
} TitleMenuItem;

typedef enum {
	GO_TO_TITLE, RETRY, QUIT
} MenuItem;

static struct {
	float move[2];
	float arrow[4];
	float direction[2];
	float action;
	float collision;
	float backCamera;
	float resetCamera;
} controller;

static FontSJIS shnm12;
static FontSJIS shnm16b;

static int titleMenuEntered;
static int titleMenuSelect;
static ControllerData *wasd[4], *action, *collision, *menu, *arrow[4], *backCamera, *resetCamera;
static ControllerData *enterKey;
static int menuSelect;

static Image hero, opponentImage;
static Image course;
static Image stageImage;

static Scene titleScene;
static Node titleNode, titleMenuNode;

static Scene raceScene;
static Node speedNode, lapNode, rankNode, centerNode, timeNode, replayNode;
static Node mapNode;
static Node lapJudgmentNodes[3];
static Node heroNode, heroRayNode;
static Node opponentNode, opponentRayNode;
static Node heroMarkerNode, opponentMarkerNode;
static Node courseNode, courseMapNode, courseDirtNode;
static Node stageNode;

static Scene menuScene;
static Node menuNode;

static Scene mapScene;

static float nextCameraPosition[3];
static float currentCameraPosition[3];
static float cameraAngle, cameraFov;
static int heroLapScore, opponentLapScore;
static int heroPreviousLap = -1, opponentPreviousLap = -1;
static int collisionFlag;
static int rank;
static int isTitle, isRaceStarted, isfinished, isReplaying;
static int isWrongWay;
static int isMenuOpened;
static float menuTransition;
static float finishedTime;
static float currentTime;
static float handle, accel;
static float heroAngle;

static Vector titleRecords, heroRecords, opponentRecords;

static Sound bgm;

void deinitGame(void);

static void quitGame(void) {
	deinitGame();
	deinitCNSG();
	ExitProcess(0);
}

static void replay(void) {
	opponentNode.isVisible = TRUE;
	opponentMarkerNode.isVisible = TRUE;
	raceScene.clock = 0.0F;
	resetIteration(&opponentRecords);
}

static void printCenter(const char text[10]) {
	drawTextSJIS(centerNode.texture, shnm16b, 0, 0, (char*)text);
	centerNode.isVisible = TRUE;
}

static float controlCar(Node *node, float accel, float handle, float front[3]) {
	float tempVec3[3][3];
	float tempMat3[1][3][3];
	float tempMat4[1][4][4];
	float velocityLengthH;
	convMat4toMat3(genRotationMat4(node->angle[0], node->angle[1], node->angle[2], tempMat4[0]), tempMat3[0]);
	initVec3(tempVec3[0], Z_MASK);
	mulMat3Vec3(tempMat3[0], tempVec3[0], front);
	normalize3(extractComponents3(front, XZ_MASK, tempVec3[0]), tempVec3[1]);
	extractComponents3(node->velocity, XZ_MASK, tempVec3[0]);
	velocityLengthH = length3(tempVec3[0]);
	mulVec3ByScalar(tempVec3[1], velocityLengthH * node->shape.mass + accel * 150000.0F, tempVec3[0]);
	subVec3(tempVec3[0], mulVec3ByScalar(node->velocity, node->shape.mass, tempVec3[2]), tempVec3[1]);
	applyForce(node, tempVec3[1], XZ_MASK, FALSE);
	if(velocityLengthH > 1.0F) node->torque[1] += handle * max(velocityLengthH * 5000.0F, 2500000.0F);
	initVec3(tempVec3[0], Y_MASK);
	mulMat3Vec3(tempMat3[0], tempVec3[0], tempVec3[1]);
	initVec3(tempVec3[0], Y_MASK);
	return cosVec3(tempVec3[0], tempVec3[1]);
}

static float convVelocityToSpeed(float velocity[3]) {
	return length3(velocity) * 3600 / 10000;
}

static int updateLapScore(unsigned int flags, int *previousLap, int *lapScore, int wrongWay) {
	if(flags & LAPA_COLLISIONMASK) {
		if(*previousLap == 2) {
			*lapScore += 1;
			wrongWay = FALSE;
		} else if(*previousLap == 1) {
			*lapScore -= 1;
			wrongWay = TRUE;
		}
		*previousLap = 0;
	}
	if(flags & LAPB_COLLISIONMASK) {
		if(*previousLap == 0) {
			*lapScore += 1;
			wrongWay = FALSE;
		} else if(*previousLap == 2) {
			*lapScore -= 1;
			wrongWay = TRUE;
		}
		*previousLap = 1;
	}
	if(flags & LAPC_COLLISIONMASK) {
		if(*previousLap == 1) {
			*lapScore += 1;
			wrongWay = FALSE;
		} else if(*previousLap == 0) {
			*lapScore -= 1;
			wrongWay = TRUE;
		}
		*previousLap = 2;
	}
	return wrongWay;
}

static int heroBehaviour(Node *node) {
	float front[3];
	if(isTitle) {
		if(playCar(&titleRecords, node, titleScene.clock)) titleScene.clock = 0.0F;
	} else {
		if(isRaceStarted) {
			if(isfinished) {
				if(isReplaying) {
					if(playCar(&heroRecords, node, currentTime)) replay();
				} else {
					controlCar(node, 0.0F, 0.0F, front);
				}
			} else {
				recordCar(&heroRecords, node, currentTime);
				if(node->collisionFlags & COURSE_COLLISIONMASK) {
					float upper[3] = { 0.0F, 25.0F, 0.0F };
					if(heroAngle < 0.75F) {
						heroAngle = controlCar(node, 0.0F, 0.0F, front);
					} else {
						heroAngle = controlCar(node, accel, controller.move[0], front);
					}
					if(heroAngle < 0.0F) {
						node->angle[0] = 0.0F;
						node->angle[2] = 0.0F;
						node->angVelocity[0] = 0.0F;
						node->angVelocity[2] = 0.0F;
						node->angMomentum[0] = 0.0F;
						node->angMomentum[2] = 0.0F;
					}
					mulVec3ByScalar(front, -75.0F, nextCameraPosition);
					addVec3(nextCameraPosition, upper, nextCameraPosition);
				}
			}
		}
		if(node->collisionFlags & DIRT_COLLISIONMASK) {
			node->collisionShape.dynamicFriction = 1.0F;
		} else {
			node->collisionShape.dynamicFriction = 0.1F;
		}
		isWrongWay = updateLapScore(node->collisionFlags, &heroPreviousLap, &heroLapScore, isWrongWay);
		memcpy_s(heroMarkerNode.position, SIZE_VEC3, node->position, SIZE_VEC3);
		memcpy_s(heroMarkerNode.angle, SIZE_VEC3, node->angle, SIZE_VEC3);
	}
	return TRUE;
}

static int opponentBehaviour(Node *node) {
	if(isRaceStarted) {
		if(playCar(&opponentRecords, node, currentTime)) {
			node->isVisible = FALSE;
			opponentMarkerNode.isVisible = FALSE;
		}
	}
	updateLapScore(node->collisionFlags, &opponentPreviousLap, &opponentLapScore, FALSE);
	memcpy_s(opponentMarkerNode.position, SIZE_VEC3, node->position, SIZE_VEC3);
	memcpy_s(opponentMarkerNode.angle, SIZE_VEC3, node->angle, SIZE_VEC3);
	return TRUE;
}

static int timeBehaviour(Node *node) {
	if(isfinished) {
		currentTime = raceScene.clock;
	} else {
		if(isRaceStarted) {
			char buffer[14];
			int minutes;
			float seconds;
			currentTime = raceScene.clock - 3.0F;
			minutes = currentTime / 60;
			seconds = currentTime - 60.0F * minutes;
			if(minutes > 9) {
				minutes = 9;
				seconds = 59.999F;
			}
			sprintf(buffer, "TIME %d`%06.3f", minutes, seconds);
			drawTextSJIS(node->texture, shnm12, 0, 0, buffer);
		} else {
			drawTextSJIS(node->texture, shnm12, 0, 0, "TIME 0`00.000");
		}
	}
	return TRUE;
}

static int speedBehaviour(Node *node) {
	char buffer[11];
	float speed = convVelocityToSpeed(heroNode.velocity);
	if(speed >= 1000.0F) {
		sprintf(buffer, "???.? km/h");
	} else {
		sprintf(buffer, "%5.1f km/h", speed);
	}
	drawTextSJIS(speedNode.texture, shnm12, 0, 0, buffer);
	return TRUE;
}

static int lapBehaviour(Node *node) {
	if(isfinished) {
		if(!isReplaying) {
			if(raceScene.clock - finishedTime > 3.0F) {
				replay();
				isReplaying = TRUE;
			}
		}
	} else {
		char buffer[8];
		int lap = max(heroLapScore / 45 + 1, 1);
		if(lap > 3) {
			isfinished = TRUE;
			finishedTime = raceScene.clock;
		} else {
			sprintf(buffer, "LAP %d/3", lap);
			drawTextSJIS(node->texture, shnm12, 0, 0, buffer);
		}
	}
	return TRUE;
}

static int rankBehaviour(Node *node) {
	if(!isfinished) {
		static char rankList[][4] = { "1st", "2nd" };
		if(heroLapScore != opponentLapScore) {
			rank = heroLapScore > opponentLapScore ? 0 : 1;
		}
		drawTextSJIS(node->texture, shnm12, 0, 0, rankList[rank]);
	}
	return TRUE;
}

static void raceStarted(void) {
	bgm = PlaySoundNeo("./assets/bgm.wav", TRUE);
}

static int centerBehaviour(Node *node) {
	float elapsed = 4.0F - raceScene.clock;
	if(isRaceStarted) {
		if(isfinished) {
			if(!isReplaying) {
				drawTextSJIS(node->texture, shnm16b, 0, 0, "FINISH!");
				node->isVisible = TRUE;
			} else {
				node->isVisible = FALSE;
			}
		} else {
			if(elapsed > 0.0F) {
				drawTextSJIS(node->texture, shnm16b, 0, 0, "START! ");
			} else {
				if(isWrongWay) {
					node->isVisible = TRUE;
					drawTextSJIS(node->texture, shnm16b, 0, 0, " 逆走! ");
				} else {
					node->isVisible = FALSE;
				}
			}
		}
	} else {
		char buffer[8];
		if(elapsed > 1.0F) {
			sprintf(buffer, "%4d   ", (int)elapsed);
			drawTextSJIS(node->texture, shnm16b, 0, 0, buffer);
			node->isVisible = TRUE;
		} else {
			raceStarted();
			isRaceStarted = TRUE;
		}
	}
	return TRUE;
}

static int replayBehaviour(Node *node) {
	if(isReplaying) {
		node->isVisible = TRUE;
	} else {
		node->isVisible = FALSE;
	}
	return TRUE;
}

static int menuBehaviour(Node *node) {
	switch(menuSelect) {
		case 0:
			drawTextSJIS(node->texture, shnm12, 0, 0, ">タイトルへ\n 再走する\n ゲーム終了");
			break;
		case 1:
			drawTextSJIS(node->texture, shnm12, 0, 0, " タイトルへ\n>再走する\n ゲーム終了");
			break;
		case 2:
			drawTextSJIS(node->texture, shnm12, 0, 0, " タイトルへ\n 再走する\n>ゲーム終了");
			break;
	}
	return TRUE;
}

static int titleMenuBehaviour(Node *node) {
	if(titleMenuEntered) {
		switch(titleMenuSelect) {
			case 0:
				drawTextSJIS(node->texture, shnm12, 0, 0, " >1P PLAY  \n  2P PLAY  \n  QUIT     ");
				break;
			case 1:
				drawTextSJIS(node->texture, shnm12, 0, 0, "  1P PLAY  \n >2P PLAY  \n  QUIT     ");
				break;
			case 2:
				drawTextSJIS(node->texture, shnm12, 0, 0, "  1P PLAY  \n  2P PLAY  \n >QUIT     ");
				break;
		}
	} else {
		drawTextSJIS(node->texture, shnm12, 0, 0, "PRESS ENTER\n           \n           ");
	}
	return TRUE;
}

static Node initCarRayNode(const char id[]) {
	Node carRayNode;
	carRayNode = initNode(id, NO_IMAGE);
	carRayNode.shape = initShapePlane(75.0F, 150.0F, BLACK, 1.0F);
	carRayNode.position[2] = 75.0F;
	carRayNode.collisionShape = carRayNode.shape;
	carRayNode.collisionMaskPassive = COURSE_COLLISIONMASK | LAP_COLLISIONMASK;
	carRayNode.isThrough = TRUE;
	carRayNode.isVisible = FALSE;
	return carRayNode;
}

static int mapBehaviour(Node *node) {
	freeImage(node->texture);
	node->texture = drawScene(&mapScene);
	node->texture.transparent = BLACK;
	return TRUE;
}

static void startGame(void) {
	isTitle = FALSE;

	if(bgm) StopSound(bgm);

	cameraAngle = 0.0F;
	cameraFov = PI / 3.0F * 1.5F;
	isfinished = FALSE;
	isRaceStarted = FALSE;
	isReplaying = FALSE;
	heroLapScore = 0;
	heroPreviousLap = -1;
	opponentLapScore = 0;
	opponentPreviousLap = -1;

	freeVector(&heroRecords);
	resetIteration(&opponentRecords);

	currentCameraPosition[0] = 0.0F;
	currentCameraPosition[1] = 25.0F;
	currentCameraPosition[2] = -75.0F;
	memcpy_s(nextCameraPosition, SIZE_VEC3, currentCameraPosition, SIZE_VEC3);

	heroNode.position[0] = 650.0F;
	heroNode.position[1] = 75.0F;
	heroNode.position[2] = 0.0F;
	clearVec3(heroNode.velocity);
	clearVec3(heroNode.angle);
	clearVec3(heroNode.angVelocity);
	clearVec3(heroNode.angMomentum);

	opponentNode.position[0] = 600.0F;
	opponentNode.position[1] = 75.0F;
	opponentNode.position[2] = 0.0F;
	opponentNode.isVisible = TRUE;
	opponentMarkerNode.isVisible = TRUE;
	clearVec3(opponentNode.velocity);
	clearVec3(opponentNode.angle);
	clearVec3(opponentNode.angVelocity);
	clearVec3(opponentNode.angMomentum);

	clearVector(&raceScene.nodes);
	pushUntilNull(&raceScene.nodes, &speedNode, &lapNode, &rankNode, &centerNode, &timeNode, &replayNode, NULL);
	pushUntilNull(&raceScene.nodes, &mapNode, NULL);
	pushUntilNull(&raceScene.nodes, &lapJudgmentNodes[0], &lapJudgmentNodes[1], &lapJudgmentNodes[2], NULL);
	pushUntilNull(&raceScene.nodes, &heroNode, &opponentNode, NULL);
	pushUntilNull(&raceScene.nodes, &courseNode, &courseDirtNode, NULL);
	push(&raceScene.nodes, &stageNode);

	raceScene.clock = 0.0F;
}

static void startTitle(void) {
	isTitle = TRUE;
	titleMenuEntered = FALSE;
	if(bgm) StopSound(bgm);
	resetIteration(&titleRecords);
	heroNode.position[0] = 600.0F;
	heroNode.position[1] = 75.0F;
	heroNode.position[2] = 0.0F;
	clearVec3(heroNode.velocity);
	clearVec3(heroNode.angle);
	clearVec3(heroNode.angVelocity);
	clearVec3(heroNode.angMomentum);
	titleScene.clock = 0.0F;
}

static void upArrowEvent(void) {
	titleMenuSelect = titleMenuSelect <= 0 ? 2 : titleMenuSelect - 1;
	menuSelect = menuSelect <= 0 ? 2 : menuSelect - 1;
}

static void downArrowEvent(void) {
	titleMenuSelect = titleMenuSelect >= 2 ? 0 : titleMenuSelect + 1;
	menuSelect = menuSelect >= 2 ? 0 : menuSelect + 1;
}

static void enterKeyEvent(void) {
	if(isMenuOpened) {
		switch(menuSelect) {
			case GO_TO_TITLE:
				isMenuOpened = FALSE;
				startTitle();
				break;
			case RETRY:
				isMenuOpened = FALSE;
				startGame();
				break;
			case QUIT:
				// saveRecord(&heroRecords, "./carRecords/title.crd");
				quitGame();
				break;
		}
	} else if(isTitle) {
		if(titleMenuEntered) {
			switch(titleMenuSelect) {
				case ONEP_PLAY:
					startGame();
					break;
				case TWOP_PLAY:
					break;
				case TITLE_QUIT:
					quitGame();
					break;
			}
		} else {
			titleMenuEntered = TRUE;
			titleMenuSelect = 0;
		}
	}
}

static void menuEvent(void) {
	if(isTitle) {
		titleMenuEntered = FALSE;
	} else {
		menuSelect = 0;
		isMenuOpened = !isMenuOpened;
	}
}

static void raceSceneBehaviour(Scene *scene, float elapsed) {
	float tempVec3[2][3];
	float tempMat4[1][4][4];
	float tempMat3[1][3][3];
	if(controller.collision) {
		if(!collisionFlag) {
			int i;
			for(i = 0;i < 3;i++) lapJudgmentNodes[i].isVisible = !lapJudgmentNodes[i].isVisible;
			heroRayNode.isVisible = !heroRayNode.isVisible;
			opponentRayNode.isVisible = !opponentRayNode.isVisible;
			courseDirtNode.isVisible = !courseDirtNode.isVisible;
			collisionFlag = TRUE;
		}
	} else {
		collisionFlag = FALSE;
	}
	if(!isReplaying) accel += 0.3F * (controller.move[1] - accel);
	cameraFov = min(PI / 3.0F * 1.5F, max(PI / 3.0F, cameraFov - 0.05F * controller.arrow[1]));
	raceScene.camera.fov = cameraFov + accel / 5.0F;
	addVec3(currentCameraPosition, mulVec3ByScalar(subVec3(nextCameraPosition, currentCameraPosition, tempVec3[0]), 2.0F * elapsed, tempVec3[1]), currentCameraPosition);
	cameraAngle -= 0.1F * controller.arrow[0];
	if(controller.resetCamera) cameraAngle -= 0.3F * cameraAngle;
	handle += 0.3F * (controller.move[0] - handle);
	genRotationMat4(0.0F, cameraAngle, 0.0F, tempMat4[0]);
	mulMat3Vec3(convMat4toMat3(tempMat4[0], tempMat3[0]), currentCameraPosition, raceScene.camera.position);
}

static void initGame(void) {
	int i;
	char lapNames[3][50] = { "./assets/courseMk2CollisionA.obj", "./assets/courseMk2CollisionB.obj", "./assets/courseMk2CollisionC.obj" };
	shnm12 = initFontSJIS(loadBitmap("assets/shnm6x12r.bmp", NULL_COLOR), loadBitmap("assets/shnmk12.bmp", NULL_COLOR), 6, 12, 12);
	shnm16b = initFontSJIS(loadBitmap("assets/shnm8x16rb.bmp", NULL_COLOR), loadBitmap("assets/shnmk16b.bmp", NULL_COLOR), 8, 16, 16);

	initControllerDataCross(wasd, 'W', 'A', 'S', 'D', controller.move);
	initControllerDataCross(arrow, VK_UP, VK_LEFT, VK_DOWN, VK_RIGHT, controller.arrow);
	action = initControllerData(VK_SPACE, 1.0F, 0.0F, &controller.action);
	collision = initControllerData(VK_F1, 1.0F, 0.0F, &controller.collision);
	backCamera = initControllerData(VK_SHIFT, 1.0F, 0.0F, &controller.backCamera);
	resetCamera = initControllerData(VK_SPACE, 1.0F, 0.0F, &controller.resetCamera);
	menu = initControllerEvent(VK_ESCAPE, NULL, menuEvent);
	initControllerEvent(VK_UP, NULL, upArrowEvent);
	initControllerEvent(VK_DOWN, NULL, downArrowEvent);
	enterKey = initControllerEvent(VK_RETURN, NULL, enterKeyEvent);

	hero = loadBitmap("assets/subaru.bmp", NULL_COLOR);
	opponentImage = loadBitmap("assets/subaru2p.bmp", NULL_COLOR);
	course = loadBitmap("assets/courseMk2.bmp", NULL_COLOR);

	raceScene = initScene();
	raceScene.camera.parent = &heroNode;
	raceScene.camera.positionMask[1] = TRUE;
	raceScene.camera.aspect = SCREEN_ASPECT;
	raceScene.camera.farLimit = 3000.0F;
	raceScene.background = BLUE;
	titleScene = raceScene;
	raceScene.camera.isRotationDisabled = TRUE;
	raceScene.behaviour = raceSceneBehaviour;
	titleScene.camera.position[0] = 0.0F;
	titleScene.camera.position[1] = 25.0F;
	titleScene.camera.position[2] = 50.0F;
	pushUntilNull(&titleScene.nodes, &titleNode, &titleMenuNode, &heroNode, &courseNode, &stageNode, NULL);

	courseNode = initNode("course", course);
	initShapeFromObj(&courseNode.shape, "./assets/courseMk2.obj", 1.0F);
	initShapeFromObj(&courseNode.collisionShape, "./assets/courseMk2Collision.obj", 1.0F);
	setVec3(courseNode.scale, 4.0F, XYZ_MASK);
	courseNode.collisionShape.dynamicFriction = 0.8F;
	courseNode.collisionShape.rollingFriction = 0.8F;
	courseNode.collisionMaskActive = COURSE_COLLISIONMASK;

	courseDirtNode = initNode("courseDirt", NO_IMAGE);
	initShapeFromObj(&courseDirtNode.shape, "./assets/courseMk2DirtCollision.obj", 100.0F);
	courseDirtNode.collisionShape = courseDirtNode.shape;
	setVec3(courseDirtNode.scale, 4.0F, XYZ_MASK);
	courseDirtNode.collisionMaskActive = DIRT_COLLISIONMASK;
	courseDirtNode.isVisible = FALSE;
	courseDirtNode.isThrough = TRUE;

	stageImage = loadBitmap("assets/stage.bmp", NULL_COLOR);
	stageNode = initNode("stage", stageImage);
	initShapeFromObj(&stageNode.shape, "./assets/stage.obj", 1.0F);
	stageNode.collisionShape = stageNode.shape;
	setVec3(stageNode.scale, 4.0F, XYZ_MASK);

	heroNode = initNode("Hero", hero);
	initShapeFromObj(&heroNode.shape, "./assets/subaru.obj", 1.0F);
	initShapeFromObj(&heroNode.collisionShape, "./assets/subaruCollision.obj", 500.0F);
	heroNode.isPhysicsEnabled = TRUE;
	setVec3(heroNode.scale, 4.0F, XYZ_MASK);
	heroNode.collisionMaskActive = CAR_COLLISIONMASK;
	heroNode.collisionMaskPassive = CAR_COLLISIONMASK | COURSE_COLLISIONMASK | LAP_COLLISIONMASK | DIRT_COLLISIONMASK;
	heroNode.behaviour = heroBehaviour;
	heroRayNode = initCarRayNode("heroRay");
	addNodeChild(&heroNode, &heroRayNode);

	opponentNode = initNode("opponent", opponentImage);
	initShapeFromObj(&opponentNode.shape, "./assets/subaru.obj", 1.0F);
	initShapeFromObj(&opponentNode.collisionShape, "./assets/subaruCollision.obj", 500.0F);
	setVec3(opponentNode.scale, 4.0F, XYZ_MASK);
	opponentNode.isPhysicsEnabled = TRUE;
	opponentNode.collisionMaskPassive = COURSE_COLLISIONMASK | LAP_COLLISIONMASK;
	opponentNode.behaviour = opponentBehaviour;
	opponentRayNode = initCarRayNode("opponentRay");
	addNodeChild(&opponentNode, &opponentRayNode);

	for(i = 0;i < 3;i++) {
		lapJudgmentNodes[i] = initNode(lapNames[i], NO_IMAGE);
		initShapeFromObj(&lapJudgmentNodes[i].shape, lapNames[i], 100.0F);
		lapJudgmentNodes[i].collisionShape = lapJudgmentNodes[i].shape;
		setVec3(lapJudgmentNodes[i].scale, 4.0F, XYZ_MASK);
		lapJudgmentNodes[i].collisionMaskActive = LAPA_COLLISIONMASK << i;
		lapJudgmentNodes[i].isVisible = FALSE;
		lapJudgmentNodes[i].isThrough = TRUE;
	}

	titleNode =	initNodeText("title", CENTER_X(104), CENTER_Y(16) - 16, 104, 16, NULL);
	drawTextSJIS(titleNode.texture, shnm16b, 0, 0, "COARSE RACING");
	titleMenuNode =	initNodeText("titleMenu", CENTER_X(66), CENTER_Y(12) + 12, 66, 36, titleMenuBehaviour);
	timeNode = initNodeText("time", -78.0F, 0.0F, 78, 12, timeBehaviour);
	speedNode = initNodeText("speed", -60.0F, 12.0F, 60, 12, speedBehaviour);
	lapNode = initNodeText("lap", 0.0F, 0.0F, 42, 12, lapBehaviour);
	rankNode = initNodeText("rank", 0.0F, 12.0F, 36, 12, rankBehaviour);
	centerNode = initNodeText("center", 62.0F, 56.0F, 56, 16, centerBehaviour);
	centerNode.isVisible = FALSE;
	replayNode = initNodeText("replay", -48.0F, -16.0F, 48, 16, replayBehaviour);
	drawTextSJIS(replayNode.texture, shnm16b, 0, 0, "REPLAY");

	mapNode = initNodeText("map", 0.0F, 24.0F, 128.0F, 128.0F, mapBehaviour);
	divVec3ByScalar(mapNode.scale, 4.0F, mapNode.scale);

	menuScene = initScene();
	menuNode = initNodeText("menu", 6.0F, CENTER_Y(36), 66, 36, menuBehaviour);
	push(&menuScene.nodes, &menuNode);

	mapScene = initScene();
	mapScene.camera = initCamera(0.0F, 1000.0F, 0.0F, SCREEN_ASPECT);
	clearVec3(mapScene.camera.worldUp);
	mapScene.camera.worldUp[0] = -1.0F;
	mapScene.camera.farLimit = 1000.0F;

	courseMapNode = courseNode;
	courseMapNode.texture = initImage(1, 1, WHITE, NULL_COLOR);
	push(&mapScene.nodes, &courseMapNode);

	heroMarkerNode = initNode("heroMarker", NO_IMAGE);
	heroMarkerNode.shape = initShapeBox(250.0F, 250.0F, 250.0F, DARK_BLUE, 1.0F);
	push(&mapScene.nodes, &heroMarkerNode);

	opponentMarkerNode = initNode("opponentMarker", NO_IMAGE);
	opponentMarkerNode.shape = initShapeBox(250.0F, 250.0F, 250.0F, DARK_RED, 1.0F);
	push(&mapScene.nodes, &opponentMarkerNode);

	loadRecord(&opponentRecords, "./carRecords/record.crd");
	loadRecord(&titleRecords, "./carRecords/title.crd");
}

void deinitGame(void) {
	discardShape(heroNode.shape);
	discardShape(courseNode.shape);
	discardShape(courseNode.collisionShape);
	discardNode(speedNode);
	discardNode(heroNode);
	discardNode(courseNode);
	freeImage(shnm12.font0201);
	freeImage(shnm12.font0208);
	freeImage(hero);
	discardScene(&raceScene);
}

int loop(float elapsed, Image *out, int sleep) {
	Image sceneImage;
	float menuTransitionTemp;
	if(!isMenuOpened) {
		if(isTitle) {
			updateScene(&titleScene, elapsed);
		} else {
			updateScene(&raceScene, elapsed / 2.0F);
			updateScene(&raceScene, elapsed / 2.0F);
		}
	} else {
		updateScene(&menuScene, elapsed);
	}
	if(controller.backCamera) {
		raceScene.camera.position[0] *= -1.0F;
		raceScene.camera.position[2] *= -1.0F;
	}
	sceneImage = isTitle ? drawScene(&titleScene) : drawScene(&raceScene);
	menuTransitionTemp = menuTransition < 0.1F ? 0.0F : 0.5F * menuTransition;
	if(menuTransitionTemp == 0.0F) {
		*out = sceneImage;
	} else {
		Image menuImage;
		menuImage = drawScene(&menuScene);
		*out = ThreeDimensionTransition(sceneImage, menuImage, 0.75F, 0.0F, menuTransitionTemp);
		freeImage(menuImage);
		freeImage(sceneImage);
	}
	if(controller.backCamera) {
		raceScene.camera.position[0] *= -1.0F;
		raceScene.camera.position[2] *= -1.0F;
	}
	menuTransition += 0.75F * (isMenuOpened - menuTransition);
	return TRUE;
}

// item.
// effect, animation

int WINAPI ctrlCHandler(DWORD dwCtrlType) {
	quitGame();
	return TRUE;
}

int main(int argc, char *argv[]) {
	initCNSG(argc, argv, SCREEN_WIDTH, SCREEN_HEIGHT);
	SetConsoleCtrlHandler(ctrlCHandler, TRUE);
	initGame();
	freeImage(drawScene(&raceScene));
	startTitle();
	gameLoop(FRAME_PER_SECOND, loop);
	deinitGame();
	deinitCNSG();
	return 0;
}
