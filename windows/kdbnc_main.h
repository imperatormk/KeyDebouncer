#ifndef KDBNC_MAIN_H
#define KDBNC_MAIN_H

#include <Windows.h>
#include <time.h>
#include <iostream>
#include <sstream>
#include <ctype.h>
#include <deque>

//Default window to use such that, if a repeated keystroke appears within this number of milliseconds, it is dropped
#define DEFAULT_TIME_WINDOW 100
#define HISTORY_QUEUE_SIZE 20

void SetHook();
void ReleaseHook();
bool ShouldDebounce(unsigned int keystroke, unsigned int scan_code, LONGLONG curr_time);
bool ShouldExit();

typedef struct keypress_s {
	unsigned char keystroke;
	LONGLONG time_of_press;
} keypress_t;

#endif