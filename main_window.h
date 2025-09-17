#pragma once
#define NOMINMAX
#include "common.h"


// Hotkey customization variables
extern int currentTabHotkey;
extern int currentF4Hotkey;
extern bool waitingForTabHotkey;
extern bool waitingForF4Hotkey;

// Function declarations
std::wstring GetKeyName(int vkCode);
void RegisterHotkeys(HWND hWnd);

// Main window procedure
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// Clipboard utility function
void CopyStrongholdResultsToClipboard();