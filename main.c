#include<stdio.h>
#include<math.h>
#include<time.h>
#include<Windows.h>

#include "./cnsglib/include/cnsg.h"

#define SCREEN_SIZE 128
#define FRAME_PER_SECOND 60

#define CAR_COLLISIONMASK 0x01
#define COURSE_COLLISIONMASK 0x02
#define LAPA_COLLISIONMASK 0x04
#define LAPB_COLLISIONMASK 0x08
#define LAPC_COLLISIONMASK 0x10

static struct {
	float move[2];
	float arrow[4];
	float direction[2];
	float action;
	float retry;
	float quit;
} controller;

static FontSJIS shnm12;

static Controller keyboard;
static ControllerEvent wasd[4], action, restart, quit, arrow[4];

static Image hero;
static Image course;
static Image stageImage;

static Node speedNode;
static Node lapNode;
static Node lapJudgmentNodes[3];
static Scene scene;
static Node heroNode;
static Node opponentNode;
static Node courseNode;
static Node stageNode;

static float cameraAngle;
static int lapScore;
static int previousLap = -1;

static int heroBehaviour(Node *node) {
	float tempVec3[3][3];
	float tempMat3[1][3][3];
	float tempMat4[1][4][4];
	if(node->collisionFlags & COURSE_COLLISIONMASK) {
		float velocityLengthH;
		convMat4toMat3(genRotationMat4(node->angle[0], node->angle[1], node->angle[2], tempMat4[0]), tempMat3[0]);
		initVec3(tempVec3[0], Z_MASK);
		mulMat3Vec3(tempMat3[0], tempVec3[0], tempVec3[1]);
		normalize3(extractComponents3(tempVec3[1], XZ_MASK, tempVec3[0]), tempVec3[1]);
		extractComponents3(node->velocity, XZ_MASK, tempVec3[0]);
		velocityLengthH = length3(tempVec3[0]);
		mulVec3ByScalar(tempVec3[1], velocityLengthH * node->shape.mass + controller.move[1] * 10000.0F, tempVec3[0]);
		subVec3(tempVec3[0], mulVec3ByScalar(node->velocity, node->shape.mass, tempVec3[2]), tempVec3[1]);
		applyForce(node, tempVec3[1], XZ_MASK, FALSE);
		if(velocityLengthH > 1.0F) node->torque[1] += controller.move[0] * 50000.0F;
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
	sprintf(buffer, "LAP %d/3", lapScore / 15 + 1);
	drawTextSJIS(node->texture, shnm12, 0, 0, buffer);
	return TRUE;
}

static void initialize(void) {
	int i;
	char lapNames[3][50] = { "./assets/courseCollisionA.obj", "./assets/courseCollisionB.obj", "./assets/courseCollisionC.obj" };
	initCNSG(SCREEN_SIZE, SCREEN_SIZE);
	shnm12 = initFontSJIS(loadBitmap("assets/shnm6x12r.bmp", NULL_COLOR), loadBitmap("assets/shnmk12.bmp", NULL_COLOR), 6, 12, 12);

	keyboard = initController();
	initControllerEventCross(wasd, 'W', 'A', 'S', 'D', controller.move);
	initControllerEventCross(arrow, VK_UP, VK_LEFT, VK_DOWN, VK_RIGHT, controller.arrow);
	action = initControllerEvent(VK_SPACE, 1.0F, 0.0F, &controller.action);
	restart = initControllerEvent('R', 1.0F, 0.0F, &controller.retry);
	quit = initControllerEvent(VK_ESCAPE, 1.0F, 0.0F, &controller.quit);
	pushUntilNull(&keyboard.events, &wasd[0], &wasd[1], &wasd[2], &wasd[3], NULL);
	pushUntilNull(&keyboard.events, &arrow[0], &arrow[1], &arrow[2], &arrow[3], NULL);
	pushUntilNull(&keyboard.events, &action, &restart, &quit, NULL);

	hero = loadBitmap("assets/car.bmp", NULL_COLOR);
	course = loadBitmap("assets/course.bmp", NULL_COLOR);
	scene = initScene();
	scene.camera = initCamera(0.0F, 50.0F, -50.0F, 1.0F);
	scene.camera.parent = &heroNode;
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

	opponentNode = initNode("opponent", hero);
	initShapeFromObj(&opponentNode.shape, "./assets/car.obj", 100.0F);
	initShapeFromObj(&opponentNode.collisionShape, "./assets/carCollision.obj", 100.0F);
	opponentNode.isPhysicsEnabled = TRUE;
	setVec3(opponentNode.scale, 16.0F, XYZ_MASK);
	opponentNode.collisionMaskActive = CAR_COLLISIONMASK;
	opponentNode.collisionMaskPassive = CAR_COLLISIONMASK | COURSE_COLLISIONMASK;

	for(i = 0;i < 3;i++) {
		lapJudgmentNodes[i] = initNode(lapNames[i], NO_IMAGE);
		initShapeFromObj(&lapJudgmentNodes[i].shape, lapNames[i], 100.0F);
		lapJudgmentNodes[i].collisionShape = lapJudgmentNodes[i].shape;
		setVec3(lapJudgmentNodes[i].scale, 4.0F, XYZ_MASK);
		lapJudgmentNodes[i].collisionMaskActive = LAPA_COLLISIONMASK << i;
		lapJudgmentNodes[i].isVisible = FALSE;
		lapJudgmentNodes[i].isThrough = TRUE;
	}

	speedNode = initNodeText("speed", -60.0F, 0.0F, 60, 12);
	speedNode.behaviour = speedBehaviour;

	lapNode = initNodeText("lap", 0.0F, 0.0F, 42, 12);
	lapNode.behaviour = lapBehaviour;
}

static void startGame(void) {
	heroNode.position[0] = 650.0F;
	heroNode.position[1] = 50.0F;
	heroNode.position[2] = 0.0F;
	clearVec3(heroNode.velocity);

	opponentNode.position[0] = 600.0F;
	opponentNode.position[1] = 50.0F;
	opponentNode.position[2] = 0.0F;

	clearVector(&scene.nodes);
	pushUntilNull(&scene.nodes, &speedNode, &lapNode, NULL);
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

float elapsedTime(LARGE_INTEGER start, LARGE_INTEGER frequency) {
	LARGE_INTEGER current;
	LARGE_INTEGER elapsed;
	QueryPerformanceCounter(&current);
	elapsed.QuadPart = current.QuadPart - start.QuadPart;
	return (float)elapsed.QuadPart / frequency.QuadPart;
}

int main(void) {
	LARGE_INTEGER previousClock;
	LARGE_INTEGER frequency;
	QueryPerformanceFrequency(&frequency);
	initialize();
	startGame();
	QueryPerformanceCounter(&previousClock);
	while(TRUE) {
		LARGE_INTEGER previousSceneClock;
		updateController(keyboard);
		drawScene(&scene);
		updateScene(&scene, elapsedTime(previousClock, frequency));
		QueryPerformanceCounter(&previousSceneClock);
		while(elapsedTime(previousClock, frequency) < 1.0F / FRAME_PER_SECOND) {
			float elapsed = elapsedTime(previousSceneClock, frequency);
			QueryPerformanceCounter(&previousSceneClock);
		};
		QueryPerformanceCounter(&previousClock);
		if(controller.quit) break;
		if(controller.retry) startGame();
		cameraAngle += 0.05F * controller.arrow[0];
		scene.camera.position[0] = 50.0F * sin(cameraAngle);
		scene.camera.position[2] = -50.0F * cos(cameraAngle);
		scene.camera.fov -= 0.05F * controller.arrow[1];
		scene.camera.fov = min(PI / 3.0F * 2.0F, max(PI / 3.0F, scene.camera.fov));
	}
	deinitialize();
	return 0;
}
