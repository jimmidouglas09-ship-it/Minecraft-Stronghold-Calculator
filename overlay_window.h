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

// Window class name
extern WCHAR szOverlayClass[];

// Overlay window handle and state
extern HWND hOverlayWnd;
extern bool overlayVisible;

// Function declarations
ATOM MyRegisterOverlayClass(HINSTANCE hInstance);
BOOL InitOverlay(HINSTANCE hInstance);
LRESULT CALLBACK OverlayProc(HWND, UINT, WPARAM, LPARAM);

// Overlay management functions
void UpdateOverlay();
void ShowOverlay();
void HideOverlay();