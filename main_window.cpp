#define NOMINMAX
#include "main_window.h"
#include "coordinate_reader.h"
#include "stronghold_calculator.h"
#include "distance_calculator.h"
#include "overlay_window.h"
#include <fstream>
#include <shlobj.h>

// Global variables for hotkey customization
int currentTabHotkey = VK_TAB;
int currentF4Hotkey = VK_F4;
bool waitingForTabHotkey = false;
bool waitingForF4Hotkey = false;

// Configuration file functions
std::wstring GetConfigFilePath() {
    wchar_t* appDataPath;
    if (SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, NULL, &appDataPath) == S_OK) {
        std::wstring configPath = std::wstring(appDataPath) + L"\\MinecraftStrongholdFinder";

        // Create directory if it doesn't exist
        CreateDirectory(configPath.c_str(), NULL);

        configPath += L"\\config.ini";
        CoTaskMemFree(appDataPath);
        return configPath;
    }

    // Fallback to current directory if AppData is not available
    return L"config.ini";
}

void SaveHotkeysToFile() {
    std::wstring configPath = GetConfigFilePath();
    std::wofstream file(configPath);

    if (file.is_open()) {
        file << L"[Hotkeys]\n";
        file << L"DirectionKey=" << currentTabHotkey << L"\n";
        file << L"DistanceKey=" << currentF4Hotkey << L"\n";
        file.close();
    }
}

void LoadHotkeysFromFile() {
    std::wstring configPath = GetConfigFilePath();
    std::wifstream file(configPath);

    if (file.is_open()) {
        std::wstring line;
        while (std::getline(file, line)) {
            // Skip empty lines and comments
            if (line.empty() || line[0] == L'#' || line[0] == L';' || line[0] == L'[') {
                continue;
            }

            // Find the '=' separator
            size_t equalPos = line.find(L'=');
            if (equalPos != std::wstring::npos) {
                std::wstring key = line.substr(0, equalPos);
                std::wstring value = line.substr(equalPos + 1);

                // Remove whitespace
                key.erase(0, key.find_first_not_of(L" \t"));
                key.erase(key.find_last_not_of(L" \t") + 1);
                value.erase(0, value.find_first_not_of(L" \t"));
                value.erase(value.find_last_not_of(L" \t") + 1);

                try {
                    int keyCode = std::stoi(value);

                    if (key == L"DirectionKey") {
                        currentTabHotkey = keyCode;
                    }
                    else if (key == L"DistanceKey") {
                        currentF4Hotkey = keyCode;
                    }
                }
                catch (const std::exception&) {
                    // Invalid number in config, keep default value
                }
            }
        }
        file.close();
    }
}

void CopyStrongholdResultsToClipboard() {
    std::wstringstream ss;
    if (!strongholdCandidates.empty()) {
        ss << L"Stronghold Locations:\n";
        int maxForClipboard = std::min(5, (int)strongholdCandidates.size());
        for (int i = 0; i < maxForClipboard; i++) {
            const auto& candidate = strongholdCandidates[i];
            ss << L"#" << (i + 1) << L": Overworld (" << candidate.projectionX
                << L", " << candidate.projectionZ << L") - Nether ("
                << candidate.netherX << L", " << candidate.netherZ << L") - "
                << std::fixed << std::setprecision(1)
                << (candidate.conditionalProb * 100.0) << L"%\n";
        }

        // Add warning to clipboard if distance validation failed
        if (appState.distanceValidationFailed) {
            ss << L"\nWARNING: Distance mismatch detected!";
        }
    }
    else {
        ss << L"Angle: " << appState.lastAngle << L"° - No strongholds found";
    }
    std::wstring text = ss.str();

    OpenClipboard(NULL);
    EmptyClipboard();
    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, (text.size() + 1) * sizeof(wchar_t));
    if (hGlob) {
        memcpy(GlobalLock(hGlob), text.c_str(), (text.size() + 1) * sizeof(wchar_t));
        GlobalUnlock(hGlob);
        SetClipboardData(CF_UNICODETEXT, hGlob);
    }
    CloseClipboard();
}

std::wstring GetKeyName(int vkCode) {
    switch (vkCode) {
    case VK_TAB: return L"TAB";
    case VK_F1: return L"F1";
    case VK_F2: return L"F2";
    case VK_F3: return L"F3";
    case VK_F4: return L"F4";
    case VK_F5: return L"F5";
    case VK_F6: return L"F6";
    case VK_F7: return L"F7";
    case VK_F8: return L"F8";
    case VK_F9: return L"F9";
    case VK_F10: return L"F10";
    case VK_F11: return L"F11";
    case VK_F12: return L"F12";
    case VK_SPACE: return L"SPACE";
    case VK_RETURN: return L"ENTER";
    case VK_ESCAPE: return L"ESC";
    case VK_INSERT: return L"INSERT";
    case VK_DELETE: return L"DELETE";
    case VK_HOME: return L"HOME";
    case VK_END: return L"END";
    case VK_PRIOR: return L"PAGE UP";
    case VK_NEXT: return L"PAGE DOWN";
    case VK_UP: return L"UP ARROW";
    case VK_DOWN: return L"DOWN ARROW";
    case VK_LEFT: return L"LEFT ARROW";
    case VK_RIGHT: return L"RIGHT ARROW";
    case VK_OEM_3: return L"` (TILDE)";
    default:
        if (vkCode >= 'A' && vkCode <= 'Z') {
            return std::wstring(1, (wchar_t)vkCode);
        }
        if (vkCode >= '0' && vkCode <= '9') {
            return std::wstring(1, (wchar_t)vkCode);
        }
        return L"KEY_" + std::to_wstring(vkCode);
    }
}

void RegisterHotkeys(HWND hWnd) {
    // Unregister existing hotkeys first
    UnregisterHotKey(hWnd, 1);
    UnregisterHotKey(hWnd, 2);

    // Register new hotkeys
    RegisterHotKey(hWnd, 1, 0, currentTabHotkey);
    RegisterHotKey(hWnd, 2, 0, currentF4Hotkey);
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_KEYDOWN:
        if (waitingForTabHotkey) {
            int newKey = (int)wParam;
            if (newKey != VK_ESCAPE && newKey != currentF4Hotkey) {
                currentTabHotkey = newKey;
                RegisterHotkeys(hWnd);
                SaveHotkeysToFile(); // Save to file when changed
            }
            waitingForTabHotkey = false;
            SetFocus(hWnd); // Remove focus from any button
            InvalidateRect(hWnd, NULL, TRUE);
            return 0;
        }
        else if (waitingForF4Hotkey) {
            int newKey = (int)wParam;
            if (newKey != VK_ESCAPE && newKey != currentTabHotkey) {
                currentF4Hotkey = newKey;
                RegisterHotkeys(hWnd);
                SaveHotkeysToFile(); // Save to file when changed
            }
            waitingForF4Hotkey = false;
            SetFocus(hWnd); // Remove focus from any button
            InvalidateRect(hWnd, NULL, TRUE);
            return 0;
        }
        break;

    case WM_COMMAND:
        if (LOWORD(wParam) == 1001) { // Change Tab hotkey button
            waitingForTabHotkey = true;
            waitingForF4Hotkey = false;
            SetFocus(hWnd); // Set focus to main window to capture keys
            InvalidateRect(hWnd, NULL, TRUE);
        }
        else if (LOWORD(wParam) == 1002) { // Change F4 hotkey button
            waitingForF4Hotkey = true;
            waitingForTabHotkey = false;
            SetFocus(hWnd); // Set focus to main window to capture keys
            InvalidateRect(hWnd, NULL, TRUE);
        }
        break;

    case WM_HOTKEY:
    {
        if (wParam == 1) { // Direction hotkey (formerly Tab)
            HWND mcHwnd = FindWindow(NULL, L"Minecraft");
            if (mcHwnd && GetShownCoordinates(mcHwnd, &appState.latestCoords)) {
                if (appState.capturePhase == 0) {
                    // First press - only mark as direction first if distance wasn't pressed yet
                    if (appState.distanceKeyPresses == 0) {
                        appState.tabPressedFirst = true;
                        ShowOverlay(); // Show overlay when direction key is first pressed
                    }
                    appState.coord1 = appState.latestCoords;
                    appState.capturePhase = 1;
                }
                else if (appState.capturePhase == 1) {
                    appState.coord2 = appState.latestCoords;
                    appState.lastAngle = angleBetween(appState.coord1.x, appState.coord1.z,
                        appState.coord2.x, appState.coord2.z);
                    appState.capturePhase = 2;

                    // Always use distance when distance key was pressed first
                    double targetDistance = -1;
                    if (appState.f4PressedFirst && appState.calculatedDistance > 0) {
                        targetDistance = appState.calculatedDistance;
                    }
                    calculateStrongholdLocationWithDistance(appState.coord1.x, appState.coord1.z,
                        appState.lastAngle, targetDistance);

                    // Copy results to clipboard
                    CopyStrongholdResultsToClipboard();
                }
                else {
                    // Third press resets everything
                    appState.capturePhase = 0;
                    appState.coord1 = { 0, 0, 0 };
                    appState.coord2 = { 0, 0, 0 };
                    appState.lastAngle = 0.0;
                    appState.distanceKeyPresses = 0;
                    appState.calculatedDistance = 0.0;
                    strongholdCandidates.clear();
                    appState.tabPressedFirst = false;
                    appState.f4PressedFirst = false;
                    appState.distanceValidationFailed = false;
                    appState.validationErrorMessage = L"";
                    HideOverlay(); // Hide overlay when resetting
                }

                UpdateOverlay();
                InvalidateRect(hWnd, NULL, TRUE);
            }
        }
        else if (wParam == 2) { // Distance hotkey (formerly F4)
            handleDistanceKey();
        }
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);
        graphics.Clear(Color(30, 30, 30));

        FontFamily fontFamily(L"Segoe UI");
        Font font(&fontFamily, 12, FontStyleRegular, UnitPixel);
        Font smallFont(&fontFamily, 10, FontStyleRegular, UnitPixel);
        Font tableFont(&fontFamily, 11, FontStyleRegular, UnitPixel);
        Font buttonFont(&fontFamily, 9, FontStyleRegular, UnitPixel);
        SolidBrush whiteBrush(Color(240, 240, 240));
        SolidBrush grayBrush(Color(180, 180, 180));
        SolidBrush greenBrush(Color(100, 200, 100));
        SolidBrush yellowBrush(Color(255, 220, 100));
        SolidBrush redBrush(Color(255, 100, 100));
        SolidBrush cyanBrush(Color(100, 200, 255));

        int marginX = 15;
        int y = 15;

        Font headerFont(&fontFamily, 14, FontStyleBold, UnitPixel);
        graphics.DrawString(L"Minecraft Stronghold Finder", -1, &headerFont,
            PointF((REAL)marginX, (REAL)y), &whiteBrush);
        y += 35;

        // Hotkey configuration section
        graphics.DrawString(L"Hotkey Settings:", -1, &font,
            PointF((REAL)marginX, (REAL)y), &whiteBrush);
        y += 20;

        // Direction hotkey
        std::wstringstream dirSS;
        dirSS << L"Direction Key: " << GetKeyName(currentTabHotkey);
        if (waitingForTabHotkey) {
            dirSS << L" (Press new key...)";
        }
        SolidBrush* dirBrush = waitingForTabHotkey ? &yellowBrush : &grayBrush;
        graphics.DrawString(dirSS.str().c_str(), -1, &font,
            PointF((REAL)marginX, (REAL)y), dirBrush);
        y += 20;

        // Distance hotkey
        std::wstringstream distSS;
        distSS << L"Distance Key: " << GetKeyName(currentF4Hotkey);
        if (waitingForF4Hotkey) {
            distSS << L" (Press new key...)";
        }
        SolidBrush* distKeyBrush = waitingForF4Hotkey ? &yellowBrush : &grayBrush;
        graphics.DrawString(distSS.str().c_str(), -1, &font,
            PointF((REAL)marginX, (REAL)y), distKeyBrush);
        y += 30;

        std::wstringstream ss;
        ss << L"Current Position:\n"
            << L"X: " << appState.latestCoords.x << L"\n"
            << L"Y: " << appState.latestCoords.y << L"\n"
            << L"Z: " << appState.latestCoords.z << L"\n";

        graphics.DrawString(ss.str().c_str(), -1, &font,
            PointF((REAL)marginX, (REAL)y), &grayBrush);
        y += 85;

        // Distance info
        if (appState.f4PressedFirst && appState.distanceKeyPresses > 0) {
            std::wstringstream distInfoSS;
            distInfoSS << GetKeyName(currentF4Hotkey) << L" Distance Calculation:\n"
                << L"Key presses: " << appState.distanceKeyPresses << L"\n"
                << L"Calculated distance: " << std::fixed << std::setprecision(0)
                << appState.calculatedDistance << L" blocks\n";
            graphics.DrawString(distInfoSS.str().c_str(), -1, &font,
                PointF((REAL)marginX, (REAL)y), &yellowBrush);
            y += 55;
        }
        else if (appState.tabPressedFirst) {
            std::wstringstream skipSS;
            skipSS << GetKeyName(currentF4Hotkey) << L" key disabled (" << GetKeyName(currentTabHotkey) << L" was pressed first)\n";
            graphics.DrawString(skipSS.str().c_str(), -1, &font,
                PointF((REAL)marginX, (REAL)y), &grayBrush);
            y += 20;
        }

        // Show validation error in main window
        if (appState.distanceValidationFailed && !appState.validationErrorMessage.empty()) {
            Font errorFont(&fontFamily, 10, FontStyleBold, UnitPixel);
            // Word wrap the error message
            RectF layoutRect((REAL)marginX, (REAL)y, 450.0f, 60.0f);
            graphics.DrawString(appState.validationErrorMessage.c_str(), -1, &errorFont,
                layoutRect, nullptr, &redBrush);
            y += 65;
        }

        if (appState.capturePhase == 0) {
            if (!appState.f4PressedFirst && !appState.tabPressedFirst) {
                std::wstringstream instrSS;
                instrSS << L"Instructions:\n"
                    << L"1. Press " << GetKeyName(currentF4Hotkey) << L" multiple times for distance calculation\n"
                    << L"2. Press " << GetKeyName(currentTabHotkey) << L" twice for direction calculation\n"
                    << L"   (or press " << GetKeyName(currentTabHotkey) << L" first to skip distance)";
                graphics.DrawString(instrSS.str().c_str(), -1, &font,
                    PointF((REAL)marginX, (REAL)y), &whiteBrush);
            }
            else if (appState.tabPressedFirst) {
                std::wstringstream skipInstrSS;
                skipInstrSS << GetKeyName(currentTabHotkey) << L" pressed first - distance calculation skipped.\n"
                    << L"Press " << GetKeyName(currentTabHotkey) << L" again for direction calculation.";
                graphics.DrawString(skipInstrSS.str().c_str(), -1, &font,
                    PointF((REAL)marginX, (REAL)y), &whiteBrush);
            }
            else if (appState.f4PressedFirst) {
                std::wstringstream distInstrSS;
                distInstrSS << L"Distance calculated! Now press " << GetKeyName(currentTabHotkey) << L" twice for direction.";
                graphics.DrawString(distInstrSS.str().c_str(), -1, &font,
                    PointF((REAL)marginX, (REAL)y), &greenBrush);
            }
        }
        else if (appState.capturePhase == 1) {
            std::wstringstream firstSS;
            firstSS << L"First point captured: (" << appState.coord1.x << L", " << appState.coord1.z << L")\n"
                << L"Press " << GetKeyName(currentTabHotkey) << L" at second point to calculate direction.";
            graphics.DrawString(firstSS.str().c_str(), -1, &font,
                PointF((REAL)marginX, (REAL)y), &yellowBrush);
        }
        else if (appState.capturePhase == 2) {
            std::wstringstream angleSS;
            angleSS << L"Eye Direction: " << std::fixed << std::setprecision(1)
                << appState.lastAngle << L"°\n";

            if (!strongholdCandidates.empty()) {
                angleSS << L"\nStronghold Locations (Overworld / Nether):";
            }
            else {
                angleSS << L"\nNo strongholds found in this direction.";
            }

            SolidBrush* angleBrush = appState.distanceValidationFailed ? &redBrush : &greenBrush;
            graphics.DrawString(angleSS.str().c_str(), -1, &font,
                PointF((REAL)marginX, (REAL)y), angleBrush);

            if (!strongholdCandidates.empty()) {
                y += 55;

                // Table header
                graphics.DrawString(L"Rank  Overworld Coords    Nether Coords      Probability  Distance", -1, &smallFont,
                    PointF((REAL)marginX, (REAL)y), &grayBrush);
                y += 20;

                // Show detailed candidate list
                int maxCandidates = std::min(10, (int)strongholdCandidates.size());
                for (int i = 0; i < maxCandidates; i++) {
                    const auto& candidate = strongholdCandidates[i];

                    std::wstringstream candidateSS;
                    candidateSS << L"#" << (i + 1) << L"   ("
                        << std::setw(5) << candidate.projectionX << L"," << std::setw(5) << candidate.projectionZ << L")   ("
                        << std::setw(4) << candidate.netherX << L"," << std::setw(4) << candidate.netherZ << L")      "
                        << std::fixed << std::setprecision(1) << std::setw(5)
                        << (candidate.conditionalProb * 100.0) << L"%     "
                        << candidate.distance << L"m";

                    SolidBrush* brush;
                    if (i == 0) {
                        brush = appState.distanceValidationFailed ? &redBrush : &greenBrush;
                    }
                    else if (candidate.conditionalProb > 0.1) {
                        brush = &yellowBrush;
                    }
                    else {
                        brush = &grayBrush;
                    }

                    graphics.DrawString(candidateSS.str().c_str(), -1, &tableFont,
                        PointF((REAL)marginX, (REAL)y), brush);
                    y += 18;
                }

                y += 10;
                std::wstringstream resetSS;
                resetSS << L"Top locations copied to clipboard. Press " << GetKeyName(currentTabHotkey) << L" to reset.";
                graphics.DrawString(resetSS.str().c_str(), -1, &font,
                    PointF((REAL)marginX, (REAL)y), &whiteBrush);
            }
        }

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_CREATE:
    {
        // Load hotkeys from file at startup
        LoadHotkeysFromFile();

        // Create hotkey change buttons - positioned after hotkey text
        CreateWindow(L"BUTTON", L"Change Direction Key", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            300, 55, 140, 25, hWnd, (HMENU)1001, GetModuleHandle(NULL), NULL);

        CreateWindow(L"BUTTON", L"Change Distance Key", WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
            300, 75, 140, 25, hWnd, (HMENU)1002, GetModuleHandle(NULL), NULL);

        // Register hotkeys (now using loaded values)
        RegisterHotkeys(hWnd);
    }
    break;

    case WM_DESTROY:
        UnregisterHotKey(hWnd, 1);
        UnregisterHotKey(hWnd, 2);
        if (hOverlayWnd) {
            DestroyWindow(hOverlayWnd);
        }
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
