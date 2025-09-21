#define NOMINMAX
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <sstream>
#include <iomanip>
#include <cmath>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using namespace Gdiplus;
using namespace std;

// Constants
const int WINDOW_WIDTH = 400;
const int WINDOW_HEIGHT = 550;
const double M_PI = 3.14159265358979323846;

// Structures
struct Vec3 {
    int x, y, z;
};

struct StrongholdCell {
    double centerX, centerZ;
    double xMin, xMax, zMin, zMax;
    double prob;
    double distance;
    int distanceRange;
};

struct StrongholdCandidate {
    int projectionX, projectionZ;
    int netherX, netherZ;
    double cellCenterX, cellCenterZ;
    double distance;
    int distanceFromOrigin;
    int distanceRange;
};

enum AppMode {
    MODE_READY,
    MODE_CALIBRATING,
    MODE_TRACKING_ANGLE,
    MODE_ANGLE_CAPTURED,
    MODE_RESULTS
};

// Application State
struct AppState {
    AppMode mode = MODE_READY;
    bool mouseSensitivityCalibrated = false;
    double mouseSensitivityUnitsPerDegree = 0.0;
    double capturedAngle = 0.0;
    Vec3 currentCoordinates = { 0, 0, 0 };
    vector<StrongholdCandidate> strongholdCandidates;
    HWND minecraftWindow = nullptr;
    string status = "Press F6 to calibrate mouse sensitivity (90° turn)";
    bool showResults = false;
} appState;

// Distance probabilities
map<int, double> distanceProbabilities = {
    {500, 0.0262}, {600, 0.0639}, {800, 0.1705}, {900, 0.1582}, {1000, 0.1427},
    {1100, 0.1204}, {1200, 0.0919}, {1300, 0.1133}, {1400, 0.1139}, {1500, 0.1228},
    {1700, 0.0586}, {1800, 0.0535}, {1900, 0.0610}, {2100, 0.0590}, {2200, 0.0431},
    {2300, 0.0375}, {2400, 0.0292}, {2500, 0.0493}, {2600, 0.0382}, {2700, 0.0347},
    {2800, 0.0258}, {3000, 0.0171}, {3100, 0.0169}, {3200, 0.0189}
};

vector<StrongholdCell> strongholdCells;

// Mouse tracker class
class MouseAngleTracker {
private:
    HWND hwnd;
    bool tracking = false;
    double totalMouseMovement = 0.0;
    bool rawInputRegistered = false;
    double sensitivityUnitsPerDegree = 0.0;

public:
    MouseAngleTracker(HWND window) : hwnd(window) {
        registerRawInput();
    }

    void registerRawInput() {
        RAWINPUTDEVICE rid;
        rid.usUsagePage = 0x01;
        rid.usUsage = 0x02;
        rid.dwFlags = RIDEV_INPUTSINK;
        rid.hwndTarget = hwnd;

        if (RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
            rawInputRegistered = true;
        }
    }

    void processRawInput(LPARAM lParam) {
        if (!tracking || !rawInputRegistered) return;

        UINT dwSize = 0;
        GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));

        LPBYTE lpb = new BYTE[dwSize];
        if (lpb == NULL) return;

        if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) {
            delete[] lpb;
            return;
        }

        RAWINPUT* raw = (RAWINPUT*)lpb;

        if (raw->header.dwType == RIM_TYPEMOUSE && raw->data.mouse.usFlags == MOUSE_MOVE_RELATIVE) {
            int deltaX = raw->data.mouse.lLastX;
            totalMouseMovement += deltaX;
        }

        delete[] lpb;
    }

    void startTracking() {
        totalMouseMovement = 0.0;
        tracking = true;
    }

    double stopTrackingAndGetDistance() {
        tracking = false;
        return totalMouseMovement;
    }

    double stopTrackingAndGetAngle() {
        tracking = false;
        if (sensitivityUnitsPerDegree <= 0) return 0.0;
        return totalMouseMovement / sensitivityUnitsPerDegree;
    }

    void setSensitivity(double unitsPerDegree) {
        sensitivityUnitsPerDegree = unitsPerDegree;
    }

    bool isTracking() const { return tracking; }
    bool isCalibrated() const { return sensitivityUnitsPerDegree > 0; }
    double getCurrentDistance() const { return totalMouseMovement; }
    double getCurrentAngle() const {
        if (sensitivityUnitsPerDegree <= 0) return 0.0;
        return totalMouseMovement / sensitivityUnitsPerDegree;
    }
};

MouseAngleTracker* mouseTracker = nullptr;

// Coordinate reading functions
unique_ptr<Bitmap> BitmapFromHWND(HWND hwnd) {
    if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
    RECT rc;
    GetWindowRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    HDC hdc = GetDC(hwnd);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);
    PrintWindow(hwnd, memDC, PW_RENDERFULLCONTENT);
    auto pBitmap = make_unique<Bitmap>(hBitmap, nullptr);
    SelectObject(memDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(hwnd, hdc);
    return pBitmap;
}

int GetShownCoordinates(HWND hwnd, Vec3* coordinates) {
    auto pBitmap = BitmapFromHWND(hwnd);
    int width = pBitmap->GetWidth();
    int height = pBitmap->GetHeight();
    int searchWidth = max(width / 3, min(125, width));
    int searchHeight = height / 3;

    BitmapData bitmapData;
    Rect rect(0, 0, searchWidth, searchHeight);
    pBitmap->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);

    int startTextX = 0, startTextY = 0, streak = 0;
    int stride = bitmapData.Stride / sizeof(ARGB);
    ARGB* pixels = static_cast<ARGB*>(bitmapData.Scan0);

    for (int y = 30; y < searchHeight; y++) {
        for (int x = 8; x < searchWidth; x++) {
            if (pixels[y * stride + x] == 0xFFFFFFFF) {
                if (!startTextX) { startTextX = x; startTextY = y; }
                streak++;
            }
            else if (streak < 4) streak = 0;
            else if (streak >= 4) break;
        }
        if (streak >= 4) break;
    }

    if (streak < 4) return 0;

    int scale = streak / 4;
    startTextX += 44 * scale;
    int coords[3] = { 0, 0, 0 };
    int index = 0;
    bool isSigned = false;

    while (startTextX < searchWidth) {
        unsigned int columnMask = 0;
        for (int dy = 0; dy < 7; dy++) {
            columnMask <<= 1;
            if (pixels[(startTextY + dy * scale) * stride + startTextX] == 0xFFFFFFFF)
                columnMask |= 1;
        }

        int digit = -1;
        switch (columnMask) {
        case 0b0111110: digit = 0; break;
        case 0b0000001: digit = 1; break;
        case 0b0100011: digit = 2; break;
        case 0b0100010: digit = 3; break;
        case 0b0001100: digit = 4; break;
        case 0b1110010: digit = 5; break;
        case 0b0011110: digit = 6; break;
        case 0b1100000: digit = 7; break;
        case 0b0110110: digit = 8; break;
        case 0b0110000: digit = 9; break;
        case 0b0001000: isSigned = true; break;
        case 0b0000011:
            if (isSigned) coords[index] *= -1;
            if (++index > 2) break;
            isSigned = false;
            break;
        default:
            if (index >= 2) break;
            if (isSigned) coords[index] *= -1;
            break;
        }

        if (digit != -1)
            coords[index] = coords[index] * 10 + digit;
        startTextX += 6 * scale;
    }

    if (isSigned && index <= 2) {
        coords[index] *= -1;
    }

    pBitmap->UnlockBits(&bitmapData);
    coordinates->x = coords[0];
    coordinates->y = coords[1];
    coordinates->z = coords[2];
    return 1;
}

void generateStrongholdCells() {
    strongholdCells.clear();
    int cellSize = 272;
    int gap = 160;
    int totalStep = cellSize + gap;

    for (int xIndex = -15; xIndex <= 15; xIndex++) {
        for (int zIndex = -15; zIndex <= 15; zIndex++) {
            double xMin, xMax, zMin, zMax;

            if (xIndex >= 0) {
                xMin = xIndex * totalStep;
                xMax = xMin + cellSize;
            }
            else {
                xMax = xIndex * totalStep - gap;
                xMin = xMax - cellSize;
            }

            if (zIndex >= 0) {
                zMin = zIndex * totalStep;
                zMax = zMin + cellSize;
            }
            else {
                zMax = zIndex * totalStep - gap;
                zMin = zMax - cellSize;
            }

            double centerX = (xMin + xMax) / 2.0;
            double centerZ = (zMin + zMax) / 2.0;
            double distanceFromOrigin = sqrt(centerX * centerX + centerZ * centerZ);

            if (distanceFromOrigin >= 512) {
                int closestDistance = 500;
                double minDiff = abs(500 - (int)distanceFromOrigin);

                for (auto& pair : distanceProbabilities) {
                    double diff = abs(pair.first - (int)distanceFromOrigin);
                    if (diff < minDiff) {
                        minDiff = diff;
                        closestDistance = pair.first;
                    }
                }

                StrongholdCell cell;
                cell.centerX = centerX;
                cell.centerZ = centerZ;
                cell.xMin = xMin;
                cell.xMax = xMax;
                cell.zMin = zMin;
                cell.zMax = zMax;
                cell.prob = distanceProbabilities[closestDistance];
                cell.distance = distanceFromOrigin;
                cell.distanceRange = closestDistance;

                strongholdCells.push_back(cell);
            }
        }
    }
}

void calculateStrongholdFromAngle(double playerX, double playerZ, double angle) {
    appState.strongholdCandidates.clear();

    double eyeStartX = playerX + 0.5;
    double eyeStartZ = playerZ + 0.5;

    double angleRad = angle * M_PI / 180.0;
    double dx = sin(angleRad);
    double dz = -cos(angleRad);

    vector<StrongholdCandidate> candidates;

    // Project the line and find intersections with stronghold cells
    for (const auto& cell : strongholdCells) {
        // Check if the line intersects this cell
        double toCenterX = cell.centerX - eyeStartX;
        double toCenterZ = cell.centerZ - eyeStartZ;
        double t = (toCenterX * dx + toCenterZ * dz);

        if (t > 0) { // Only forward projections
            double projectionX = eyeStartX + t * dx;
            double projectionZ = eyeStartZ + t * dz;

            // Check if projection is within cell bounds
            if (projectionX >= cell.xMin && projectionX <= cell.xMax &&
                projectionZ >= cell.zMin && projectionZ <= cell.zMax) {

                StrongholdCandidate candidate;
                candidate.projectionX = (int)round(projectionX);
                candidate.projectionZ = (int)round(projectionZ);
                candidate.netherX = (int)round(projectionX / 8.0);
                candidate.netherZ = (int)round(projectionZ / 8.0);
                candidate.cellCenterX = cell.centerX;
                candidate.cellCenterZ = cell.centerZ;
                candidate.distance = (int)round(sqrt(pow(projectionX - playerX, 2) + pow(projectionZ - playerZ, 2)));
                candidate.distanceFromOrigin = (int)round(cell.distance);
                candidate.distanceRange = cell.distanceRange;

                candidates.push_back(candidate);
            }
        }
    }

    if (!candidates.empty()) {
        // Sort by distance and take closest reasonable candidates
        sort(candidates.begin(), candidates.end(), [](const StrongholdCandidate& a, const StrongholdCandidate& b) {
            return a.distance < b.distance;
            });

        // Take up to 3 closest candidates
        for (size_t i = 0; i < min((size_t)3, candidates.size()); i++) {
            appState.strongholdCandidates.push_back(candidates[i]);
        }

        appState.status = "Stronghold locations calculated from angle projection";
        appState.showResults = true;
        appState.mode = MODE_RESULTS;
    }
    else {
        // Fallback: project to a reasonable distance and give approximate coordinates
        double fallbackDistance = 1500; // Default assumption
        double fallbackX = eyeStartX + dx * fallbackDistance;
        double fallbackZ = eyeStartZ + dz * fallbackDistance;

        StrongholdCandidate fallback;
        fallback.projectionX = (int)round(fallbackX);
        fallback.projectionZ = (int)round(fallbackZ);
        fallback.netherX = (int)round(fallbackX / 8.0);
        fallback.netherZ = (int)round(fallbackZ / 8.0);
        fallback.distance = (int)fallbackDistance;
        fallback.distanceFromOrigin = (int)round(sqrt(fallbackX * fallbackX + fallbackZ * fallbackZ));

        appState.strongholdCandidates.push_back(fallback);
        appState.status = "Approximate location (no exact cell match)";
        appState.showResults = true;
        appState.mode = MODE_RESULTS;
    }
}

void findMinecraftWindow() {
    appState.minecraftWindow = FindWindowA("LWJGL", nullptr);
    if (!appState.minecraftWindow) {
        appState.minecraftWindow = FindWindowA(nullptr, "Minecraft");
    }
}

void startCalibration() {
    if (mouseTracker) {
        mouseTracker->startTracking();
        appState.mode = MODE_CALIBRATING;
        appState.status = "CALIBRATING: Turn exactly 90°, then press F6 again";
    }
}

void finishCalibration() {
    if (mouseTracker && appState.mode == MODE_CALIBRATING) {
        double totalDistance = mouseTracker->stopTrackingAndGetDistance();
        if (totalDistance > 0) {
            appState.mouseSensitivityUnitsPerDegree = totalDistance / 90.0;
            mouseTracker->setSensitivity(appState.mouseSensitivityUnitsPerDegree);
            appState.mouseSensitivityCalibrated = true;
            appState.mode = MODE_READY;

            ostringstream ss;
            ss << "CALIBRATED: " << fixed << setprecision(3) << appState.mouseSensitivityUnitsPerDegree
                << " units/degree. Press F7 to track angle.";
            appState.status = ss.str();
        }
        else {
            appState.mode = MODE_READY;
            appState.status = "Calibration failed - no mouse movement detected";
        }
    }
}

void startAngleTracking() {
    if (mouseTracker && appState.mouseSensitivityCalibrated) {
        mouseTracker->startTracking();
        appState.mode = MODE_TRACKING_ANGLE;
        appState.status = "TRACKING: Turn to face stronghold, then press F7 again";
    }
}

void finishAngleTracking() {
    if (mouseTracker && appState.mode == MODE_TRACKING_ANGLE) {
        appState.capturedAngle = mouseTracker->stopTrackingAndGetAngle();
        appState.mode = MODE_ANGLE_CAPTURED;

        ostringstream ss;
        ss << "ANGLE CAPTURED: " << fixed << setprecision(1) << appState.capturedAngle
            << "° - Press F8 to read coordinates";
        appState.status = ss.str();
    }
}

void readCoordinatesAndCalculate() {
    findMinecraftWindow();
    if (!appState.minecraftWindow) {
        // Try anyway with last known coordinates
        if (appState.currentCoordinates.x == 0 && appState.currentCoordinates.z == 0) {
            appState.status = "Minecraft not found - using default position (0,0)";
            appState.currentCoordinates = { 0, 64, 0 };
        }
    }
    else {
        Vec3 coords;
        if (GetShownCoordinates(appState.minecraftWindow, &coords)) {
            appState.currentCoordinates = coords;
        }
        else {
            appState.status = "Could not read coordinates - using last known position";
        }
    }

    // Calculate stronghold location
    calculateStrongholdFromAngle(appState.currentCoordinates.x, appState.currentCoordinates.z, appState.capturedAngle);
}

void resetApp() {
    appState.strongholdCandidates.clear();
    appState.showResults = false;
    appState.mode = MODE_READY;
    appState.capturedAngle = 0.0;
    if (appState.mouseSensitivityCalibrated) {
        appState.status = "Ready - Press F7 to track angle";
    }
    else {
        appState.status = "Press F6 to calibrate mouse first";
    }
}

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    static bool f6Pressed = false, f7Pressed = false, f8Pressed = false, f9Pressed = false;

    switch (uMsg) {
    case WM_CREATE:
        SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        mouseTracker = new MouseAngleTracker(hwnd);
        generateStrongholdCells();
        SetTimer(hwnd, 1, 16, NULL);
        break;

    case WM_INPUT:
        if (mouseTracker) {
            mouseTracker->processRawInput(lParam);
        }
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    case WM_TIMER: {
        // Check hotkeys
        bool currentF6 = (GetAsyncKeyState(VK_F6) & 0x8000) != 0;
        bool currentF7 = (GetAsyncKeyState(VK_F7) & 0x8000) != 0;
        bool currentF8 = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
        bool currentF9 = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;

        if (currentF6 && !f6Pressed) {
            if (appState.mode == MODE_CALIBRATING) {
                finishCalibration();
            }
            else {
                startCalibration();
            }
            InvalidateRect(hwnd, NULL, TRUE);
        }

        if (currentF7 && !f7Pressed) {
            if (appState.mode == MODE_TRACKING_ANGLE) {
                finishAngleTracking();
            }
            else if (appState.mouseSensitivityCalibrated && appState.mode == MODE_READY) {
                startAngleTracking();
            }
            InvalidateRect(hwnd, NULL, TRUE);
        }

        if (currentF8 && !f8Pressed) {
            if (appState.mode == MODE_ANGLE_CAPTURED) {
                readCoordinatesAndCalculate();
                InvalidateRect(hwnd, NULL, TRUE);
            }
        }

        if (currentF9 && !f9Pressed) {
            resetApp();
            InvalidateRect(hwnd, NULL, TRUE);
        }

        f6Pressed = currentF6;
        f7Pressed = currentF7;
        f8Pressed = currentF8;
        f9Pressed = currentF9;

        // Update display if tracking
        if (appState.mode == MODE_CALIBRATING || appState.mode == MODE_TRACKING_ANGLE) {
            InvalidateRect(hwnd, NULL, TRUE);
        }
        break;
    }

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);

        // Background
        HBRUSH bgBrush = CreateSolidBrush(RGB(15, 15, 20));
        FillRect(hdc, &rect, bgBrush);
        DeleteObject(bgBrush);

        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);

        // Fonts
        HFONT titleFont = CreateFontA(20, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");

        HFONT normalFont = CreateFontA(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");

        HFONT bigFont = CreateFontA(16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");

        HFONT smallFont = CreateFontA(12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, "Segoe UI");

        int yPos = 10;

        // Title
        SelectObject(hdc, titleFont);
        SetTextColor(hdc, RGB(100, 200, 255));
        RECT titleRect = { 10, yPos, rect.right - 10, yPos + 30 };
        DrawTextA(hdc, "Stronghold Calculator", -1, &titleRect, DT_CENTER);
        yPos += 40;

        // Mode indicator
        SelectObject(hdc, bigFont);
        switch (appState.mode) {
        case MODE_CALIBRATING:
            SetTextColor(hdc, RGB(255, 200, 0));
            break;
        case MODE_TRACKING_ANGLE:
            SetTextColor(hdc, RGB(0, 255, 100));
            break;
        case MODE_ANGLE_CAPTURED:
            SetTextColor(hdc, RGB(255, 150, 0));
            break;
        case MODE_RESULTS:
            SetTextColor(hdc, RGB(100, 255, 100));
            break;
        default:
            SetTextColor(hdc, RGB(200, 200, 200));
            break;
        }

        RECT modeRect = { 10, yPos, rect.right - 10, yPos + 20 };
        string modeText;
        switch (appState.mode) {
        case MODE_CALIBRATING: modeText = "CALIBRATING"; break;
        case MODE_TRACKING_ANGLE: modeText = "TRACKING"; break;
        case MODE_ANGLE_CAPTURED: modeText = "ANGLE READY"; break;
        case MODE_RESULTS: modeText = "RESULTS"; break;
        default: modeText = "READY"; break;
        }
        DrawTextA(hdc, modeText.c_str(), -1, &modeRect, DT_CENTER);
        yPos += 30;

        // Real-time tracking display
        if (appState.mode == MODE_CALIBRATING && mouseTracker) {
            SelectObject(hdc, normalFont);
            SetTextColor(hdc, RGB(255, 255, 100));

            ostringstream ss;
            ss << "Movement: " << fixed << setprecision(0) << mouseTracker->getCurrentDistance() << " units";
            RECT trackRect = { 10, yPos, rect.right - 10, yPos + 20 };
            DrawTextA(hdc, ss.str().c_str(), -1, &trackRect, DT_CENTER);
            yPos += 30;
        }

        if (appState.mode == MODE_TRACKING_ANGLE && mouseTracker && appState.mouseSensitivityCalibrated) {
            SelectObject(hdc, normalFont);
            SetTextColor(hdc, RGB(100, 255, 255));

            ostringstream ss;
            ss << "Angle: " << fixed << setprecision(1) << mouseTracker->getCurrentAngle() << "°";
            RECT angleRect = { 10, yPos, rect.right - 10, yPos + 20 };
            DrawTextA(hdc, ss.str().c_str(), -1, &angleRect, DT_CENTER);
            yPos += 30;
        }

        if (appState.mode == MODE_ANGLE_CAPTURED) {
            SelectObject(hdc, normalFont);
            SetTextColor(hdc, RGB(255, 200, 100));

            ostringstream ss;
            ss << "Captured: " << fixed << setprecision(1) << appState.capturedAngle << "°";
            RECT capturedRect = { 10, yPos, rect.right - 10, yPos + 20 };
            DrawTextA(hdc, ss.str().c_str(), -1, &capturedRect, DT_CENTER);
            yPos += 30;
        }

        // Calibration status
        SelectObject(hdc, smallFont);
        if (appState.mouseSensitivityCalibrated) {
            SetTextColor(hdc, RGB(100, 255, 100));
            ostringstream ss;
            ss << "? Calibrated: " << fixed << setprecision(2)
                << appState.mouseSensitivityUnitsPerDegree << " u/deg";
            RECT calibRect = { 10, yPos, rect.right - 10, yPos + 18 };
            DrawTextA(hdc, ss.str().c_str(), -1, &calibRect, DT_LEFT);
        }
        else {
            SetTextColor(hdc, RGB(255, 150, 150));
            RECT calibRect = { 10, yPos, rect.right - 10, yPos + 18 };
            DrawTextA(hdc, "? Not calibrated", -1, &calibRect, DT_LEFT);
        }
        yPos += 25;

        // Current coordinates
        if (appState.currentCoordinates.x != 0 || appState.currentCoordinates.z != 0) {
            SetTextColor(hdc, RGB(200, 200, 255));
            ostringstream ss;
            ss << "Position: " << appState.currentCoordinates.x << ", " << appState.currentCoordinates.z;
            RECT posRect = { 10, yPos, rect.right - 10, yPos + 18 };
            DrawTextA(hdc, ss.str().c_str(), -1, &posRect, DT_LEFT);
            yPos += 25;
        }

        // Status
        SelectObject(hdc, normalFont);
        SetTextColor(hdc, RGB(255, 255, 150));
        RECT statusRect = { 10, yPos, rect.right - 10, yPos + 40 };
        DrawTextA(hdc, appState.status.c_str(), -1, &statusRect, DT_LEFT | DT_WORDBREAK);
        yPos += 50;

        // Hotkeys
        SelectObject(hdc, smallFont);
        SetTextColor(hdc, RGB(150, 150, 200));

        RECT hotkey1 = { 10, yPos, rect.right - 10, yPos + 16 };
        DrawTextA(hdc, "F6 - Calibrate (90° turn)", -1, &hotkey1, DT_LEFT);
        yPos += 18;

        RECT hotkey2 = { 10, yPos, rect.right - 10, yPos + 16 };
        DrawTextA(hdc, "F7 - Track angle to stronghold", -1, &hotkey2, DT_LEFT);
        yPos += 18;

        RECT hotkey3 = { 10, yPos, rect.right - 10, yPos + 16 };
        DrawTextA(hdc, "F8 - Read coords & calculate", -1, &hotkey3, DT_LEFT);
        yPos += 18;

        RECT hotkey4 = { 10, yPos, rect.right - 10, yPos + 16 };
        DrawTextA(hdc, "F9 - Reset", -1, &hotkey4, DT_LEFT);
        yPos += 30;

        // Results
        if (appState.showResults && !appState.strongholdCandidates.empty()) {
            SelectObject(hdc, bigFont);
            SetTextColor(hdc, RGB(100, 255, 100));
            RECT resultTitleRect = { 10, yPos, rect.right - 10, yPos + 20 };
            DrawTextA(hdc, "?? STRONGHOLD:", -1, &resultTitleRect, DT_CENTER);
            yPos += 30;

            for (size_t i = 0; i < min((size_t)2, appState.strongholdCandidates.size()); i++) {
                const auto& candidate = appState.strongholdCandidates[i];

                if (i > 0) {
                    SelectObject(hdc, smallFont);
                    SetTextColor(hdc, RGB(180, 180, 180));
                    RECT altRect = { 10, yPos, rect.right - 10, yPos + 16 };
                    DrawTextA(hdc, "Alternative:", -1, &altRect, DT_LEFT);
                    yPos += 20;
                }

                // Overworld coordinates
                SelectObject(hdc, normalFont);
                SetTextColor(hdc, RGB(255, 255, 100));
                ostringstream overworld;
                overworld << "Overworld: " << candidate.projectionX << ", " << candidate.projectionZ;
                RECT overworldRect = { 15, yPos, rect.right - 10, yPos + 18 };
                DrawTextA(hdc, overworld.str().c_str(), -1, &overworldRect, DT_LEFT);
                yPos += 22;

                // Nether coordinates
                SetTextColor(hdc, RGB(255, 150, 100));
                ostringstream nether;
                nether << "Nether: " << candidate.netherX << ", " << candidate.netherZ;
                RECT netherRect = { 15, yPos, rect.right - 10, yPos + 18 };
                DrawTextA(hdc, nether.str().c_str(), -1, &netherRect, DT_LEFT);
                yPos += 22;

                // Distance info
                SelectObject(hdc, smallFont);
                SetTextColor(hdc, RGB(180, 180, 180));
                ostringstream distance;
                distance << "Distance: " << candidate.distance << " blocks";
                RECT distRect = { 15, yPos, rect.right - 10, yPos + 16 };
                DrawTextA(hdc, distance.str().c_str(), -1, &distRect, DT_LEFT);
                yPos += 20;

                if (i < appState.strongholdCandidates.size() - 1) yPos += 10;
            }

            yPos += 15;

            // Instructions
            SelectObject(hdc, smallFont);
            SetTextColor(hdc, RGB(150, 255, 150));
            RECT instrRect = { 10, yPos, rect.right - 10, yPos + 30 };
            DrawTextA(hdc, "Go to these coordinates and dig down!\nPress F9 to reset for new calculation.", -1, &instrRect, DT_CENTER);
        }

        // Workflow instructions at bottom
        if (!appState.showResults && yPos < rect.bottom - 80) {
            SelectObject(hdc, smallFont);
            SetTextColor(hdc, RGB(120, 120, 120));

            RECT workflowRect = { 10, rect.bottom - 70, rect.right - 10, rect.bottom - 55 };
            DrawTextA(hdc, "WORKFLOW:", -1, &workflowRect, DT_LEFT);

            RECT step1Rect = { 10, rect.bottom - 50, rect.right - 10, rect.bottom - 35 };
            DrawTextA(hdc, "1. F6: Calibrate (turn exactly 90°)", -1, &step1Rect, DT_LEFT);

            RECT step2Rect = { 10, rect.bottom - 30, rect.right - 10, rect.bottom - 15 };
            DrawTextA(hdc, "2. F7: Track angle (face stronghold)", -1, &step2Rect, DT_LEFT);

            RECT step3Rect = { 10, rect.bottom - 10, rect.right - 10, rect.bottom + 5 };
            DrawTextA(hdc, "3. F8: Calculate location", -1, &step3Rect, DT_LEFT);
        }

        // Cleanup fonts
        DeleteObject(titleFont);
        DeleteObject(normalFont);
        DeleteObject(bigFont);
        DeleteObject(smallFont);

        EndPaint(hwnd, &ps);
        break;
    }

    case WM_NCHITTEST:
        return HTCAPTION; // Make window draggable

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        }
        break;

    case WM_DESTROY:
        KillTimer(hwnd, 1);
        delete mouseTracker;
        mouseTracker = nullptr;
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Register window class
    const wchar_t CLASS_NAME[] = L"StrongholdCalculator";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    // Get screen dimensions for positioning
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Position window on the right side of screen
    int xPos = screenWidth - WINDOW_WIDTH - 20;
    int yPos = (screenHeight - WINDOW_HEIGHT) / 2;

    // Create window
    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        CLASS_NAME,
        L"Stronghold Calculator",
        WS_POPUP | WS_VISIBLE,
        xPos, yPos,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        GdiplusShutdown(gdiplusToken);
        return 0;
    }

    // Set window transparency
    SetLayeredWindowAttributes(hwnd, 0, 245, LWA_ALPHA);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    GdiplusShutdown(gdiplusToken);
    return 0;
}