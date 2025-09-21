#pragma once
#include "Common.h"

class MouseAngleTracker;
extern MouseAngleTracker* mouseTracker;

class WindowProc {
public:
    static LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    static void handleCreate(HWND hwnd);
    static void handleTimer(HWND hwnd);
    static void handlePaint(HWND hwnd);
    static void handleDestroy();

    // Track hotkey states to detect key presses (not just holds)
    static bool hotkeyPressed[256];
};