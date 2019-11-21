#include<stdio.h>
#include<math.h>
#include<time.h>
#include<Windows.h>

#include "./cnsglib/include/cnsg.h"

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

typedef struct {
	float time;
	float position[3];
	float angle[3];
	float velocity[3];
	float angVelocity[3];
	float angMomentum[3];
} CarRecord;

#pragma pack(1)
typedef struct {
	float time;
	float position[3];
	float angle[3];
	float velocity[3];
	float angVelocity[3];
	float angMomentum[3];
	char reserved[36];
} CarRecordForSave;
#pragma pack()

typedef enum {
	GO_TO_TITLE, RETRY, QUIT
} MenuItem;

static struct {
	float move[2];
	float arrow[4];
	float direction[2];
	float action;
	float collision;
	float quit;
	float backCamera;
	float resetCamera;
} controller;

static FontSJIS shnm12;
static FontSJIS shnm16b;

static Controller keyboard, menuController;
static ControllerData wasd[4], action, collision, quit, arrow[4], backCamera, resetCamera;
static ControllerData quitMenu, downArrow, upArrow, enterKey;
static int menuSelect;

static Image hero, opponentImage;
static Image course;
static Image stageImage;

static Scene scene;
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
static int isRaceStarted, isfinished, isReplaying;
static int isMenuOpened;
static float menuTransition;
static int quitPrevious;
static float finishedTime;
static float currentTime;
static float handle, accel;
static float heroAngle;

static Vector heroRecords, opponentRecords;

static Sound bgm;

static void addCarRecord(Vector *records, float time, float position[3], float angle[3], float velocity[3], float angVelocity[3], float angMomentum[3]) {
	CarRecord record;
	record.time = time;
	memcpy_s(record.position, SIZE_VEC3, position, SIZE_VEC3);
	memcpy_s(record.angle, SIZE_VEC3, angle, SIZE_VEC3);
	memcpy_s(record.velocity, SIZE_VEC3, velocity, SIZE_VEC3);
	memcpy_s(record.angVelocity, SIZE_VEC3, angVelocity, SIZE_VEC3);
	memcpy_s(record.angMomentum, SIZE_VEC3, angMomentum, SIZE_VEC3);
	pushAlloc(records, sizeof(CarRecord), &record);
}

static void recordCar(Vector *records, Node *node, float time) {
	if(records->length < 36000) addCarRecord(records, time, node->position, node->angle, node->velocity, node->angVelocity, node->angMomentum);
}

static int playCar(Vector *records, Node *node, float time) {
	CarRecord *record;
	record = nextData(records);
	if(record == NULL) return -1;
	if(record->time > time) {
		previousData(records);
	} else {
		memcpy_s(node->position, SIZE_VEC3, record->position, SIZE_VEC3);
		memcpy_s(node->angle, SIZE_VEC3, record->angle, SIZE_VEC3);
		memcpy_s(node->velocity, SIZE_VEC3, record->velocity, SIZE_VEC3);
		memcpy_s(node->angVelocity, SIZE_VEC3, record->angVelocity, SIZE_VEC3);
		memcpy_s(node->angMomentum, SIZE_VEC3, record->angMomentum, SIZE_VEC3);
		while(record->time > time) record = nextData(records);
	}
	return 0;
}

static void saveRecord(Vector *records, const char path[]) {
	FILE *file;
	CarRecord *record;
	if(fopen_s(&file, path, "wb")) {
		printf("saveRecord: Failed to open a file: %s\n", path);
	} else {
		iterf(records, &record) {
			CarRecordForSave data;
			data.time = record->time;
			memcpy_s(data.position, SIZE_VEC3, record->position, SIZE_VEC3);
			memcpy_s(data.angle, SIZE_VEC3, record->angle, SIZE_VEC3);
			memcpy_s(data.velocity, SIZE_VEC3, record->velocity, SIZE_VEC3);
			memcpy_s(data.angVelocity, SIZE_VEC3, record->angVelocity, SIZE_VEC3);
			memcpy_s(data.angMomentum, SIZE_VEC3, record->angMomentum, SIZE_VEC3);
			fwrite(&data, sizeof(CarRecordForSave), 1, file);
		}
		fclose(file);
	}
}

static void loadRecord(Vector *records, const char path[]) {
	FILE *file;
	*records = initVector();
	if(fopen_s(&file, path, "rb")) {
		printf("loadRecord: Failed to open a file: %s\n", path);
	} else {
		CarRecordForSave data;
		size_t count;
		for(count = fread_s(&data, sizeof(CarRecordForSave), sizeof(CarRecordForSave), 1, file);count == 1;count = fread_s(&data, sizeof(CarRecordForSave), sizeof(CarRecordForSave), 1, file)) {
			addCarRecord(records, data.time, data.position, data.angle, data.velocity, data.angVelocity, data.angMomentum);
		}
		fclose(file);
	}
}

static void replay() {
	opponentNode.isVisible = TRUE;
	opponentMarkerNode.isVisible = TRUE;
	scene.clock = 0.0F;
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

static void updateLapScore(unsigned int flags, int *previousLap, int *lapScore) {
	if(flags & LAPA_COLLISIONMASK) {
		if(*previousLap == 2) {
			*lapScore += 1;
		} else if(*previousLap == 1) {
			*lapScore -= 1;
		}
		*previousLap = 0;
	}
	if(flags & LAPB_COLLISIONMASK) {
		if(*previousLap == 0) {
			*lapScore += 1;
		} else if(*previousLap == 2) {
			*lapScore -= 1;
		}
		*previousLap = 1;
	}
	if(flags & LAPC_COLLISIONMASK) {
		if(*previousLap == 1) {
			*lapScore += 1;
		} else if(*previousLap == 0) {
			*lapScore -= 1;
		}
		*previousLap = 2;
	}
}

static int heroBehaviour(Node *node) {
	float front[3];
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
	updateLapScore(node->collisionFlags, &heroPreviousLap, &heroLapScore);
	memcpy_s(heroMarkerNode.position, SIZE_VEC3, node->position, SIZE_VEC3);
	memcpy_s(heroMarkerNode.angle, SIZE_VEC3, node->angle, SIZE_VEC3);
	return TRUE;
}

static int opponentBehaviour(Node *node) {
	if(isRaceStarted) {
		if(playCar(&opponentRecords, node, currentTime)) {
			node->isVisible = FALSE;
			opponentMarkerNode.isVisible = FALSE;
		}
	}
	updateLapScore(node->collisionFlags, &opponentPreviousLap, &opponentLapScore);
	memcpy_s(opponentMarkerNode.position, SIZE_VEC3, node->position, SIZE_VEC3);
	memcpy_s(opponentMarkerNode.angle, SIZE_VEC3, node->angle, SIZE_VEC3);
	return TRUE;
}

static int timeBehaviour(Node *node) {
	if(isfinished) {
		currentTime = scene.clock;
	} else {
		if(isRaceStarted) {
			char buffer[14];
			int minutes;
			float seconds;
			currentTime = scene.clock - 3.0F;
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
			if(scene.clock - finishedTime > 3.0F) {
				replay();
				isReplaying = TRUE;
			}
		}
	} else {
		char buffer[8];
		int lap = max(heroLapScore / 45 + 1, 1);
		if(lap > 3) {
			isfinished = TRUE;
			finishedTime = scene.clock;
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
	float elapsed = 4.0F - scene.clock;
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
				node->isVisible = FALSE;
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

	clearVector(&scene.nodes);
	pushUntilNull(&scene.nodes, &speedNode, &lapNode, &rankNode, &centerNode, &timeNode, &replayNode, NULL);
	pushUntilNull(&scene.nodes, &mapNode, NULL);
	pushUntilNull(&scene.nodes, &lapJudgmentNodes[0], &lapJudgmentNodes[1], &lapJudgmentNodes[2], NULL);
	pushUntilNull(&scene.nodes, &heroNode, &opponentNode, NULL);
	pushUntilNull(&scene.nodes, &courseNode, &courseDirtNode, NULL);
	push(&scene.nodes, &stageNode);

	scene.clock = 0.0F;
}

static void upArrowEvent(void) {
	menuSelect = menuSelect <= 0 ? 2 : menuSelect - 1;
}

static void downArrowEvent(void) {
	menuSelect = menuSelect >= 2 ? 0 : menuSelect + 1;
}

static void enterKeyEvent(void) {
	switch(menuSelect) {
		case RETRY:
			isMenuOpened = FALSE;
			startGame();
			break;
	}
}

static void sceneBehaviour(Scene *_scene, float elapsed) {
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
	scene.camera.fov = cameraFov + accel / 5.0F;
	addVec3(currentCameraPosition, mulVec3ByScalar(subVec3(nextCameraPosition, currentCameraPosition, tempVec3[0]), 2.0F * elapsed, tempVec3[1]), currentCameraPosition);
	cameraAngle -= 0.1F * controller.arrow[0];
	if(controller.resetCamera) cameraAngle -= 0.3F * cameraAngle;
	handle += 0.3F * (controller.move[0] - handle);
	genRotationMat4(0.0F, cameraAngle, 0.0F, tempMat4[0]);
	mulMat3Vec3(convMat4toMat3(tempMat4[0], tempMat3[0]), currentCameraPosition, scene.camera.position);
}

static void initGame(void) {
	int i;
	char lapNames[3][50] = { "./assets/courseMk2CollisionA.obj", "./assets/courseMk2CollisionB.obj", "./assets/courseMk2CollisionC.obj" };
	shnm12 = initFontSJIS(loadBitmap("assets/shnm6x12r.bmp", NULL_COLOR), loadBitmap("assets/shnmk12.bmp", NULL_COLOR), 6, 12, 12);
	shnm16b = initFontSJIS(loadBitmap("assets/shnm8x16rb.bmp", NULL_COLOR), loadBitmap("assets/shnmk16b.bmp", NULL_COLOR), 8, 16, 16);

	keyboard = initController();
	initControllerDataCross(wasd, 'W', 'A', 'S', 'D', controller.move);
	initControllerDataCross(arrow, VK_UP, VK_LEFT, VK_DOWN, VK_RIGHT, controller.arrow);
	action = initControllerData(VK_SPACE, 1.0F, 0.0F, &controller.action);
	collision = initControllerData(VK_F1, 1.0F, 0.0F, &controller.collision);
	backCamera = initControllerData(VK_SHIFT, 1.0F, 0.0F, &controller.backCamera);
	resetCamera = initControllerData(VK_SPACE, 1.0F, 0.0F, &controller.resetCamera);
	quit = initControllerData(VK_ESCAPE, 1.0F, 0.0F, &controller.quit);
	pushUntilNull(&keyboard.events, &wasd[0], &wasd[1], &wasd[2], &wasd[3], NULL);
	pushUntilNull(&keyboard.events, &arrow[0], &arrow[1], &arrow[2], &arrow[3], NULL);
	pushUntilNull(&keyboard.events, &action, &quit, &collision, &backCamera, &resetCamera, NULL);

	menuController = initController();
	quitMenu = initControllerData(VK_ESCAPE, 1.0F, 0.0F, NULL);
	upArrow = initControllerEvent(VK_UP, NULL, upArrowEvent);
	downArrow = initControllerEvent(VK_DOWN, NULL, downArrowEvent);
	enterKey = initControllerEvent(VK_RETURN, NULL, enterKeyEvent);
	pushUntilNull(&menuController.events, &quitMenu, &upArrow, &downArrow, &enterKey, NULL);

	hero = loadBitmap("assets/subaru.bmp", NULL_COLOR);
	opponentImage = loadBitmap("assets/subaru2p.bmp", NULL_COLOR);
	course = loadBitmap("assets/courseMk2.bmp", NULL_COLOR);
	scene = initScene();
	scene.camera.parent = &heroNode;
	scene.camera.positionMask[1] = TRUE;
	scene.camera.aspect = SCREEN_ASPECT;
	scene.camera.isRotationDisabled = TRUE;
	scene.camera.farLimit = 3000.0F;
	scene.background = BLUE;
	scene.behaviour = sceneBehaviour;

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
	menuNode = initNodeText("quit", 6.0F, (SCREEN_HEIGHT - 36.0F) / 2.0F, 66, 36, menuBehaviour);
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
}

static void deinitGame(void) {
	discardShape(heroNode.shape);
	discardShape(courseNode.shape);
	discardShape(courseNode.collisionShape);
	discardNode(speedNode);
	discardNode(heroNode);
	discardNode(courseNode);
	freeImage(shnm12.font0201);
	freeImage(shnm12.font0208);
	freeImage(hero);
	discardScene(&scene);
}

int loop(float elapsed, Image *out, int sleep) {
	Image sceneImage;
	float menuTransitionTemp;
	if(controller.backCamera) {
		scene.camera.position[0] *= -1.0F;
		scene.camera.position[2] *= -1.0F;
	}
	sceneImage = drawScene(&scene);
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
		scene.camera.position[0] *= -1.0F;
		scene.camera.position[2] *= -1.0F;
	}
	if(!isMenuOpened) {
		clearController(&menuController);
		updateController(keyboard);
		updateScene(&scene, elapsed / 2.0F);
		updateScene(&scene, elapsed / 2.0F);
		updateScene(&menuScene, elapsed);
	} else {
		clearController(&keyboard);
		updateController(menuController);
		updateScene(&menuScene, elapsed);
	}
	if(controller.quit || sleep) {
		isMenuOpened = TRUE;
	} else if(quitMenu.state) {
		isMenuOpened = FALSE;
	}
	if(enterKey.state && menuSelect == QUIT) {
		// saveRecord(&heroRecords, "./carRecords/title.crd");
		return FALSE;
	}
	quitPrevious = controller.quit;
	menuTransition += 0.75F * (isMenuOpened - menuTransition);
	return TRUE;
}

// item.
// title
// effect, animation

int WINAPI ctrlCHandler(DWORD dwCtrlType) {
	deinitGame();
	deinitCNSG();
	ExitProcess(0);
	return TRUE;
}

int main(int argc, char *argv[]) {
	initCNSG(argc, argv, SCREEN_WIDTH, SCREEN_HEIGHT);
	SetConsoleCtrlHandler(ctrlCHandler, TRUE);
	initGame();
	startGame();
	gameLoop(FRAME_PER_SECOND, loop);
	deinitGame();
	deinitCNSG();
	return 0;
}
