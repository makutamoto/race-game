#include<stdio.h>
#include<math.h>
#include<time.h>
#include<Windows.h>

#include "./cnsglib/include/cnsg.h"

#include "./include/common.h"
#include "./include/race.h"
#include "./include/title.h"
#include "./include/menu.h"

FontSJIS shnm12, shnm16b;

Image carImage;
Node stageNode, courseNode;
Shape carShape, carCollisionShape;
WindowManager *rootManager;

static void initGame(void) {
	shnm12 = initFontSJIS(loadBitmap("assets/shnm6x12r.bmp", NULL_COLOR), loadBitmap("assets/shnmk12.bmp", NULL_COLOR), 6, 12, 12);
	shnm16b = initFontSJIS(loadBitmap("assets/shnm8x16rb.bmp", NULL_COLOR), loadBitmap("assets/shnmk16b.bmp", NULL_COLOR), 8, 16, 16);
	initRace();
	initTitle();
	initMenu();
}

int main(int argc, char *argv[]) {
	rootManager = initCNSG(argc, argv, SCREEN_WIDTH, SCREEN_HEIGHT);
	initGame();
	startTitle();
	gameLoop(FRAME_PER_SECOND);
	return 0;
}
