#include<stdio.h>
#include<math.h>
#include<time.h>
#include<Windows.h>

#include "./cnsglib/include/cnsg.h"

#define SCREEN_SIZE 128
#define FRAME_PER_SECOND 60

#define CAR_COLLISIONMASK 0x01
#define COURSE_COLLISIONMASK 0x02
#define LAP_COLLISIONMASK (LAPA_COLLISIONMASK | LAPB_COLLISIONMASK | LAPC_COLLISIONMASK)
#define LAPA_COLLISIONMASK 0x04
#define LAPB_COLLISIONMASK 0x08
#define LAPC_COLLISIONMASK 0x10

static struct {
	float move[2];
	float arrow[4];
	float direction[2];
	float action;
	float retry;
	float collision;
	float quit;
	float backCamera;
} controller;

static FontSJIS shnm12;
static FontSJIS shnm16b;

static Controller keyboard;
static ControllerEvent wasd[4], action, restart, collision, quit, arrow[4], backCamera;

static Image hero;
static Image course;
static Image stageImage;

static Scene scene;
static Node speedNode, lapNode, rankNode, centerNode;
static Node lapJudgmentNodes[3];
static Node heroNode, heroRayNode;
static Node opponentNode, opponentRayNode;
static Node courseNode;
static Node stageNode;

static Scene resultScene;
static Node resultNode;

static float nextCameraPosition[3] = { 0.0F, 50.0F, -50.0F };
static float currentCameraPosition[3] = { 0.0F, 50.0F, -50.0F };
static float cameraAngle;
static int lapScore, opponentLapScore;
static int previousLap = -1, opponentPreviousLap = -1;
static int collisionFlag;
static int rank;
static int isfinished;
static float transition;

static int autoDrive(Node *node) {
	float tempVec3[2][3];
	Node *ray = node->children.firstItem->data;
	if(ray->collisionTargets.length != 0) {
		CollisionInfo *rayInfo = ray->collisionTargets.firstItem->data;
		subVec3(mulVec3ByScalar(rayInfo->normals.firstItem->data, -10000.0F, tempVec3[0]), mulVec3ByScalar(node->velocity, node->shape.mass, tempVec3[1]), tempVec3[0]);
		applyForce(node, tempVec3[0], XZ_MASK, FALSE);
	}
	return TRUE;
}

static int heroBehaviour(Node *node) {
	float tempVec3[3][3];
	float tempMat3[1][3][3];
	float tempMat4[1][4][4];
	CollisionInfo *targetInfo;
	if(node->collisionFlags & COURSE_COLLISIONMASK) {
		if(isfinished) {
			autoDrive(node);
		} else {
			float velocityLengthH;
			convMat4toMat3(genRotationMat4(node->angle[0], node->angle[1], node->angle[2], tempMat4[0]), tempMat3[0]);
			initVec3(tempVec3[0], Z_MASK);
			mulMat3Vec3(tempMat3[0], tempVec3[0], tempVec3[1]);
			normalize3(extractComponents3(tempVec3[1], XZ_MASK, tempVec3[0]), tempVec3[1]);
			extractComponents3(node->velocity, XZ_MASK, tempVec3[0]);
			velocityLengthH = length3(tempVec3[0]);
			mulVec3ByScalar(tempVec3[1], velocityLengthH * node->shape.mass + controller.move[1] * 30000.0F, tempVec3[0]);
			subVec3(tempVec3[0], mulVec3ByScalar(node->velocity, node->shape.mass, tempVec3[2]), tempVec3[1]);
			applyForce(node, tempVec3[1], XZ_MASK, FALSE);
			if(velocityLengthH > 1.0F) node->torque[1] += controller.move[0] * 30000.0F;
		}
	}
	if(node->collisionFlags & LAPA_COLLISIONMASK) previousLap = 0;
	if(node->collisionFlags & LAPB_COLLISIONMASK) previousLap = 1;
	if(node->collisionFlags & LAPC_COLLISIONMASK) {
		if(previousLap == 1) {
			lapScore += 1;
		} else if(previousLap == 0) {
			lapScore -= 1;
		}
		previousLap = -1;
	}
	iterf(&node->collisionTargets, &targetInfo) {
		if(targetInfo->target->collisionMaskActive & LAP_COLLISIONMASK) {
			float upper[3] = { 0.0F, 25.0F, 0.0F };
			mulVec3ByScalar(targetInfo->normals.firstItem->data, 50.0F, nextCameraPosition);
			addVec3(nextCameraPosition, upper, nextCameraPosition);
		}
	}
	return TRUE;
}

static int opponentBehaviour(Node *node) {
	if(node->collisionFlags & LAPA_COLLISIONMASK) opponentPreviousLap = 0;
	if(node->collisionFlags & LAPB_COLLISIONMASK) opponentPreviousLap = 1;
	if(node->collisionFlags & LAPC_COLLISIONMASK) {
		if(opponentPreviousLap == 1) {
			opponentLapScore += 1;
		} else if(opponentPreviousLap == 0) {
			opponentLapScore -= 1;
		}
		opponentPreviousLap = -1;
	}
	autoDrive(node);
	return TRUE;
}

static int speedBehaviour(Node *node) {
	char buffer[11];
	sprintf(buffer, "%5.1f km/h", length3(heroNode.velocity) * 3600 / 10000);
	drawTextSJIS(speedNode.texture, shnm12, 0, 0, buffer);
	return TRUE;
}

static int lapBehaviour(Node *node) {
	char buffer[8];
	int lap = lapScore / 15 + 1;
	if(lap > 3) {
		lap = 3;
		isfinished = TRUE;
		transition += transition * transition + 0.01F;
	}
	sprintf(buffer, "LAP %d/3", lap);
	drawTextSJIS(node->texture, shnm12, 0, 0, buffer);
	return TRUE;
}

static int rankBehaviour(Node *node) {
	static char rankList[][4] = { "1st", "2nd" };
	if(lapScore != opponentLapScore) {
		rank = lapScore > opponentLapScore ? 0 : 1;
	}
	drawTextSJIS(node->texture, shnm12, 0, 0, rankList[rank]);
	return TRUE;
}

static int centerBehaviour(Node *node) {
	if(isfinished) {
		drawTextSJIS(node->texture, shnm16b, 0, 0, "FINISH!");
		node->isVisible = TRUE;
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
	carRayNode.shape = initShapePlane(30.0F, 150.0F, BLACK, 1.0F);
	carRayNode.position[2] = 75.0F;
	carRayNode.collisionShape = carRayNode.shape;
	carRayNode.collisionMaskPassive = LAPA_COLLISIONMASK | LAPB_COLLISIONMASK | LAPC_COLLISIONMASK;
	carRayNode.isThrough = TRUE;
	carRayNode.isVisible = FALSE;
	return carRayNode;
}

static void initialize(void) {
	int i;
	char lapNames[3][50] = { "./assets/courseCollisionA.obj", "./assets/courseCollisionB.obj", "./assets/courseCollisionC.obj" };
	initCNSG(SCREEN_SIZE, SCREEN_SIZE);
	shnm12 = initFontSJIS(loadBitmap("assets/shnm6x12r.bmp", NULL_COLOR), loadBitmap("assets/shnmk12.bmp", NULL_COLOR), 6, 12, 12);
	shnm16b = initFontSJIS(loadBitmap("assets/shnm8x16rb.bmp", NULL_COLOR), loadBitmap("assets/shnmk16b.bmp", NULL_COLOR), 8, 16, 16);

	keyboard = initController();
	initControllerEventCross(wasd, 'W', 'A', 'S', 'D', controller.move);
	initControllerEventCross(arrow, VK_UP, VK_LEFT, VK_DOWN, VK_RIGHT, controller.arrow);
	action = initControllerEvent(VK_SPACE, 1.0F, 0.0F, &controller.action);
	restart = initControllerEvent('R', 1.0F, 0.0F, &controller.retry);
	collision = initControllerEvent(VK_F1, 1.0F, 0.0F, &controller.collision);
	backCamera = initControllerEvent(VK_SHIFT, 1.0F, 0.0F, &controller.backCamera);

	quit = initControllerEvent(VK_ESCAPE, 1.0F, 0.0F, &controller.quit);
	pushUntilNull(&keyboard.events, &wasd[0], &wasd[1], &wasd[2], &wasd[3], NULL);
	pushUntilNull(&keyboard.events, &arrow[0], &arrow[1], &arrow[2], &arrow[3], NULL);
	pushUntilNull(&keyboard.events, &action, &restart, &quit, &collision, &backCamera, NULL);

	hero = loadBitmap("assets/car.bmp", NULL_COLOR);
	course = loadBitmap("assets/course.bmp", NULL_COLOR);
	scene = initScene();
	scene.camera = initCamera(0.0F, 50.0F, -50.0F, 1.0F);
	scene.camera.parent = &heroNode;
	scene.camera.isRotationDisabled = TRUE;
	scene.camera.positionMask[1] = TRUE;
	scene.background = BLUE;

	courseNode = initNode("course", course);
	initShapeFromObj(&courseNode.shape, "./assets/course.obj", 1.0F);
	initShapeFromObj(&courseNode.collisionShape, "./assets/courseCollision.obj", 1.0F);
	setVec3(courseNode.scale, 4.0F, XYZ_MASK);
	courseNode.collisionMaskActive = COURSE_COLLISIONMASK;

	stageImage = loadBitmap("assets/stage.bmp", NULL_COLOR);
	stageNode = initNode("stage", stageImage);
	initShapeFromObj(&stageNode.shape, "./assets/stage.obj", 1.0F);
	stageNode.collisionShape = stageNode.shape;
	setVec3(stageNode.scale, 4.0F, XYZ_MASK);

	heroNode = initNode("Hero", hero);
	initShapeFromObj(&heroNode.shape, "./assets/car.obj", 100.0F);
	initShapeFromObj(&heroNode.collisionShape, "./assets/carCollision.obj", 100.0F);
	heroNode.isPhysicsEnabled = TRUE;
	setVec3(heroNode.scale, 16.0F, XYZ_MASK);
	heroNode.collisionMaskActive = CAR_COLLISIONMASK;
	heroNode.collisionMaskPassive = CAR_COLLISIONMASK | COURSE_COLLISIONMASK | LAPA_COLLISIONMASK | LAPB_COLLISIONMASK | LAPC_COLLISIONMASK;
	heroNode.behaviour = heroBehaviour;
	heroRayNode = initCarRayNode("heroRay");
	push(&heroNode.children, &heroRayNode);

	opponentNode = initNode("opponent", hero);
	initShapeFromObj(&opponentNode.shape, "./assets/car.obj", 100.0F);
	initShapeFromObj(&opponentNode.collisionShape, "./assets/carCollision.obj", 100.0F);
	opponentNode.isPhysicsEnabled = TRUE;
	setVec3(opponentNode.scale, 16.0F, XYZ_MASK);
	opponentNode.collisionMaskActive = CAR_COLLISIONMASK;
	opponentNode.collisionMaskPassive = CAR_COLLISIONMASK | COURSE_COLLISIONMASK | LAPA_COLLISIONMASK | LAPB_COLLISIONMASK | LAPC_COLLISIONMASK;
	opponentNode.behaviour = opponentBehaviour;
	opponentRayNode = initCarRayNode("opponentRay");
	push(&opponentNode.children, &opponentRayNode);

	for(i = 0;i < 3;i++) {
		lapJudgmentNodes[i] = initNode(lapNames[i], NO_IMAGE);
		initShapeFromObj(&lapJudgmentNodes[i].shape, lapNames[i], 100.0F);
		lapJudgmentNodes[i].collisionShape = lapJudgmentNodes[i].shape;
		setVec3(lapJudgmentNodes[i].scale, 4.0F, XYZ_MASK);
		lapJudgmentNodes[i].collisionMaskActive = LAPA_COLLISIONMASK << i;
		lapJudgmentNodes[i].isVisible = FALSE;
		lapJudgmentNodes[i].isThrough = TRUE;
	}

	speedNode = initNodeText("speed", -60.0F, 0.0F, 60, 12, speedBehaviour);
	lapNode = initNodeText("lap", 0.0F, 0.0F, 42, 12, lapBehaviour);
	rankNode = initNodeText("rank", 0.0F, 12.0F, 36, 12, rankBehaviour);
	centerNode = initNodeText("center", 32.0F, 56.0F, 56, 16, centerBehaviour);
	centerNode.isVisible = FALSE;

	resultScene = initScene();
	resultNode = initNodeText("result", 0.0F, 0.0F, 84, 36, resultBehaviour);
	push(&resultScene.nodes, &resultNode);
}

static void startGame(void) {
	heroNode.position[0] = 650.0F;
	heroNode.position[1] = 50.0F;
	heroNode.position[2] = 0.0F;
	clearVec3(heroNode.velocity);
	clearVec3(heroNode.angle);
	clearVec3(heroNode.angVelocity);

	opponentNode.position[0] = 600.0F;
	opponentNode.position[1] = 50.0F;
	opponentNode.position[2] = 0.0F;

	clearVector(&scene.nodes);
	pushUntilNull(&scene.nodes, &speedNode, &lapNode, &rankNode, &centerNode, NULL);
	pushUntilNull(&scene.nodes, &lapJudgmentNodes[0], &lapJudgmentNodes[1], &lapJudgmentNodes[2], NULL);
	pushUntilNull(&scene.nodes, &heroNode, &opponentNode, NULL);
	push(&scene.nodes, &courseNode);
	push(&scene.nodes, &stageNode);
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
	*out = linearTransition(drawScene(&scene), drawScene(&resultScene), transition);
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
			for(i = 0;i < 3;i++) lapJudgmentNodes[i].isVisible = !lapJudgmentNodes[i].isVisible;
			opponentRayNode.isVisible = !opponentRayNode.isVisible;
			collisionFlag = TRUE;
		}
	} else {
		collisionFlag = FALSE;
	}
	scene.camera.fov -= 0.05F * controller.arrow[1];
	scene.camera.fov = min(PI / 3.0F * 2.0F, max(PI / 3.0F, scene.camera.fov));
	addVec3(currentCameraPosition, mulVec3ByScalar(subVec3(nextCameraPosition, currentCameraPosition, tempVec3[0]), 2.0F * elapsed, tempVec3[1]), currentCameraPosition);
	cameraAngle -= 0.1F * controller.arrow[0];
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
