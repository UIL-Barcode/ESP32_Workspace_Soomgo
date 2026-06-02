#pragma once

#ifndef SCREEN_TOOLS_H
#define SCREEN_TOOLS_H
#include <stdbool.h>

#ifndef SCREEN_WIDTH
#define SCREEN_WIDTH 32
#endif

#ifndef SCREEN_HEIGHT
#define SCREEN_HEIGHT 8
#endif

#define INCREASE(X, Y) ((X) = ((X) + 1) % (Y))
#define DECREASE(X, Y) ((X) = ((X) + (Y) - 1) % (Y))

#define MOVING_SCREEN_LEFT	(1 << 0)
#define MOVING_SCREEN_RIGHT	(1 << 1)
#define MOVING_SCREEN_UP	(1 << 2)
#define MOVING_SCREEN_DOWN	(1 << 3)

typedef struct
{
	int x;
	int y;
	int step;
}moving;

typedef struct
{
	int max_r;
	int max_c;
	int dir;
	moving act;
	char* data;
	char** screen;
}screen_setup_t;

extern int letter_rows;
extern int letter_cols;
extern int total_lines;
extern char** frame;

int make_screen(screen_setup_t* setup);
void moving_screen(screen_setup_t* setup);
void free_screen();
void print_screen(screen_setup_t* setup);
void print_screen_int(screen_setup_t* setup);

#endif