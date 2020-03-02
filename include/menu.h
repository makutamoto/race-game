#ifndef MENU_H
#define MENU_H

typedef enum {
	GO_TO_TITLE, RETRY, QUIT
} MenuItem;

void initMenu(void);
void startMenu(void);

#endif
