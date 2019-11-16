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

static struct {
	float move[2];
	float arrow[4];
	float direction[2];
	float action;
	float retry;
	float collision;
	float quit;
	float backCamera;
	float resetCamera;
} controller;

static FontSJIS shnm12;
static FontSJIS shnm16b;

static Controller keyboard;
static ControllerEvent wasd[4], action, restart, collision, quit, arrow[4], backCamera, resetCamera;

static Image hero, opponentImage;
static Image course;
static Image stageImage;

static Scene scene;
static Node speedNode, lapNode, rankNode, centerNode, timeNode;
static Node mapNode;
static Node lapJudgmentNodes[3];
static Node heroNode, heroRayNode, heroLeftRayNode, heroRightRayNode;
static Node opponentNode, opponentRayNode;
static Node heroMarkerNode, opponentMarkerNode;
static Node courseNode, courseMapNode, courseDirtNode;
static Node stageNode;

static Scene resultScene;
static Node resultNode;

static Scene mapScene;

static float nextCameraPosition[3] = { 0.0F, 25.0F, -75.0F };
static float currentCameraPosition[3] = { 0.0F, 25.0F, -75.0F };
static float cameraAngle, cameraFov;
static int heroLapScore, opponentLapScore;
static int heroPreviousLap = -1, opponentPreviousLap = -1;
static int collisionFlag;
static int rank;
static int isfinished;
static float transition;
static LARGE_INTEGER startTime;
static float handle, accel;

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

static int autoDrive(Node *node, float normal[3]) {
	float tempVec3[3][3];
	float tempMat3[1][3][3];
	float tempMat4[1][4][4];
	float front[3];
	float autoHandle;
	Node *ray, *leftRay, *rightRay;
	ray = dataAt(&node->children, 0);
	leftRay = dataAt(&node->children, 1);
	rightRay = dataAt(&node->children, 2);
	if(ray->collisionTargets.length != 0) {
		CollisionInfo *rayInfo = ray->collisionTargets.firstItem->data;
		CollisionInfoNode2Node *info = rayInfo->info.firstItem->data;
		memcpy_s(normal, SIZE_VEC3, info->normal, SIZE_VEC3);
	}
	convMat4toMat3(genRotationMat4(node->angle[0], node->angle[1], node->angle[2], tempMat4[0]), tempMat3[0]);
	initVec3(tempVec3[0], Z_MASK);
	mulMat3Vec3(tempMat3[0], tempVec3[0], front);
	tempVec3[1][0] = front[0];
	tempVec3[1][1] = front[2];
	tempVec3[1][2] = 0.0F;
	clearVec3(tempVec3[0]);
	tempVec3[0][0] = normal[0];
	tempVec3[0][1] = normal[2];
	cross(tempVec3[0], tempVec3[1], tempVec3[2]);
	// node->torque[1] += 300000.0F *  * (1.0F - cosVec3(normal, tempVec3[1])) / 2.0F;
	// subVec3(mulVec3ByScalar(front, 30000.0F, tempVec3[0]), mulVec3ByScalar(node->velocity, node->collisionShape.mass, tempVec3[1]), tempVec3[0]);
	autoHandle = -sign(tempVec3[2][2]);
	if(leftRay->collisionFlags & DIRT_COLLISIONMASK) autoHandle += 2.0F;
	if(rightRay->collisionFlags & DIRT_COLLISIONMASK) autoHandle -= 2.0F;
	controlCar(node, (ray->collisionFlags & COURSE_COLLISIONMASK) ? -0.5F : 1.0F, autoHandle, front);
	return TRUE;
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

static float force[3];
static float heroAngle;
static int heroBehaviour(Node *node) {
	// CollisionInfo *targetInfo;
	if(node->collisionFlags & COURSE_COLLISIONMASK) {
		if(/*TRUE*/ isfinished) {
			autoDrive(node, force);
		} else {
			float front[3];
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
	if(node->collisionFlags & DIRT_COLLISIONMASK) {
		node->collisionShape.dynamicFriction = 1.0F;
	} else {
		node->collisionShape.dynamicFriction = 0.1F;
	}
	updateLapScore(node->collisionFlags, &heroPreviousLap, &heroLapScore);
	// iterf(&node->collisionTargets, &targetInfo) {
	//   if(targetInfo->target->collisionMaskActive & LAP_COLLISIONMASK) {
	//     CollisionInfoNode2Node *info = targetInfo->info.firstItem->data;
	//     float upper[3] = { 0.0F, 25.0F, 0.0F };
	//     mulVec3ByScalar(info->normal, 75.0F, nextCameraPosition);
	//     addVec3(nextCameraPosition, upper, nextCameraPosition);
	//   }
	// }
	memcpy_s(heroMarkerNode.position, SIZE_VEC3, node->position, SIZE_VEC3);
	memcpy_s(heroMarkerNode.angle, SIZE_VEC3, node->angle, SIZE_VEC3);
	return TRUE;
}

static float onormal[3];
static int opponentBehaviour(Node *node) {
	updateLapScore(node->collisionFlags, &opponentPreviousLap, &opponentLapScore);
	autoDrive(node, onormal);
	memcpy_s(opponentMarkerNode.position, SIZE_VEC3, node->position, SIZE_VEC3);
	memcpy_s(opponentMarkerNode.angle, SIZE_VEC3, node->angle, SIZE_VEC3);
	return TRUE;
}

static int timeBehaviour(Node *node) {
	if(!isfinished) {
		char buffer[14];
		float elapsed = elapsedTime(startTime);
		int minutes = elapsed / 60;
		float seconds = elapsed - 60.0F * minutes;
		sprintf(buffer, "TIME %d`%06.3f", minutes, seconds);
		drawTextSJIS(node->texture, shnm12, 0, 0, buffer);
	}
	return TRUE;
}

static int speedBehaviour(Node *node) {
	char buffer[11];
	sprintf(buffer, "%5.1f km/h", length3(heroNode.velocity) * 3600 / 10000);
	drawTextSJIS(speedNode.texture, shnm12, 0, 0, buffer);
	return TRUE;
}

static int lapBehaviour(Node *node) {
	if(!isfinished) {
		char buffer[8];
		int lap = heroLapScore / 45 + 1;
		if(lap > 3) {
			lap = 3;
			isfinished = TRUE;
		}
		sprintf(buffer, "LAP %d/3", lap);
		drawTextSJIS(node->texture, shnm12, 0, 0, buffer);
	}
	return TRUE;
}

static int rankBehaviour(Node *node) {
	static char rankList[][4] = { "1st", "2nd" };
	if(heroLapScore != opponentLapScore) {
		rank = heroLapScore > opponentLapScore ? 0 : 1;
	}
	drawTextSJIS(node->texture, shnm12, 0, 0, rankList[rank]);
	return TRUE;
}

static int centerBehaviour(Node *node) {
	if(isfinished) {
		drawTextSJIS(node->texture, shnm16b, 0, 0, "FINISH!");
		node->isVisible = TRUE;
	} else {
		node->isVisible = FALSE;
	}
	return TRUE;
}

static int resultBehaviour(Node *node) {
	drawTextSJIS(node->texture, shnm12, 0, 0, "RANK\n1st YOU\n2nd OPP");
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

static Node initCarLeftRayNode(const char id[]) {
	Node carRayNode;
	carRayNode = initNode(id, NO_IMAGE);
	carRayNode.shape = initShapePlane(100.0F, 50.0F, BLACK, 1.0F);
	carRayNode.position[0] = -50.0F;
	carRayNode.collisionShape = carRayNode.shape;
	carRayNode.collisionMaskPassive = DIRT_COLLISIONMASK;
	carRayNode.isThrough = TRUE;
	carRayNode.isVisible = FALSE;
	return carRayNode;
}

static Node initCarRightRayNode(const char id[]) {
	Node carRayNode;
	carRayNode = initNode(id, NO_IMAGE);
	carRayNode.shape = initShapePlane(100.0F, 50.0F, BLACK, 1.0F);
	carRayNode.position[0] = 50.0F;
	carRayNode.collisionShape = carRayNode.shape;
	carRayNode.collisionMaskPassive = DIRT_COLLISIONMASK;
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

static void initialize(void) {
	int i;
	char lapNames[3][50] = { "./assets/courseMk2CollisionA.obj", "./assets/courseMk2CollisionB.obj", "./assets/courseMk2CollisionC.obj" };
	initCNSG(SCREEN_WIDTH, SCREEN_HEIGHT);
	shnm12 = initFontSJIS(loadBitmap("assets/shnm6x12r.bmp", NULL_COLOR), loadBitmap("assets/shnmk12.bmp", NULL_COLOR), 6, 12, 12);
	shnm16b = initFontSJIS(loadBitmap("assets/shnm8x16rb.bmp", NULL_COLOR), loadBitmap("assets/shnmk16b.bmp", NULL_COLOR), 8, 16, 16);

	keyboard = initController();
	initControllerEventCross(wasd, 'W', 'A', 'S', 'D', controller.move);
	initControllerEventCross(arrow, VK_UP, VK_LEFT, VK_DOWN, VK_RIGHT, controller.arrow);
	action = initControllerEvent(VK_SPACE, 1.0F, 0.0F, &controller.action);
	restart = initControllerEvent('R', 1.0F, 0.0F, &controller.retry);
	collision = initControllerEvent(VK_F1, 1.0F, 0.0F, &controller.collision);
	backCamera = initControllerEvent(VK_SHIFT, 1.0F, 0.0F, &controller.backCamera);
	resetCamera = initControllerEvent(VK_SPACE, 1.0F, 0.0F, &controller.resetCamera);

	quit = initControllerEvent(VK_ESCAPE, 1.0F, 0.0F, &controller.quit);
	pushUntilNull(&keyboard.events, &wasd[0], &wasd[1], &wasd[2], &wasd[3], NULL);
	pushUntilNull(&keyboard.events, &arrow[0], &arrow[1], &arrow[2], &arrow[3], NULL);
	pushUntilNull(&keyboard.events, &action, &restart, &quit, &collision, &backCamera, &resetCamera, NULL);

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
	heroLeftRayNode = initCarLeftRayNode("heroRay");
	heroRightRayNode = initCarRightRayNode("heroRay");
	addNodeChild(&heroNode, &heroRayNode);
	addNodeChild(&heroNode, &heroLeftRayNode);
	addNodeChild(&heroNode, &heroRightRayNode);

	opponentNode = initNode("opponent", opponentImage);
	initShapeFromObj(&opponentNode.shape, "./assets/subaru.obj", 1.0F);
	initShapeFromObj(&opponentNode.collisionShape, "./assets/subaruCollision.obj", 500.0F);
	opponentNode.isPhysicsEnabled = TRUE;
	setVec3(opponentNode.scale, 4.0F, XYZ_MASK);
	opponentNode.collisionMaskActive = CAR_COLLISIONMASK;
	opponentNode.collisionMaskPassive = CAR_COLLISIONMASK | COURSE_COLLISIONMASK | LAPA_COLLISIONMASK | LAPB_COLLISIONMASK | LAPC_COLLISIONMASK;
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
	centerNode = initNodeText("center", 32.0F, 56.0F, 56, 16, centerBehaviour);
	centerNode.isVisible = FALSE;

	mapNode = initNodeText("map", 0.0F, 24.0F, 128.0F, 128.0F, mapBehaviour);
	divVec3ByScalar(mapNode.scale, 4.0F, mapNode.scale);

	resultScene = initScene();
	resultNode = initNodeText("result", 0.0F, 0.0F, 84, 36, resultBehaviour);
	push(&resultScene.nodes, &resultNode);

	mapScene = initScene();
	mapScene.camera = initCamera(0.0F, 1000.0F, 0.0F, SCREEN_ASPECT);
	initVec3(mapScene.camera.worldUp, X_MASK);
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
}

static void startGame(void) {
	cameraAngle = 0.0F;
	cameraFov = PI / 3.0F * 1.5F;
	isfinished = FALSE;
	heroLapScore = 0;
	heroPreviousLap = -1;
	opponentLapScore = 0;
	opponentPreviousLap = -1;

	heroNode.position[0] = 650.0F;
	heroNode.position[1] = 75.0F;
	heroNode.position[2] = 0.0F;
	clearVec3(heroNode.velocity);
	clearVec3(heroNode.angle);
	clearVec3(heroNode.angVelocity);
	clearVec3(heroNode.angMomentum);

	opponentNode.position[0] = 600.0F;
	opponentNode.position[1] = 50.0F;
	opponentNode.position[2] = 0.0F;
	clearVec3(opponentNode.velocity);
	clearVec3(opponentNode.angle);
	clearVec3(opponentNode.angVelocity);
	clearVec3(opponentNode.angMomentum);

	clearVector(&scene.nodes);
	pushUntilNull(&scene.nodes, &speedNode, &lapNode, &rankNode, &centerNode, &timeNode, NULL);
	pushUntilNull(&scene.nodes, &mapNode, NULL);
	pushUntilNull(&scene.nodes, &lapJudgmentNodes[0], &lapJudgmentNodes[1], &lapJudgmentNodes[2], NULL);
	pushUntilNull(&scene.nodes, &heroNode/*, &opponentNode*/, NULL);
	pushUntilNull(&scene.nodes, &courseNode, &courseDirtNode, NULL);
	push(&scene.nodes, &stageNode);

	QueryPerformanceCounter(&startTime);
}

static void deinitialize(void) {
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
	deinitGraphics();
}

int loop(float elapsed, Image *out) {
	float tempVec3[2][3];
	float tempMat4[1][4][4];
	float tempMat3[1][3][3];
	updateController(keyboard);
	if(controller.backCamera) {
		scene.camera.position[0] *= -1.0F;
		scene.camera.position[2] *= -1.0F;
	}
	*out = drawScene(&scene);//linearTransition(drawScene(&scene), drawScene(&resultScene), transition);
	if(controller.backCamera) {
		scene.camera.position[0] *= -1.0F;
		scene.camera.position[2] *= -1.0F;
	}
	updateScene(&scene, elapsed / 2.0F);
	updateScene(&scene, elapsed / 2.0F);
	updateScene(&resultScene, elapsed);
	if(controller.quit) return FALSE;
	if(controller.retry) startGame();
	if(controller.collision) {
		if(!collisionFlag) {
			int i;
			// for(i = 0;i < 3;i++) lapJudgmentNodes[i].isVisible = !lapJudgmentNodes[i].isVisible;
			heroRayNode.isVisible = !heroRayNode.isVisible;
			heroLeftRayNode.isVisible = !heroLeftRayNode.isVisible;
			heroRightRayNode.isVisible = !heroRightRayNode.isVisible;
			opponentRayNode.isVisible = !opponentRayNode.isVisible;
			courseDirtNode.isVisible = !courseDirtNode.isVisible;
			collisionFlag = TRUE;
		}
	} else {
		collisionFlag = FALSE;
	}
	accel += 0.3F * (controller.move[1] - accel);
	cameraFov = min(PI / 3.0F * 1.5F, max(PI / 3.0F, cameraFov - 0.05F * controller.arrow[1]));
	scene.camera.fov = cameraFov + accel / 5.0F;
	addVec3(currentCameraPosition, mulVec3ByScalar(subVec3(nextCameraPosition, currentCameraPosition, tempVec3[0]), 2.0F * elapsed, tempVec3[1]), currentCameraPosition);
	cameraAngle -= 0.1F * controller.arrow[0];
	if(controller.resetCamera) cameraAngle -= 0.3F * cameraAngle;
	handle += 0.3F * (controller.move[0] - handle);
	genRotationMat4(0.0F, cameraAngle, 0.0F, tempMat4[0]);
	mulMat3Vec3(convMat4toMat3(tempMat4[0], tempMat3[0]), currentCameraPosition, scene.camera.position);
	return TRUE;
}

int main(void) {
	initialize();
	startGame();
	gameLoop(FRAME_PER_SECOND, loop);
	deinitialize();
	return 0;
}
