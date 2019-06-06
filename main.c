#include<stdio.h>
#include<math.h>
#include<stdint.h>
#include<windows.h>
#include<time.h>

#include "./include/matrix.h"
#include "./include/graphics.h"
#include "./include/colors.h"
#include "./include/sprite.h"
#include "./include/vector.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define NOF_MAX_EVENTS 10

HWND window;
HANDLE input;
HANDLE doubleScreens[2];
HANDLE visibleScreen, hiddenScreen;
char *buffer;
size_t bufferLength;
COORD bufferSizeCoord = { 250, 250 };
INPUT_RECORD inputRecords[NOF_MAX_EVENTS];
struct {
	float move[2];
	float direction[2];
} controller;

Image hero;
Image background;
Image childImage;

Sprite heroSprite;
Sprite child;

RECT storedSize;
void pushWindowSize() {
	WINDOWINFO info;
	info.cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(window, &info);
	storedSize = info.rcWindow;
}
void popWindowSize() {
	MoveWindow(window, storedSize.left, storedSize.top, storedSize.right - storedSize.left, storedSize.bottom - storedSize.top, FALSE);
}

HANDLE createScreen() {
	static CONSOLE_CURSOR_INFO info = { 1, FALSE };
	COORD actualBufferSize;
	CONSOLE_FONT_INFOEX font = { .cbSize = sizeof(CONSOLE_FONT_INFOEX) };
	HANDLE screen = CreateConsoleScreenBuffer(GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	pushWindowSize();
	SetConsoleActiveScreenBuffer(screen);
	SetConsoleCursorInfo(screen, &info);
	GetCurrentConsoleFontEx(screen, FALSE, &font);
	font.dwFontSize.X = 1;
	font.dwFontSize.Y = 1;
	SetCurrentConsoleFontEx(screen, FALSE, &font);
	actualBufferSize.X = 2 * bufferSizeCoord.X;
	actualBufferSize.Y = bufferSizeCoord.Y;
	SetConsoleScreenBufferSize(screen, actualBufferSize);
	popWindowSize();
	return screen;
}

void swapScreens() {
	static int visible = 1;
	hiddenScreen = doubleScreens[visible];
	visibleScreen = doubleScreens[(visible ^= 1)];
}

void initialize() {
	window = GetConsoleWindow();
	input = GetStdHandle(STD_INPUT_HANDLE);
	doubleScreens[0] = createScreen();
	doubleScreens[1] = createScreen();
	swapScreens();
	bufferLength = bufferSizeCoord.X * bufferSizeCoord.Y;
	buffer = (char*)malloc(bufferLength);
	hero = loadBitmap("assets/hero.bmp", BLACK);
	background = loadBitmap("assets/Gochi.bmp", NULL_COLOR);
	childImage = genCircle(32, RED);
	heroSprite = initSprite("Hero", hero);
	child = initSprite("child", childImage);
	heroSprite.shadowScale = 0.75;
	heroSprite.shadowOffset[1] = 10.0;
	child.position[0] = 10.0;
	child.angle = 3.14 / 3;
	addChild(&heroSprite, &child);
}

BOOL pollEvents() {
	DWORD nofEvents;
	GetNumberOfConsoleInputEvents(input, &nofEvents);
	if(nofEvents == 0) return TRUE;
	ReadConsoleInput(input, inputRecords, NOF_MAX_EVENTS, &nofEvents);
	for(int i = 0;i < nofEvents;i += 1) {
		switch(inputRecords[i].EventType) {
			case KEY_EVENT:
			KEY_EVENT_RECORD *keyEvent = &inputRecords[i].Event.KeyEvent;
			if(keyEvent->bKeyDown) {
				switch(keyEvent->wVirtualKeyCode) {
					case 'Q': return FALSE;
					case 'W':
						controller.move[1] -= 1.0;
						break;
					case 'S':
						controller.move[1] = 1.0;
						break;
					case 'A':
						controller.move[0] = -1.0;
						break;
					case 'D':
						controller.move[0] = 1.0;
						break;
					case VK_UP:
						controller.direction[1] = -1.0;
						break;
					case VK_DOWN:
						controller.direction[1] = 1.0;
						break;
					case VK_LEFT:
						controller.direction[0] = -1.0;
						break;
					case VK_RIGHT:
						controller.direction[0] = 1.0;
						break;
				}
			} else {
				switch(keyEvent->wVirtualKeyCode) {
					case 'W':
					case 'S':
						controller.move[1] = 0.0;
						break;
					case 'A':
					case 'D':
						controller.move[0] = 0.0;
						break;
					case VK_UP:
					case VK_DOWN:
						controller.direction[1] = 0.0;
						break;
					case VK_LEFT:
					case VK_RIGHT:
						controller.direction[0] = 0.0;
						break;
				}
			}
			break;
			default: break;
		}
	}
	return TRUE;
}

void flush() {
	DWORD nofWritten;
	static COORD cursor = { 0, 0 };
	WORD *data = (WORD*)malloc(2 * bufferLength * sizeof(WORD));
	size_t index;
	for(index = 0;index < bufferLength;index++) {
		WORD attribute = BACKGROUND_BLUE - 1 + (((buffer[index] & 8) | ((buffer[index] & 1) << 2) | (buffer[index] & 2) | ((buffer[index] & 4) >> 2)) << 4);
		data[2 * index] = attribute;
		data[2 * index + 1] = attribute;
	}
	WriteConsoleOutputAttribute(hiddenScreen, data, bufferLength, cursor, &nofWritten);
	free(data);
	swapScreens();
}

void loop() {
	float move[2];
	addVec2(heroSprite.position, mulVec2ByScalar(controller.move, 0.5, move), heroSprite.position);
	heroSprite.angle = angleVec2(controller.direction) + PI / 2.0;
}

void draw() {
	memset(buffer, 0, bufferLength);
	clearTransformation();
	drawSprite(heroSprite);
	flush();
	// Scene System.
}

void deinitialize() {
	freeImage(hero);
	freeImage(background);
	freeImage(childImage);
	free(buffer);
}

int main() {
	initialize();
	while(TRUE) {
		if(!pollEvents()) break;
		loop();
		draw();
	}
	deinitialize();
	return 0;
}
