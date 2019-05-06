#include<stdio.h>
#include<math.h>
#include<stdint.h>
#include<windows.h>
#include<time.h>

#include "./include/matrix.h"
#include "./include/graphics.h"
#include "./include/colors.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define NOF_MAX_EVENTS 10

typedef struct {
	char id[10];
	float velocity[2];
	float angVelocity;
	float position[2];
	float angle;
	float scale[2];
	float shadowScale;
	float shadowOffset[2];
	Image *image;
	Sprite *children[10];
}	Sprite;

HWND window;
HANDLE store;
HANDLE input;
HANDLE screen;
char *buffer;
size_t bufferSize;
COORD bufferSizeCoord = { 300, 300 };
INPUT_RECORD inputRecords[NOF_MAX_EVENTS];

Image hero;
Image background;

Sprite heroSprite;

void initSprite(Sprite *sprite, const char *id, Image *image) {
	memcpy_s(sprite->id, sizeof(sprite->id), id, min( sizeof(sprite->id), strlen(id)));
	sprite->velocity[0] = 0.0;
	sprite->velocity[1] = 0.0;
	sprite->angVelocity = 0.0;
	sprite->position[0] = 0.0;
	sprite->position[1] = 0.0;
	sprite->angle = 0.0;
	sprite->scale[0] = 1.0;
	sprite->scale[1] = 1.0;
	sprite->shadowScale = 0.0;
	sprite->shadowOffset[0] = 0.0;
	sprite->shadowOffset[1] = 0.0;
	sprite->image = image;
	int i;
	for(i = 0;i < sizeof(sprite->children) / sizeof(*Sprite);i++) sprite->children[i] = NULL;
}

void drawSprite(Sprite sprite) {
	pushTransformation();
	scaleTransformation(sprite.scale[0], sprite.scale[1]);
	rotateTransformation(sprite.angle);
	for(i = 0;i < sizeof(sprite->children) / sizeof(*Sprite);i++)
		if(sprite.children[i] != NULL) drawSprite(sprite.children[i]); 
	}
	pushTransformation();
	scaleTransformation(sprite.shadowScale, sprite.shadowScale);
	if(sprite.shadowOffset[0] != 0.0 || sprite.shadowOffset[1] != 0.0) fillBuffer(sprite.position[0] + sprite.shadowOffset[0], sprite.position[1] + sprite.shadowOffset[1], sprite.image->width, sprite.image->height, sprite.image->data, sprite.image->transparent, TRUE);
	popTransformation();
	fillBuffer(sprite.position[0], sprite.position[1], sprite.image->width, sprite.image->height, sprite.image->data, sprite.image->transparent, FALSE);
	popTransformation();
}

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

void initialize() {
	CONSOLE_FONT_INFOEX font;
	DWORD mode;
	window = GetConsoleWindow();
	store = GetStdHandle(STD_OUTPUT_HANDLE);
	input = GetStdHandle(STD_INPUT_HANDLE);
	screen = CreateConsoleScreenBuffer(GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	GetConsoleMode(store, &mode);
	SetConsoleMode(screen, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	font.cbSize = sizeof(CONSOLE_FONT_INFOEX);
	GetCurrentConsoleFontEx(screen, FALSE, &font);
	font.dwFontSize.X = 1;
	font.dwFontSize.Y = 1;
	SetCurrentConsoleFontEx(screen, FALSE, &font);
	bufferSize = bufferSizeCoord.X * bufferSizeCoord.Y;
	buffer = (char*)malloc(bufferSize);
	pushWindowSize();
	SetConsoleActiveScreenBuffer(screen);
	popWindowSize();
	loadBitmap("assets/hero.bmp", &hero, BLACK);
	loadBitmap("assets/Gochi.bmp", &background, NULL_COLOR);
	initSprite(&heroSprite, "Hero", &hero);
	heroSprite.shadowScale = 0.75;
	heroSprite.shadowOffset[1] = 10.0;
}

BOOL pollEvents() {
	DWORD nofEvents;
	ReadConsoleInput(input, inputRecords, NOF_MAX_EVENTS, &nofEvents);
	for(int i = 0;i < nofEvents;i += 1) {
		switch(inputRecords[i].EventType) {
			case KEY_EVENT:
			KEY_EVENT_RECORD *keyEvent = &inputRecords[i].Event.KeyEvent;
			if(keyEvent->bKeyDown) {
				switch(keyEvent->uChar.AsciiChar) {
					case 'q': return FALSE;
					case 'w':
						heroSprite.position[1] -= 2;
						break;
					case 's':
						heroSprite.position[1] += 2;
						break;
					case 'a':
						heroSprite.position[0] -= 2;
						break;
					case 'd':
						heroSprite.position[0] += 2;
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
	static char colors[][10] = {
		"\x1b[40m ", "\x1b[41m ", "\x1b[42m ", "\x1b[43m ",
		"\x1b[44m ", "\x1b[45m ", "\x1b[46m ", "\x1b[47m ",
		"\x1b[100m ", "\x1b[101m ", "\x1b[102m ", "\x1b[103m ",
		"\x1b[104m ", "\x1b[105m ", "\x1b[106m ", "\x1b[107m "
	};
	static CONSOLE_CURSOR_INFO cursor = { 1, FALSE };
	DWORD nofWritten;
	size_t bufferSize = (bufferSizeCoord.X * 8 + 20) * bufferSizeCoord.Y;
	char temp[20], temp2[10];
	char *data = (char*)malloc(bufferSize);
	char previousColor = 0xFF;
	size_t dataIndex = 0;
	data[0] = '\0';
	unsigned int row;
	for(row = 0;row < bufferSizeCoord.Y;row++) {
		temp[0] = '\0';
		strcat_s(temp, sizeof(temp), "\x1b[");
		_itoa_s(row, temp2, sizeof(temp2), 10);
		strcat_s(temp, sizeof(temp), temp2);
		strcat_s(temp, sizeof(temp), ";0H");
		strcat_s(&data[dataIndex], bufferSize, temp);
		dataIndex += strlen(temp2) + 4;
		unsigned int col;
		for(col = 0;col < bufferSizeCoord.X;col++) {
			size_t index = bufferSizeCoord.X * row + col;
			if(buffer[index] == previousColor) {
				strcat_s(&data[dataIndex], bufferSize, " ");
				dataIndex += 1;
			} else {
				strcat_s(&data[dataIndex], bufferSize, colors[buffer[index]]);
				dataIndex += strlen(colors[buffer[index]]);
			}
			previousColor = buffer[index];
		}
	}
	WriteConsole(screen, data, strlen(data), &nofWritten, NULL);
	SetConsoleCursorInfo(screen, &cursor);
	free(data);
}

void draw() {
	memset(buffer, 0, bufferSize);
	clearTransformation();
	drawRect(0, 0, bufferSizeCoord.X, bufferSizeCoord.Y, DARK_BLUE);
	drawSprite(heroSprite);
	flush();
}

void loop() {
	while(TRUE) {
		if(!pollEvents()) return;
		draw();
	}
}

void deinitialize() {
	pushWindowSize();
	SetConsoleActiveScreenBuffer(store);
	popWindowSize();
	CloseHandle(screen);
	CloseHandle(store);
	discardImage(hero);
	free(buffer);
}

int main() {
	initialize();
	loop();
	deinitialize();
	return 0;
}
