#include "WindowProc.h"
#include "MouseTracker.h"
#include "AppController.h"
#include "UIRenderer.h"
#include "OptionsWindow.h"

// Static member initialization - now handles full range of keys
bool WindowProc::hotkeyPressed[256] = { false };

LRESULT CALLBACK WindowProc::WindowProcedure(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_CREATE:
        handleCreate(hwnd);
        break;
    case WM_INPUT:
        if (mouseTracker) {
            mouseTracker->processRawInput(lParam);
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    case WM_TIMER:
        handleTimer(hwnd);
        break;
    case WM_PAINT:
        handlePaint(hwnd);
        break;
    case WM_NCHITTEST:
        return HTCAPTION; // Make window draggable
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        }
        break;
    case WM_DESTROY:
        handleDestroy();
        break;
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void WindowProc::handleCreate(HWND hwnd) {
    SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    mouseTracker = new MouseAngleTracker(hwnd);
    AppController::initialize();
    SetTimer(hwnd, 1, 16, NULL);
}

void WindowProc::handleTimer(HWND hwnd) {
    // Handle options window
    if (appState.showOptionsWindow) {
        OptionsWindow::showOptionsDialog(hwnd);
        appState.showOptionsWindow = false;
    }

    // Check all hotkeys using settings - now supports any valid key
    bool currentCalibrate = (GetAsyncKeyState(Settings::hotkeys.calibrateKey) & 0x8000) != 0;
    bool currentTrackAngle = (GetAsyncKeyState(Settings::hotkeys.trackAngleKey) & 0x8000) != 0;
    bool currentCalculate = (GetAsyncKeyState(Settings::hotkeys.calculateKey) & 0x8000) != 0;
    bool currentReset = (GetAsyncKeyState(Settings::hotkeys.resetKey) & 0x8000) != 0;
    bool currentOptions = (GetAsyncKeyState(Settings::hotkeys.optionsKey) & 0x8000) != 0;
    bool currentSecondThrow = (GetAsyncKeyState(Settings::hotkeys.secondThrowKey) & 0x8000) != 0;

    // Get key indices for the pressed state array (ensure within bounds)
    int calibrateIdx = Settings::hotkeys.calibrateKey < 256 ? Settings::hotkeys.calibrateKey : 0;
    int trackIdx = Settings::hotkeys.trackAngleKey < 256 ? Settings::hotkeys.trackAngleKey : 0;
    int calculateIdx = Settings::hotkeys.calculateKey < 256 ? Settings::hotkeys.calculateKey : 0;
    int resetIdx = Settings::hotkeys.resetKey < 256 ? Settings::hotkeys.resetKey : 0;
    int optionsIdx = Settings::hotkeys.optionsKey < 256 ? Settings::hotkeys.optionsKey : 0;
    int secondThrowIdx = Settings::hotkeys.secondThrowKey < 256 ? Settings::hotkeys.secondThrowKey : 0;

    // Calibrate hotkey
    if (currentCalibrate && !hotkeyPressed[calibrateIdx]) {
        if (appState.mode == MODE_CALIBRATING) {
            AppController::finishCalibration();
        }
        else {
            AppController::startCalibration();
        }
        InvalidateRect(hwnd, NULL, TRUE);
    }

    // Track angle hotkey
    if (currentTrackAngle && !hotkeyPressed[trackIdx]) {
        if (appState.mode == MODE_TRACKING_ANGLE || appState.mode == MODE_SECOND_TRACKING_ANGLE) {
            AppController::finishAngleTracking();
        }
        else if (appState.mouseSensitivityCalibrated &&
            (appState.mode == MODE_READY || appState.mode == MODE_SECOND_THROW_READY)) {
            AppController::startAngleTracking();
        }
        InvalidateRect(hwnd, NULL, TRUE);
    }

    // Calculate hotkey
    if (currentCalculate && !hotkeyPressed[calculateIdx]) {
        if (appState.mode == MODE_ANGLE_CAPTURED || appState.mode == MODE_SECOND_ANGLE_CAPTURED) {
            AppController::readCoordinatesAndCalculate();
            InvalidateRect(hwnd, NULL, TRUE);
        }
    }

    // Reset hotkey
    if (currentReset && !hotkeyPressed[resetIdx]) {
        AppController::resetApp();
        InvalidateRect(hwnd, NULL, TRUE);
    }

    // Options hotkey
    if (currentOptions && !hotkeyPressed[optionsIdx]) {
        AppController::showOptions();
    }

    // Second throw hotkey
    if (currentSecondThrow && !hotkeyPressed[secondThrowIdx]) {
        if (appState.mode == MODE_RESULTS && appState.firstThrow.isValid) {
            AppController::startSecondThrow();
            InvalidateRect(hwnd, NULL, TRUE);
        }
    }

    // Update pressed states
    hotkeyPressed[calibrateIdx] = currentCalibrate;
    hotkeyPressed[trackIdx] = currentTrackAngle;
    hotkeyPressed[calculateIdx] = currentCalculate;
    hotkeyPressed[resetIdx] = currentReset;
    hotkeyPressed[optionsIdx] = currentOptions;
    hotkeyPressed[secondThrowIdx] = currentSecondThrow;

    // Update display if tracking
    if (appState.mode == MODE_CALIBRATING ||
        appState.mode == MODE_TRACKING_ANGLE ||
        appState.mode == MODE_SECOND_TRACKING_ANGLE) {
        InvalidateRect(hwnd, NULL, TRUE);
    }
}

void WindowProc::handlePaint(HWND hwnd) {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    RECT rect;
    GetClientRect(hwnd, &rect);
    UIRenderer::renderUI(hdc, rect);
    EndPaint(hwnd, &ps);
}

void WindowProc::handleDestroy() {
    KillTimer(GetActiveWindow(), 1);
    delete mouseTracker;
    mouseTracker = nullptr;
    PostQuitMessage(0);
}