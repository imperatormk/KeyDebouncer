#include "kdbnc_main.h"

//Type this string at any point to quit the key debouncer (if the history queue size is large enough to support the exit string)
const std::string EXIT_STR = "kdbnc::exit"; 
// The handle to the actual keypress hook
HHOOK _hook;
// This struct contains the data received by the hook callback. i.e. the information about the key that was pressed.
KBDLLHOOKSTRUCT kbdStruct;

// The time window used to make the "block/no-block" decision
int time_window;
// A deque of size HISTORY_QUEUE_SIZE that records the past HISTORY_QUEUE_SIZE keypresses
// Currently just used for detecting an exit string sequence, but could potentially be used for more intelligent debouncing methods that take into account past typing history of each individual user
std::deque<keypress_t> typingHistory;

// This is the callback function. Consider it the event that is raised when, in this case, a key is pressed.
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
			
			FILETIME time;
			GetSystemTimeAsFileTime(&time);
			// Make the time more usable and convert from "0.1uS increments/100nS increments since Jan 1st 1601" to "milliseconds since Jan 1st 1601"
			LONGLONG curr_time = ((LONGLONG)time.dwLowDateTime + ((LONGLONG)(time.dwHighDateTime) << 32LL))/10000LL;
			// Compare the user input with the previously input char. If it's a repeated char within a given time window, abort the future CallNextHookEx calls (i.e. consume the input)
			should_db = ShouldDebounce(kbdStruct.vkCode, kbdStruct.scanCode, curr_time);
			if (ShouldExit()) {
				ReleaseHook();
				PostQuitMessage(0);
				return CallNextHookEx(_hook, nCode, wParam, lParam);
			}
		}
	}
	
	#ifdef DEBUG
	std::stringstream strm;
	for (keypress_t kp : typingHistory) {
		strm << kp.keystroke;
	}
	std::cout << "Current typingHistory deque " << strm.str() << std::endl;
	#endif
		
	// If we shouldn't debounce, call the next hook in the hook chain. Otherwise, return 1 and consume the input
	return (should_db) ? 1 : CallNextHookEx(_hook, nCode, wParam, lParam);
}

void SetHook()
{	
	std::cout << "Setting hook..." << std::endl;
	typingHistory.resize(HISTORY_QUEUE_SIZE, {});
	// Set the hook and set it to use the callback function above
	// WH_KEYBOARD_LL means it will set a low level keyboard hook. More information about it at MSDN.
	// The last 2 parameters are NULL, 0 because the callback function is in the same thread and window as the
	// function that sets and releases the hook.
	if (!(_hook = SetWindowsHookEx(WH_KEYBOARD_LL, HookCallback, NULL, 0)))
	{
		std::cout << "Failed to install hook!" << std::endl;
	}
}

void ReleaseHook()
{
	std::cout << "Releasing hook..." << std::endl;
	UnhookWindowsHookEx(_hook);
}

bool ShouldDebounce(unsigned int keystroke, unsigned int scan_code, LONGLONG curr_time)
{
	// ignore mouse clicks or certain keys like backspace, shift, ctrl, etc.
	if ((keystroke == 1) || (keystroke == 2)) return false; 
		
	//Convert the VK_CODE into a standard ASCII value (despite the fact that it's not really needed in theory) because I just like it better that way
	//Otherwise, debug logging and/or exit-program-detection is worse
	BYTE kbstate[256];
	GetKeyboardState(kbstate);
	SHORT shiftstate = GetAsyncKeyState(VK_SHIFT);
	kbstate[VK_SHIFT] = ((shiftstate & 0x8000) >> (sizeof(SHORT) - sizeof(BYTE)) * 8) | (shiftstate & 0x0001);
	WORD conversion_arr[2];
	int charsCopied = ToAsciiEx(keystroke, scan_code, kbstate, conversion_arr, 0, 0);
	if (charsCopied != 1) {
		#ifdef DEBUG
		std::cout << "ToAsciiEx returned something other than 1 character for this VK Code conversion: {" << keystroke << "}, {" << curr_time << "}" << std::endl;
		#endif
		return false;
	}
	unsigned char converted_keystroke = conversion_arr[0];
	
	#ifdef DEBUG
	std::cout << "Previous keystroke: " << typingHistory.back().keystroke << " This keystroke: " << converted_keystroke << std::endl;
	std::cout << "Time difference: " << curr_time - typingHistory.back().time_of_press << std::endl;
	#endif
		
	bool result = (converted_keystroke == typingHistory.back().keystroke && (curr_time - typingHistory.back().time_of_press) < time_window) ? true : false;
	
	if (result)  {
		std::cout << "Blocking keystroke " << converted_keystroke << " at time " << curr_time << std::endl;
	}
	
	if (!typingHistory.empty() && typingHistory.size() + 1 >= HISTORY_QUEUE_SIZE) {
		typingHistory.pop_front();
	}
	typingHistory.push_back((keypress_t) { converted_keystroke, curr_time });
	
	return result;
}

bool ShouldExit() 
{
	if (HISTORY_QUEUE_SIZE < EXIT_STR.length()) {
		return false;
	}
	for (int i = 0; i <= HISTORY_QUEUE_SIZE - EXIT_STR.length(); i++) {
		for (int j = 0; j < EXIT_STR.length(); j++) {
			if (EXIT_STR[j] != tolower(typingHistory[i+j].keystroke)) {
				goto continue_outer_loop;
			}
		}
		return true;
		continue_outer_loop:;
	}
	return false;
}

int main(int argc, char** argv)
{
	if (argc > 1) {
		time_window = std::stoi(argv[1]);
	} else {
		time_window = DEFAULT_TIME_WINDOW;
	}
	std::cout << "Starting key debouncer with time window of " << time_window << " ms..." << std::endl << "Type \"" << EXIT_STR << "\" to exit" << std::endl;
	
	if (HISTORY_QUEUE_SIZE < EXIT_STR.length()) {
		std::cout << "Note: HISTORY_QUEUE_SIZE is defined as less than the length of the current exit string. The process will need to be killed manually" << std::endl;
	}
	
	SetHook();

	MSG msg;
	//GetMessage is a blocking call that will only actually ever receive a message when the HookCallback posts an exit message after detecting the ShouldExit exit condition.
	//Thus, when we *do* receive a message, regardless of what it is, that means we should exit
	GetMessage(&msg, NULL, 0, 0);
	std::cout << "Key Debouncer exiting..." << std::endl;
	return 0;
}
