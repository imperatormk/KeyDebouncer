#include <Windows.h>
#include <time.h>
#include <iostream>
#include <cstdio>
#include <atomic>

//Window to use such that, if a repeated keystroke appears within this number of millseconds, it is dropped
#define DEFAULT_TIME_WINDOW 30

// variable to store the HANDLE to the hook. Don't declare it anywhere else then globally
// or you will get problems since every function uses this variable.
HHOOK _hook;

// This struct contains the data received by the hook callback. As you see in the callback function
// it contains the thing you will need: vkCode = virtual key code.
KBDLLHOOKSTRUCT kbdStruct;

bool ShouldDebounce(int key_stroke, LONG time_ms);

std::atomic<int> prevKeyStroke;
std::atomic<LONG> prevTimeRecord;

int time_window_ms;

// This is the callback function. Consider it the event that is raised when, in this case, 
// a key is pressed.
LRESULT __stdcall HookCallback(int nCode, WPARAM wParam, LPARAM lParam)
{
	bool should_db = false;
	if (nCode >= 0)
	{
		// the action is valid: HC_ACTION.
		if (wParam == WM_KEYDOWN)
		{
			// lParam is the pointer to the struct containing the data needed, so cast and assign it to kdbStruct.
			kbdStruct = *((KBDLLHOOKSTRUCT*)lParam);
			
			SYSTEMTIME time;
			GetSystemTime(&time);
			LONG time_ms = (time.wSecond * 1000) + time.wMilliseconds;
			// Compare the user input with the previously input char. If it's a repeated char within a given time window, abort the future CallNextHookEx calls (i.e. consume the input)
			should_db = ShouldDebounce(kbdStruct.vkCode, time_ms);
		}
	}

	// If we shouldn't debounce, call the next hook in the hook chain. Otherwise, return 1 and consume the input
	return (should_db) ? 1 : CallNextHookEx(_hook, nCode, wParam, lParam);
}

void SetHook()
{
	//Initialize our atomic variables
	prevKeyStroke = 1;
	prevTimeRecord = 0;
	
	// Set the hook and set it to use the callback function above
	// WH_KEYBOARD_LL means it will set a low level keyboard hook. More information about it at MSDN.
	// The last 2 parameters are NULL, 0 because the callback function is in the same thread and window as the
	// function that sets and releases the hook.
	if (!(_hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0)))
	{
		MessageBox(NULL, "Failed to install hook!", "Error", MB_ICONERROR);
	}
}

void ReleaseHook()
{
	UnhookWindowsHookEx(_hook);
}

bool ShouldDebounce(int key_stroke, LONG time_ms)
{
	// ignore mouse clicks or commonly held down keys like backspace
	if ((key_stroke == 1) || (key_stroke == 2) || (key_stroke == VK_BACK))
		return false; 
	
	bool result = (key_stroke == prevKeyStroke && (time_ms - prevTimeRecord) < time_window_ms) ? true : false;
	
	if (result) {
		printf("Blocking keystroke %c at time %lu\r\n", key_stroke, time_ms);
	}
	
	prevKeyStroke = key_stroke;
	prevTimeRecord = time_ms;
	
	return result;
}

int main(int argc, char** argv)
{
	if (argc > 1) {
		time_window_ms = std::stoi(argv[1]);
	} else {
		time_window_ms = DEFAULT_TIME_WINDOW;
	}
	printf("Starting key debouncer with time window %d...\r\n", time_window_ms);
	
	// Set the hook
	SetHook();

	// loop to keep the console application running.
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
	}
}
