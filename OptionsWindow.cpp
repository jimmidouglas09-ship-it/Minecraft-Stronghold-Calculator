#include "OptionsWindow.h"
#include <commctrl.h>

// Static variables for the persistent window
static HWND g_hOptionsWnd = NULL;
static bool g_isCapturing = false;
static int g_capturingControl = -1;
static WNDPROC g_originalEditProc = NULL;

// Control IDs
#define IDC_CALIBRATE_EDIT      2001
#define IDC_TRACK_EDIT          2002
#define IDC_CALCULATE_EDIT      2003
#define IDC_SECOND_THROW_EDIT   2004
#define IDC_RESET_EDIT          2005
#define IDC_OPTIONS_EDIT        2006
#define IDC_SAVE_BTN           2007
#define IDC_DEFAULTS_BTN       2008
#define IDC_CLOSE_BTN          2009

// Custom edit control procedure to capture key presses
LRESULT CALLBACK EditKeyCapture(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYDOWN || msg == WM_SYSKEYDOWN) {
        if (g_isCapturing) {
            int vkCode = (int)wParam;

            // Don't capture Tab, Enter, or Escape for UI navigation
            if (vkCode == VK_TAB || vkCode == VK_RETURN || vkCode == VK_ESCAPE) {
                g_isCapturing = false;
                g_capturingControl = -1;
                SetWindowTextA(hwnd, "Click to set key");
                return CallWindowProc(g_originalEditProc, hwnd, msg, wParam, lParam);
            }

            // Get the key name and set it
            string keyName = Settings::virtualKeyToString(vkCode);
            SetWindowTextA(hwnd, keyName.c_str());

            // Update the corresponding setting
            switch (g_capturingControl) {
            case IDC_CALIBRATE_EDIT: Settings::hotkeys.calibrateKey = vkCode; break;
            case IDC_TRACK_EDIT: Settings::hotkeys.trackAngleKey = vkCode; break;
            case IDC_CALCULATE_EDIT: Settings::hotkeys.calculateKey = vkCode; break;
            case IDC_SECOND_THROW_EDIT: Settings::hotkeys.secondThrowKey = vkCode; break;
            case IDC_RESET_EDIT: Settings::hotkeys.resetKey = vkCode; break;
            case IDC_OPTIONS_EDIT: Settings::hotkeys.optionsKey = vkCode; break;
            }

            g_isCapturing = false;
            g_capturingControl = -1;
            return 0; // Consume the key press
        }
    }
    else if (msg == WM_LBUTTONDOWN) {
        if (!g_isCapturing) {
            g_isCapturing = true;
            g_capturingControl = GetDlgCtrlID(hwnd);
            SetWindowTextA(hwnd, "Press a key...");
        }
    }
    else if (msg == WM_KILLFOCUS) {
        if (g_isCapturing && GetDlgCtrlID(hwnd) == g_capturingControl) {
            g_isCapturing = false;
            g_capturingControl = -1;
            // Restore the current key name
            int currentKey = VK_F6;
            switch (GetDlgCtrlID(hwnd)) {
            case IDC_CALIBRATE_EDIT: currentKey = Settings::hotkeys.calibrateKey; break;
            case IDC_TRACK_EDIT: currentKey = Settings::hotkeys.trackAngleKey; break;
            case IDC_CALCULATE_EDIT: currentKey = Settings::hotkeys.calculateKey; break;
            case IDC_SECOND_THROW_EDIT: currentKey = Settings::hotkeys.secondThrowKey; break;
            case IDC_RESET_EDIT: currentKey = Settings::hotkeys.resetKey; break;
            case IDC_OPTIONS_EDIT: currentKey = Settings::hotkeys.optionsKey; break;
            }
            SetWindowTextA(hwnd, Settings::virtualKeyToString(currentKey).c_str());
        }
    }

    return CallWindowProc(g_originalEditProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK OptionsWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        switch (wmId) {
        case IDC_SAVE_BTN:
            Settings::saveSettings();
            MessageBoxA(hwnd, "Settings saved successfully!", "Settings", MB_OK | MB_ICONINFORMATION);
            break;

        case IDC_DEFAULTS_BTN:
            Settings::hotkeys = HotkeySettings(); // Reset to defaults
            OptionsWindow::updateControls();
            break;

        case IDC_CLOSE_BTN:
            ShowWindow(hwnd, SW_HIDE);
            break;
        }
        break;
    }

    case WM_CLOSE:
        ShowWindow(hwnd, SW_HIDE);
        return 0;

    case WM_DESTROY:
        g_hOptionsWnd = NULL;
        break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void OptionsWindow::showOptionsDialog(HWND parent) {
    if (g_hOptionsWnd == NULL) {
        createOptionsWindow(parent);
    }

    if (g_hOptionsWnd) {
        ShowWindow(g_hOptionsWnd, SW_SHOW);
        SetForegroundWindow(g_hOptionsWnd);
        updateControls();
    }
}

void OptionsWindow::createOptionsWindow(HWND parent) {
    // Register window class for options window
    static bool classRegistered = false;
    if (!classRegistered) {
        WNDCLASSA wc = { 0 };
        wc.lpfnWndProc = OptionsWindowProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "StrongholdOptionsWindow";
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        RegisterClassA(&wc);
        classRegistered = true;
    }

    // Create the options window
    g_hOptionsWnd = CreateWindowExA(
        0,
        "StrongholdOptionsWindow",
        "Stronghold Calculator - Hotkey Settings",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 480, 380,
        NULL, // No parent - independent window
        NULL,
        GetModuleHandle(NULL),
        NULL
    );

    if (!g_hOptionsWnd) return;

    // Create controls
    int yPos = 20;
    int labelWidth = 200;
    int editWidth = 120;
    int spacing = 35;

    // Title
    CreateWindowA("STATIC", "Click on any field below and press a key to assign it:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, yPos, 400, 20, g_hOptionsWnd, NULL, GetModuleHandle(NULL), NULL);
    yPos += 30;

    // Calibrate hotkey
    CreateWindowA("STATIC", "Calibrate (90° turn):",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, yPos, labelWidth, 20, g_hOptionsWnd, NULL, GetModuleHandle(NULL), NULL);
    HWND hCalibrateEdit = CreateWindowA("EDIT", "",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
        230, yPos - 2, editWidth, 24, g_hOptionsWnd, (HMENU)IDC_CALIBRATE_EDIT, GetModuleHandle(NULL), NULL);
    yPos += spacing;

    // Track angle hotkey
    CreateWindowA("STATIC", "Track angle:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, yPos, labelWidth, 20, g_hOptionsWnd, NULL, GetModuleHandle(NULL), NULL);
    HWND hTrackEdit = CreateWindowA("EDIT", "",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
        230, yPos - 2, editWidth, 24, g_hOptionsWnd, (HMENU)IDC_TRACK_EDIT, GetModuleHandle(NULL), NULL);
    yPos += spacing;

    // Calculate hotkey
    CreateWindowA("STATIC", "Calculate & read coords:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, yPos, labelWidth, 20, g_hOptionsWnd, NULL, GetModuleHandle(NULL), NULL);
    HWND hCalculateEdit = CreateWindowA("EDIT", "",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
        230, yPos - 2, editWidth, 24, g_hOptionsWnd, (HMENU)IDC_CALCULATE_EDIT, GetModuleHandle(NULL), NULL);
    yPos += spacing;

    // Second throw hotkey
    CreateWindowA("STATIC", "Second throw:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, yPos, labelWidth, 20, g_hOptionsWnd, NULL, GetModuleHandle(NULL), NULL);
    HWND hSecondEdit = CreateWindowA("EDIT", "",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
        230, yPos - 2, editWidth, 24, g_hOptionsWnd, (HMENU)IDC_SECOND_THROW_EDIT, GetModuleHandle(NULL), NULL);
    yPos += spacing;

    // Reset hotkey
    CreateWindowA("STATIC", "Reset:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, yPos, labelWidth, 20, g_hOptionsWnd, NULL, GetModuleHandle(NULL), NULL);
    HWND hResetEdit = CreateWindowA("EDIT", "",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
        230, yPos - 2, editWidth, 24, g_hOptionsWnd, (HMENU)IDC_RESET_EDIT, GetModuleHandle(NULL), NULL);
    yPos += spacing;

    // Options hotkey
    CreateWindowA("STATIC", "Options:",
        WS_VISIBLE | WS_CHILD | SS_LEFT,
        20, yPos, labelWidth, 20, g_hOptionsWnd, NULL, GetModuleHandle(NULL), NULL);
    HWND hOptionsEdit = CreateWindowA("EDIT", "",
        WS_VISIBLE | WS_CHILD | WS_BORDER | ES_READONLY | ES_CENTER,
        230, yPos - 2, editWidth, 24, g_hOptionsWnd, (HMENU)IDC_OPTIONS_EDIT, GetModuleHandle(NULL), NULL);
    yPos += 50;

    // Buttons
    CreateWindowA("BUTTON", "Save Settings",
        WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        60, yPos, 100, 30, g_hOptionsWnd, (HMENU)IDC_SAVE_BTN, GetModuleHandle(NULL), NULL);
    CreateWindowA("BUTTON", "Defaults",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        180, yPos, 100, 30, g_hOptionsWnd, (HMENU)IDC_DEFAULTS_BTN, GetModuleHandle(NULL), NULL);
    CreateWindowA("BUTTON", "Close",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        300, yPos, 100, 30, g_hOptionsWnd, (HMENU)IDC_CLOSE_BTN, GetModuleHandle(NULL), NULL);

    // Subclass all edit controls to capture key presses
    g_originalEditProc = (WNDPROC)SetWindowLongPtr(hCalibrateEdit, GWLP_WNDPROC, (LONG_PTR)EditKeyCapture);
    SetWindowLongPtr(hTrackEdit, GWLP_WNDPROC, (LONG_PTR)EditKeyCapture);
    SetWindowLongPtr(hCalculateEdit, GWLP_WNDPROC, (LONG_PTR)EditKeyCapture);
    SetWindowLongPtr(hSecondEdit, GWLP_WNDPROC, (LONG_PTR)EditKeyCapture);
    SetWindowLongPtr(hResetEdit, GWLP_WNDPROC, (LONG_PTR)EditKeyCapture);
    SetWindowLongPtr(hOptionsEdit, GWLP_WNDPROC, (LONG_PTR)EditKeyCapture);
}

void OptionsWindow::updateControls() {
    if (!g_hOptionsWnd) return;

    SetWindowTextA(GetDlgItem(g_hOptionsWnd, IDC_CALIBRATE_EDIT),
        Settings::virtualKeyToString(Settings::hotkeys.calibrateKey).c_str());
    SetWindowTextA(GetDlgItem(g_hOptionsWnd, IDC_TRACK_EDIT),
        Settings::virtualKeyToString(Settings::hotkeys.trackAngleKey).c_str());
    SetWindowTextA(GetDlgItem(g_hOptionsWnd, IDC_CALCULATE_EDIT),
        Settings::virtualKeyToString(Settings::hotkeys.calculateKey).c_str());
    SetWindowTextA(GetDlgItem(g_hOptionsWnd, IDC_SECOND_THROW_EDIT),
        Settings::virtualKeyToString(Settings::hotkeys.secondThrowKey).c_str());
    SetWindowTextA(GetDlgItem(g_hOptionsWnd, IDC_RESET_EDIT),
        Settings::virtualKeyToString(Settings::hotkeys.resetKey).c_str());
    SetWindowTextA(GetDlgItem(g_hOptionsWnd, IDC_OPTIONS_EDIT),
        Settings::virtualKeyToString(Settings::hotkeys.optionsKey).c_str());
}

// Legacy functions - now empty since we removed dropdown logic
INT_PTR CALLBACK OptionsWindow::DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    return FALSE;
}

void OptionsWindow::populateControls(HWND hDlg) {
    // Not used anymore
}

void OptionsWindow::saveFromControls(HWND hDlg) {
    // Not used anymore - settings are updated immediately when keys are pressed
}

string OptionsWindow::getKeyName(int vk) {
    return Settings::virtualKeyToString(vk);
}