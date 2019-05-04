#include<stdio.h>
#include<math.h>
#include<stdint.h>
#include<windows.h>

#include "./include/logger.h"
#include "./include/colors.h"
#include "./include/bitmap.h"

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define NOF_MAX_EVENTS 10

HWND window;
HANDLE store;
HANDLE input;
HANDLE screen1, screen2;
INPUT_RECORD inputRecords[NOF_MAX_EVENTS];

int32_t width, height;
uint32_t *image;

BOOL loadBmp(char *fileName, uint32_t **img, int32_t *width, int32_t *height) {
	FILE *file;
	BitmapHeader header;
	BitmapInfoHeader infoHeader;
	uint32_t *image;
	if(fopen_s(&file, fileName, "rb")) {
		fprintf(stderr, "Failed to open the file '%s'\n", fileName);
		return FALSE;
	}
	if(fread_s(&header, sizeof(BitmapHeader), 1, sizeof(BitmapHeader), file) != sizeof(BitmapHeader)) {
		fprintf(stderr, "BMP header does not exist in the file '%s'\n", fileName);
		return FALSE;
	}
	if(header.magicNumber[0] != 'B' || header.magicNumber[1] != 'M') {
		fprintf(stderr, "Unknown magic number.\n");
		return FALSE;
	}
	if(header.dibSize != 40) {
		fprintf(stderr, "Unknown DIB header type.\n");
		return  FALSE;
	}
	if(fread_s(&infoHeader, sizeof(BitmapInfoHeader), 1, sizeof(BitmapInfoHeader), file) != sizeof(BitmapInfoHeader)) {
		fprintf(stderr, "Failed to load Bitmap info header.\n");
		return FALSE;
	}
	if(infoHeader.width < 0 || infoHeader.height < 0) {
		fprintf(stderr, "Negative demensions are not supported.\n");
		return FALSE;
	}
	if(infoHeader.compressionMethod != 0) {
		fprintf(stderr, "Compressions are not supported.\n");
		return FALSE;
	}
	if(fseek(file, header.offset, SEEK_SET)) {
		fprintf(stderr, "Image data does not exist.\n");
		return FALSE;
	}
	image = (uint32_t*)malloc(infoHeader.imageSize);
	if(fread_s(image, infoHeader.imageSize, 1, infoHeader.imageSize, file) != infoHeader.imageSize) {
		fprintf(stderr, "Image data is corrupted.\n");
		return FALSE;
	}
	*width = infoHeader.width;
	*height = infoHeader.height;
	*img = image;
	fclose(file);
	return TRUE;
}

void drawImage() {
	DWORD nofWritten;
	COORD cursor;
	SetConsoleTextAttribute(screen1, BG_BLACK | FG_WHITE);
	for(int32_t y = 0;y < height;y++) {
		for(int32_t x = 0;x < width;x++) {
			size_t index = y * (int32_t)ceil(width / 8.0) + x / 8;
			uint8_t location = x % 8;
			uint8_t highLow = location % 2;
			uint8_t color;
			if(highLow) {
				color = (image[index] >> ((location - 1) * 4)) & 0x0F;
			} else {
				color = (image[index] >> ((location + 1) * 4)) & 0x0F;
			}
			color = (color & 0x8) | ((color << 2) & 0x4) | (color & 0x2) | ((color >> 2) & 0x1);
			cursor.X = x;
			cursor.Y = height - 1 - y;
			SetConsoleCursorPosition(screen1, cursor);
			SetConsoleTextAttribute(screen1, color << 4);
			WriteConsole(screen1, " ", 2, &nofWritten, NULL);
		}
	}
	SetConsoleTextAttribute(screen1, FG_WHITE | BG_BLACK);
}

int drawRect(HANDLE screen, unsigned int px, unsigned int py, unsigned int width, unsigned int height) {
	DWORD nofWritten;
	COORD cursor;
	size_t size = width + 1;
	char *line = malloc(size);
	if(line == NULL) return FALSE;
	unsigned int i;
	for(i = 0;i < size - 1;i++) line[i] = ' ';
	line[i] = '\0';
	SetConsoleTextAttribute(screen, BG_WHITE);
	cursor.X = px;
	unsigned int y;
	for(y = py;y < py + height;y++) {
		cursor.Y = y;
		SetConsoleCursorPosition(screen, cursor);
		WriteConsole(screen, line, size, &nofWritten, NULL);
	}
	SetConsoleTextAttribute(screen, FG_WHITE | BG_BLACK);
	free(line);
	return TRUE;
}

int drawCircle(HANDLE screen, unsigned int px, unsigned int py, unsigned int radius) {
	DWORD nofWritten;
	COORD cursor;
	unsigned int edgeSize = radius * 2;
	size_t size = radius * 2 * 8 + 1;
	char *line = malloc(size);
	if(line == NULL) return FALSE;
	SetConsoleTextAttribute(screen, BG_WHITE);
	cursor.X = px;
	unsigned int y;
	for(y = 0;y < edgeSize;y++) {
		cursor.Y = py + y;
		SetConsoleCursorPosition(screen, cursor);
		line[0] = '\0';
		unsigned int x;
		for(x = 0;x < edgeSize;x++) {
			int xc = x - radius;
			int yc = y - radius;
			if(radius * radius >= xc * xc + yc * yc) {
				strcat_s(line, size, "\x1b[107m ");
			} else {
				strcat_s(line, size, "\x1b[40m ");
			}
		}
		WriteConsole(screen, line, strlen(line), &nofWritten, NULL);
	}
	SetConsoleTextAttribute(screen, FG_WHITE | BG_BLACK);
	free(line);
	return TRUE;
}

RECT storedSize;
void pushWindowSize() {
	WINDOWINFO info;
	info.cbSize = sizeof(WINDOWINFO);
	GetWindowInfo(window, &info);
	storedSize = info.rcWindow;
}
void popWindwoSize() {
	MoveWindow(window, storedSize.left, storedSize.top, storedSize.right - storedSize.left, storedSize.bottom - storedSize.top, FALSE);
}

void initialize() {
	CONSOLE_FONT_INFOEX font;
	COORD size = {10000, 10000};
	DWORD mode;
	if(initLogger()) printf("Failed to Initialize the logger.");
	loadBmp("hero.bmp", &image, &width, &height);
	CONSOLE_CURSOR_INFO cursor = { 1, FALSE };
	window = GetConsoleWindow();
	store = GetStdHandle(STD_OUTPUT_HANDLE);
	input = GetStdHandle(STD_INPUT_HANDLE);
	screen1 = CreateConsoleScreenBuffer(GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	screen2 = CreateConsoleScreenBuffer(GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleTitle("Picture");
	GetConsoleMode(store, &mode);
	mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
	SetConsoleMode(screen1, mode);
	SetConsoleMode(screen2, mode);
	font.cbSize = sizeof(CONSOLE_FONT_INFOEX);
	SetConsoleScreenBufferSize(screen1, size);
	SetConsoleScreenBufferSize(screen2, size);
	GetCurrentConsoleFontEx(screen1, FALSE, &font);
	font.dwFontSize.X = 1;
	font.dwFontSize.Y = 1;
	SetCurrentConsoleFontEx(screen1, FALSE, &font);
	SetCurrentConsoleFontEx(screen2, FALSE, &font);
	SetConsoleCursorInfo(screen1, &cursor);
	SetConsoleCursorInfo(screen2, &cursor);
	pushWindowSize();
	SetConsoleActiveScreenBuffer(screen1);
	popWindwoSize();
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
				}
			}
			break;
			default: break;
		}
	}
	return TRUE;
}

void loop() {
	while(TRUE) {
		if(!pollEvents()) return;
		drawCircle(screen1, 0, 0, 50);
	}
}

void deinitialize() {
	SetConsoleActiveScreenBuffer(store);
	CloseHandle(screen2);
	CloseHandle(screen1);
	CloseHandle(store);
	free(image);
	closeLogger();
}

int main() {
	initialize();
	loop();
	deinitialize();
	return 0;
}
