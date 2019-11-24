#include<stdio.h>
#include<math.h>
#include<time.h>
#include<Windows.h>

#include "./cnsglib/include/cnsg.h"

#include "./include/record.h"

#define SCREEN_HEIGHT 128
#define SCREEN_WIDTH 180 // 1.414 * SCREEN_HEIGHT
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
	float arrow[2];
	float collision;
	float backCamera;
	float resetCamera;
} controller;

static FontSJIS shnm12;
static FontSJIS shnm16b;

static int titleMenuEntered;
static TitleMenuItem titleMenuSelect;
static ControllerData *wasd[4], *collision, *menu, *arrow[4], *backCamera, *resetCamera;
static ControllerData *enterKey;
static MenuItem menuSelect;

static Image hero, opponentImage;
static Image course;
static Image stageImage;

static Scene titleScene;
static Node titleNode, titleMenuNode;

static Scene raceScene;
static Camera camera2P;
static Node speedNode, lapNode, rankNode, finishNode, centerNode, timeNode, replayNode, wrongWayNode;
static Node speed2pNode, lap2pNode, rank2pNode;
static Node mapNode;
static Node lapJudgmentNodes[3];
static Node heroNode;
static Node opponentNode;
static Node heroMarkerNode, opponentMarkerNode;
static Node courseNode, courseMapNode, courseDirtNode;
static Node stageNode;

static Scene menuScene;
static Node menuNode;

static Scene mapScene;

static float nextCameraPosition[3], opponentNextCameraPosition[3];
static float currentCameraPosition[3];
static float cameraAngle, cameraFov;
static int heroLapScore, opponentLapScore;
static int heroPreviousLap = -1, opponentPreviousLap = -1;
static int collisionFlag;
static int is2p;
static int isTitle, isRaceStarted, isFinished, isHeroFinished, isOpponentFinished, isReplaying;
static int isWrongWay, isWrongWay2p;
static int isMenuOpened;
static float menuTransition;
static float finishedTime;
static float currentTime;
static float heroAccel, opponentAccel;
static char rankList[][4] = { "1st", "2nd" };

static Vector titleRecords, heroRecords, opponentRecords, opponent2pRecords;

static Sound bgm;

void deinitGame(void);

static void quitGame(void) {
	deinitGame();
	deinitCNSG();
	ExitProcess(0);
}

static void replay(void) {
	isWrongWay = FALSE;
	if(is2p) {
		isWrongWay2p = FALSE;
	} else {
		opponentNode.isVisible = TRUE;
		opponentMarkerNode.isVisible = TRUE;
		resetIteration(&opponentRecords);
	}
	raceScene.clock = 0.0F;
}

static float controlCar(Node *node, float accel, float handle, float front[3]) {
	float tempVec3[3][3];
	float tempMat3[1][3][3];
	float tempMat4[1][4][4];
	float velocityLengthH;
	float angle;
	if(node->collisionFlags & DIRT_COLLISIONMASK) {
		node->collisionShape.dynamicFriction = 1.0F;
	} else {
		node->collisionShape.dynamicFriction = 0.1F;
	}
	convMat4toMat3(genRotationMat4(node->angle[0], node->angle[1], node->angle[2], tempMat4[0]), tempMat3[0]);
	initVec3(tempVec3[0], Y_MASK);
	mulMat3Vec3(tempMat3[0], tempVec3[0], tempVec3[1]);
	initVec3(tempVec3[0], Y_MASK);
	angle =	cosVec3(tempVec3[0], tempVec3[1]);
	if(angle < 0.75F) {
		accel = 0.0F;
		handle = 0.0F;
	}
	initVec3(tempVec3[0], Z_MASK);
	mulMat3Vec3(tempMat3[0], tempVec3[0], front);
	normalize3(extractComponents3(front, XZ_MASK, tempVec3[0]), tempVec3[1]);
	extractComponents3(node->velocity, XZ_MASK, tempVec3[0]);
	velocityLengthH = length3(tempVec3[0]);
	mulVec3ByScalar(tempVec3[1], velocityLengthH * node->collisionShape.mass + accel * 150000.0F, tempVec3[0]);
	subVec3(tempVec3[0], mulVec3ByScalar(node->velocity, node->collisionShape.mass, tempVec3[2]), tempVec3[1]);
	applyForce(node, tempVec3[1], XZ_MASK, FALSE);
	if(velocityLengthH > 1.0F) node->torque[1] += handle * max(velocityLengthH * 5000.0F, 2500000.0F);
	return angle;
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
			if(isReplaying) {
				if(playCar(&heroRecords, node, currentTime)) replay();
			} else {
				recordCar(&heroRecords, node, currentTime);
				if(node->collisionFlags & COURSE_COLLISIONMASK) {
					float upper[3] = { 0.0F, 25.0F, 0.0F };
					if(heroLapScore >= 3 * 45) {
						isHeroFinished = TRUE;
						controlCar(node, 0, 0, front);
					} else {
						if(controlCar(node, heroAccel, controller.move[0], front) < 0.0F) {
							node->angle[0] = 0.0F;
							node->angle[2] = 0.0F;
							node->angVelocity[0] = 0.0F;
							node->angVelocity[2] = 0.0F;
							node->angMomentum[0] = 0.0F;
							node->angMomentum[2] = 0.0F;
						}
					}
					mulVec3ByScalar(front, -75.0F, nextCameraPosition);
					addVec3(nextCameraPosition, upper, nextCameraPosition);
				}
			}
		}
		isWrongWay = updateLapScore(node->collisionFlags, &heroPreviousLap, &heroLapScore, isWrongWay);
		memcpy_s(heroMarkerNode.position, SIZE_VEC3, node->position, SIZE_VEC3);
		memcpy_s(heroMarkerNode.angle, SIZE_VEC3, node->angle, SIZE_VEC3);
	}
	return TRUE;
}

static int opponentBehaviour(Node *node) {
	if(isRaceStarted) {
		if(is2p) {
			if(isReplaying) {
				playCar(&opponent2pRecords, node, currentTime);
			} else {
				recordCar(&opponent2pRecords, node, currentTime);
				if(node->collisionFlags & COURSE_COLLISIONMASK) {
					float upper[3] = { 0.0F, 25.0F, 0.0F };
					float front[3];
					if(opponentLapScore >= 3 * 45) {
						isOpponentFinished = TRUE;
						controlCar(node, 0.0F, 0.0F, front);
					} else {
						if(controlCar(node, opponentAccel, controller.arrow[0], front) < 0.0F) {
							node->angle[0] = 0.0F;
							node->angle[2] = 0.0F;
							node->angVelocity[0] = 0.0F;
							node->angVelocity[2] = 0.0F;
							node->angMomentum[0] = 0.0F;
							node->angMomentum[2] = 0.0F;
						}
					}
					mulVec3ByScalar(front, -75.0F, opponentNextCameraPosition);
					addVec3(opponentNextCameraPosition, upper, opponentNextCameraPosition);
				}
			}
		} else {
			if(playCar(&opponentRecords, node, currentTime)) {
				node->isVisible = FALSE;
				opponentMarkerNode.isVisible = FALSE;
			}
		}
	}
	isWrongWay2p = updateLapScore(node->collisionFlags, &opponentPreviousLap, &opponentLapScore, isWrongWay2p);
	memcpy_s(opponentMarkerNode.position, SIZE_VEC3, node->position, SIZE_VEC3);
	memcpy_s(opponentMarkerNode.angle, SIZE_VEC3, node->angle, SIZE_VEC3);
	return TRUE;
}

static int timeBehaviour(Node *node) {
	if((!is2p && isHeroFinished) || (isHeroFinished && isOpponentFinished)) {
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

static void speedBehaviourCommon(Node *node, Node *car) {
	char buffer[11];
	float speed = convVelocityToSpeed(car->velocity);
	if(speed >= 1000.0F) {
		sprintf(buffer, "???.? km/h");
	} else {
		sprintf(buffer, "%5.1f km/h", speed);
	}
	drawTextSJIS(node->texture, shnm12, 0, 0, buffer);
}

static int speedBehaviour(Node *node) {
	speedBehaviourCommon(node, &heroNode);
	return TRUE;
}

static int speed2pBehaviour(Node *node) {
	speedBehaviourCommon(node, &opponentNode);
	return TRUE;
}

static void lapBehaviourCommon(Node *node, int score) {
	char buffer[8];
	int lap = min(max(score / 45 + 1, 1), 3);
	sprintf(buffer, "LAP %d/3", lap);
	drawTextSJIS(node->texture, shnm12, 0, 0, buffer);
}

static int lapBehaviour(Node *node) {
	if(!isHeroFinished) lapBehaviourCommon(node, heroLapScore);
	return TRUE;
}

static int lap2pBehaviour(Node *node) {
	if(!isOpponentFinished) lapBehaviourCommon(node, opponentLapScore);
	return TRUE;
}

static void rankBehaviourCommon(Node *node, int *rank, int scoreA, int scoreB) {
	if(!(isHeroFinished || isOpponentFinished)) {
		if(scoreA != scoreB) {
			*rank = scoreA > scoreB ? 0 : 1;
		}
		drawTextSJIS(node->texture, shnm12, 0, 0, rankList[*rank]);
	}
}

static int rankBehaviour(Node *node) {
	static int rank = 0;
	rankBehaviourCommon(node, &rank, heroLapScore, opponentLapScore);
	return TRUE;
}

static int rank2pBehaviour(Node *node) {
	static int rank = 1;
	rankBehaviourCommon(node, &rank, opponentLapScore, heroLapScore);
	return TRUE;
}

static void raceStarted(void) {
	bgm = PlaySoundNeo("./assets/bgm.wav", TRUE);
}

static int centerBehaviour(Node *node) {
	float elapsed = 4.0F - raceScene.clock;
	if(isRaceStarted) {
		if(elapsed > 0.0F) {
			drawTextSJIS(node->texture, shnm16b, 0, 0, "START! ");
		} else {
			node->isVisible = FALSE;
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

static int mapBehaviour(Node *node) {
	drawScene(&mapScene, &node->texture);
	return TRUE;
}

static void startGame(void) {
	isTitle = FALSE;
	if(bgm) StopSound(bgm);

	cameraAngle = 0.0F;
	cameraFov = PI / 3.0F * 1.5F;
	isFinished = FALSE;
	isHeroFinished = FALSE;
	isOpponentFinished = FALSE;
	isRaceStarted = FALSE;
	isReplaying = FALSE;
	isWrongWay = FALSE;
	isWrongWay2p = FALSE;
	heroLapScore = 0;
	heroPreviousLap = -1;
	opponentLapScore = 0;
	opponentPreviousLap = -1;

	freeVector(&heroRecords);

	currentCameraPosition[0] = 0.0F;
	currentCameraPosition[1] = 25.0F;
	currentCameraPosition[2] = -75.0F;
	memcpy_s(nextCameraPosition, SIZE_VEC3, currentCameraPosition, SIZE_VEC3);
	if(is2p) {
		freeVector(&opponent2pRecords);
		timeNode.position[0] = -timeNode.scale[0] / 2.0F;
		mapNode.position[0] = -mapNode.scale[0] / 2.0F;
		opponentNode.collisionMaskActive = CAR_COLLISIONMASK;
		opponentNode.collisionMaskPassive = DIRT_COLLISIONMASK | CAR_COLLISIONMASK | COURSE_COLLISIONMASK | LAP_COLLISIONMASK;
		memcpy_s(camera2P.position, SIZE_VEC3, currentCameraPosition, SIZE_VEC3);
		memcpy_s(opponentNextCameraPosition, SIZE_VEC3, camera2P.position, SIZE_VEC3);
	} else {
		resetIteration(&opponentRecords);
		timeNode.position[0] = 0.0F;
		timeNode.interfaceAlign[0] = RIGHT;
		mapNode.position[0] = 0.0F;
		mapNode.interfaceAlign[0] = LEFT;
		opponentNode.collisionMaskActive = 0;
		opponentNode.collisionMaskPassive = DIRT_COLLISIONMASK | COURSE_COLLISIONMASK | LAP_COLLISIONMASK;
	}

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
	pushUntilNull(&raceScene.nodes, &speedNode, &lapNode, &rankNode, &wrongWayNode, &centerNode, &timeNode, &replayNode, &finishNode, NULL);
	if(is2p) pushUntilNull(&raceScene.nodes, &speed2pNode, &lap2pNode, &rank2pNode, NULL);
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
	if(isTitle) titleMenuSelect = titleMenuSelect <= 0 ? 2 : titleMenuSelect - 1;
	if(isMenuOpened) menuSelect = menuSelect <= 0 ? 2 : menuSelect - 1;
}

static void downArrowEvent(void) {
	if(isTitle) titleMenuSelect = titleMenuSelect >= 2 ? 0 : titleMenuSelect + 1;
	if(isMenuOpened) menuSelect = menuSelect >= 2 ? 0 : menuSelect + 1;
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
					is2p = FALSE;
					startGame();
					break;
				case TWOP_PLAY:
					is2p = TRUE;
					startGame();
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
			courseDirtNode.isVisible = !courseDirtNode.isVisible;
			collisionFlag = TRUE;
		}
	} else {
		collisionFlag = FALSE;
	}
	if(isFinished) {
		if(!isReplaying) {
			if(raceScene.clock - finishedTime > 3.0F) {
				replay();
				isReplaying = TRUE;
			}
		}
	} else {
		if((!is2p && isHeroFinished) || (isHeroFinished && isOpponentFinished)) {
			isFinished = TRUE;
			finishedTime = raceScene.clock;
		}
	}
	if(!isReplaying) {
		heroAccel += 0.3F * (controller.move[1] - heroAccel);
		opponentAccel += 0.3F * (controller.arrow[1] - opponentAccel);
	}
	if(is2p) {
		cameraFov = PI / 3.0F * 1.5F;
		cameraAngle = 0.0F;
	} else {
		cameraFov = min(PI / 3.0F * 1.5F, max(PI / 3.0F, cameraFov - 0.05F * controller.arrow[1]));
		cameraAngle -= 0.1F * controller.arrow[0];
	}
	raceScene.camera.fov = cameraFov + heroAccel / 5.0F;
	camera2P.fov = cameraFov + opponentAccel / 5.0F;
	addVec3(currentCameraPosition, mulVec3ByScalar(subVec3(nextCameraPosition, currentCameraPosition, tempVec3[0]), 2.0F * elapsed, tempVec3[1]), currentCameraPosition);
	addVec3(camera2P.position, mulVec3ByScalar(subVec3(opponentNextCameraPosition, camera2P.position, tempVec3[0]), 2.0F * elapsed, tempVec3[1]), camera2P.position);
	if(controller.resetCamera) cameraAngle -= 0.3F * cameraAngle;
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
	raceScene.camera.farLimit = 3000.0F;
	raceScene.background = BLUE;
	titleScene = raceScene;
	raceScene.camera.isRotationDisabled = TRUE;
	raceScene.behaviour = raceSceneBehaviour;

	camera2P = raceScene.camera;
	camera2P.parent = &opponentNode;

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

	heroNode = initNode("hero", hero);
	initShapeFromObj(&heroNode.shape, "./assets/subaru.obj", 1.0F);
	initShapeFromObj(&heroNode.collisionShape, "./assets/subaruCollision.obj", 500.0F);
	heroNode.isPhysicsEnabled = TRUE;
	setVec3(heroNode.scale, 4.0F, XYZ_MASK);
	heroNode.collisionMaskActive = CAR_COLLISIONMASK;
	heroNode.collisionMaskPassive = CAR_COLLISIONMASK | COURSE_COLLISIONMASK | LAP_COLLISIONMASK | DIRT_COLLISIONMASK;
	heroNode.behaviour = heroBehaviour;

	opponentNode = initNode("opponent", opponentImage);
	opponentNode.shape = heroNode.shape;
	opponentNode.collisionShape = heroNode.collisionShape;
	setVec3(opponentNode.scale, 4.0F, XYZ_MASK);
	opponentNode.isPhysicsEnabled = TRUE;
	opponentNode.behaviour = opponentBehaviour;

	for(i = 0;i < 3;i++) {
		lapJudgmentNodes[i] = initNode(lapNames[i], NO_IMAGE);
		initShapeFromObj(&lapJudgmentNodes[i].shape, lapNames[i], 100.0F);
		lapJudgmentNodes[i].collisionShape = lapJudgmentNodes[i].shape;
		setVec3(lapJudgmentNodes[i].scale, 4.0F, XYZ_MASK);
		lapJudgmentNodes[i].collisionMaskActive = LAPA_COLLISIONMASK << i;
		lapJudgmentNodes[i].isVisible = FALSE;
		lapJudgmentNodes[i].isThrough = TRUE;
	}

	titleNode =	initNodeText("title", 0.0F, -16.0F, CENTER, CENTER, 104, 16, NULL);
	drawTextSJIS(titleNode.texture, shnm16b, 0, 0, "COARSE RACING");
	titleMenuNode =	initNodeText("titleMenu", 0.0F, 22.0F, CENTER, CENTER, 66, 36, titleMenuBehaviour);
	timeNode = initNodeText("time", 0.0F, 0.0F, RIGHT, TOP, 78, 12, timeBehaviour);
	speedNode = initNodeText("speed", 0.0F, 12.0F, RIGHT, TOP, 60, 12, speedBehaviour);
	speed2pNode = initNodeText("speed2p", 0.0F, 12.0F, RIGHT, TOP, 60, 12, speed2pBehaviour);
	speed2pNode.isVisible = FALSE;
	lapNode = initNodeText("lap", 0.0F, 0.0F, LEFT, TOP, 42, 12, lapBehaviour);
	lap2pNode = initNodeText("lap2p", 0.0F, 0.0F, LEFT, TOP, 42, 12, lap2pBehaviour);
	lap2pNode.isVisible = FALSE;
	lap2pNode.interfaceAlign[0] = RIGHT;
	rankNode = initNodeText("rank", 0.0F, 12.0F, LEFT, TOP, 36, 12, rankBehaviour);
	rank2pNode = initNodeText("rank2p", 0.0F, 12.0F, LEFT, TOP, 36, 12, rank2pBehaviour);
	rank2pNode.isVisible = FALSE;
	finishNode = initNodeText("finished", 0.0F, 0.0F, CENTER, CENTER, 56, 16, NULL);
	finishNode.isVisible = FALSE;
	drawTextSJIS(finishNode.texture, shnm16b, 0, 0, "FINISH!");
	centerNode = initNodeText("center", 0.0F, 0.0F, CENTER, CENTER, 56, 16, centerBehaviour);
	centerNode.isVisible = FALSE;
	wrongWayNode = initNodeText("wrongWay", 0.0F, 0.0F, CENTER, CENTER, 40, 16, NULL);
	wrongWayNode.isVisible = FALSE;
	drawTextSJIS(wrongWayNode.texture, shnm16b, 0, 0, "逆走!");
	replayNode = initNodeText("replay", 0.0F, 0.0F, RIGHT, BOTTOM, 48, 16, NULL);
	drawTextSJIS(replayNode.texture, shnm16b, 0, 0, "REPLAY");

	mapNode = initNodeText("map", 0.0F, 24.0F, LEFT, TOP, 32, 32, mapBehaviour);
	mapNode.texture.transparent = BLACK;

	menuScene = initScene();
	menuNode = initNodeText("menu", 6.0F, 0.0F, LEFT, CENTER, 66, 36, menuBehaviour);
	push(&menuScene.nodes, &menuNode);

	mapScene = initScene();
	mapScene.camera = initCamera(0.0F, 1000.0F, 0.0F, 0.0F);
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
	Image sceneImage = initImageBulk(SCREEN_WIDTH, SCREEN_HEIGHT, NULL_COLOR);
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
	if(isTitle) {
		drawScene(&titleScene, &sceneImage);
	} else {
		Image halfScreen;
		if(is2p) {
			halfScreen = initImageBulk(SCREEN_WIDTH / 2, SCREEN_HEIGHT, NULL_COLOR);
			speedNode.isVisible = FALSE;
			speed2pNode.isVisible = TRUE;
			lapNode.isVisible = FALSE;
			lap2pNode.isVisible = TRUE;
			rankNode.isVisible = FALSE;
			rank2pNode.isVisible = TRUE;
			mapNode.interfaceAlign[0] = LEFT;
			timeNode.interfaceAlign[0] = LEFT;
			wrongWayNode.isVisible = isWrongWay2p & !(isOpponentFinished ^ isReplaying);
			finishNode.isVisible = isOpponentFinished & !isReplaying;
			replayNode.isVisible = isReplaying;
			drawSceneWithCamera(&raceScene, &halfScreen, &camera2P);
			pasteImage(sceneImage, halfScreen, SCREEN_WIDTH / 2, 0);
			speedNode.isVisible = TRUE;
			speed2pNode.isVisible = FALSE;
			lapNode.isVisible = TRUE;
			lap2pNode.isVisible = FALSE;
			rankNode.isVisible = TRUE;
			rank2pNode.isVisible = FALSE;
			mapNode.interfaceAlign[0] = RIGHT;
			timeNode.interfaceAlign[0] = RIGHT;
			wrongWayNode.isVisible = isWrongWay & !(isHeroFinished ^ isReplaying);
			finishNode.isVisible = isHeroFinished & !isReplaying;
			replayNode.isVisible = FALSE;
			drawScene(&raceScene, &halfScreen);
			pasteImage(sceneImage, halfScreen, 0, 0);
		} else {
			wrongWayNode.isVisible = isWrongWay;
			finishNode.isVisible = isHeroFinished & !isReplaying;
			replayNode.isVisible = isReplaying;
			drawScene(&raceScene, &sceneImage);
		}
	}
	menuTransitionTemp = menuTransition < 0.1F ? 0.0F : 0.5F * menuTransition;
	if(menuTransitionTemp == 0.0F) {
		*out = sceneImage;
	} else {
		Image menuImage = initImageBulk(SCREEN_WIDTH, SCREEN_HEIGHT, NULL_COLOR);
		drawScene(&menuScene, &menuImage);
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
	drawScene(&raceScene, &NO_IMAGE);
	startTitle();
	gameLoop(FRAME_PER_SECOND, loop);
	deinitGame();
	deinitCNSG();
	return 0;
}
