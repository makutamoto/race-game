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
HANDLE screen;
char *buffer;
size_t bufferSize;
COORD bufferSizeCoord = { 200, 200 };
INPUT_RECORD inputRecords[NOF_MAX_EVENTS];

int32_t width, height;
uint32_t *image;

typedef struct {
	unsigned int width;
	unsigned int height;
	char *data;
} Image;

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

void setPixelColor(unsigned int x, unsigned int y, char color) {
	// matrix;
	buffer[bufferSizeCoord.X * y + x] = color;
}

void drawImage(unsigned int ox, unsigned int oy) {
	for(int32_t y = 0;y < height;y++) {
		for(int32_t x = 0;x < width;x++) {
			size_t index = y * (int32_t)ceil(width / 8.0) + x / 8;
			uint8_t location = x % 8;
			uint8_t highLow = location % 2;
			uint8_t color;
			char attribute[10];
			if(highLow) {
				color = (image[index] >> ((location - 1) * 4)) & 0x0F;
			} else {
				color = (image[index] >> ((location + 1) * 4)) & 0x0F;
			}
			setPixelColor(ox + x, oy + height - 1 - y, color);
		}
	}
}

int drawRect(HANDLE screen, unsigned int px, unsigned int py, unsigned int width, unsigned int height) {
	DWORD nofWritten;
	COORD cursor;
	size_t size = width + 1;
	char *line = (char*)malloc(size);
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

void drawCircle(unsigned int ox, unsigned int oy, unsigned int radius, char color) {
	int y;
	for(y = 0;y < 2 * radius;y++) {
		int x;
		for(x = 0;x < 2 * radius;x++) {
			int cx = x - radius;
			int cy = y - radius;
			if(radius * radius >= cx * cx + cy * cy) setPixelColor(x + ox, y + oy, color);
		}
	}
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
	if(initLogger()) printf("Failed to Initialize the logger.");
	loadBmp("hero.bmp", &image, &width, &height);
	CONSOLE_CURSOR_INFO cursor = { 1, FALSE };
	window = GetConsoleWindow();
	store = GetStdHandle(STD_OUTPUT_HANDLE);
	input = GetStdHandle(STD_INPUT_HANDLE);
	screen = CreateConsoleScreenBuffer(GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	SetConsoleTitle("Picture");
	GetConsoleMode(store, &mode);
	SetConsoleMode(screen, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
	font.cbSize = sizeof(CONSOLE_FONT_INFOEX);
	GetCurrentConsoleFontEx(screen, FALSE, &font);
	font.dwFontSize.X = 1;
	font.dwFontSize.Y = 1;
	SetCurrentConsoleFontEx(screen, FALSE, &font);
	SetConsoleCursorInfo(screen, &cursor);
	bufferSize = bufferSizeCoord.X * bufferSizeCoord.Y;
	buffer = (char*)malloc(bufferSize);
	pushWindowSize();
	SetConsoleActiveScreenBuffer(screen);
	popWindowSize();
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

void flush() {
	DWORD nofWritten;
	size_t bufferSize = (bufferSizeCoord.X * 8 + 20) * bufferSizeCoord.Y;
	char temp[20], temp2[10];
	char *data = (char*)malloc(bufferSize);
	char previousColor = 0xFF;
	data[0] = '\0';
	unsigned int row;
	for(row = 0;row < bufferSizeCoord.Y;row++) {
		temp[0] = '\0';
		strcat_s(temp, sizeof(temp), "\x1b[");
		_itoa_s(row, temp2, sizeof(temp2), 10);
		strcat_s(temp, sizeof(temp), temp2);
		strcat_s(temp, sizeof(temp), ";0H");
		strcat_s(data, bufferSize, temp);
		unsigned int col;
		for(col = 0;col < bufferSizeCoord.X;col++) {
			size_t index = bufferSizeCoord.X * row + col;
			if(buffer[index] == previousColor) {
				strcat_s(data, bufferSize, " ");
			} else {
				temp[0] = '\0';
				strcat_s(temp, sizeof(temp), "\x1b[");
				_itoa_s(buffer[index] + (buffer[index] < 8 ? 40 : 92), temp2, sizeof(temp2), 10);
				strcat_s(temp, sizeof(temp), temp2);
				strcat_s(temp, sizeof(temp), "m ");
				strcat_s(data, bufferSize, temp);
			}
			previousColor = buffer[index];
		}
	}
	WriteConsole(screen, data, strlen(data), &nofWritten, NULL);
	free(data);
}

int y = 0;
void draw() {
	memset(buffer, 0, bufferSize);
	drawCircle(50, 50, 50, 1);
	drawImage(100, 100);
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
	free(buffer);
	free(image);
	closeLogger();
}

int main() {
	initialize();
	loop();
	deinitialize();
	return 0;
}
