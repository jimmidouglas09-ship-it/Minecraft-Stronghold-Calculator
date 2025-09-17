#pragma once
#define NOMINMAX
#include <windows.h>
#include <gdiplus.h>
#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include <map>
#include <iomanip>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// Forward declarations
ATOM MyRegisterClass(HINSTANCE hInstance);
ATOM MyRegisterOverlayClass(HINSTANCE hInstance);
BOOL InitInstance(HINSTANCE, int);
BOOL InitOverlay(HINSTANCE hInstance);

// Global variables
extern HINSTANCE hInst;
extern WCHAR szTitle[];
extern WCHAR szWindowClass[];
extern ULONG_PTR gdiplusToken;

// Data structures
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
    double rawProb;
    double conditionalProb;
    int distance;
    int distanceFromOrigin;
    int distanceRange;
    std::wstring bounds;
};

// Application state structure
struct ApplicationState {
    Vec3 latestCoords = { 0, 0, 0 };
    Vec3 coord1 = { 0, 0, 0 };
    Vec3 coord2 = { 0, 0, 0 };
    int capturePhase = 0; // 0 = none, 1 = first captured, 2 = second captured
    double lastAngle = 0.0;

    // Distance calculation variables
    int distanceKeyPresses = 0;
    double calculatedDistance = 0.0;
    std::chrono::steady_clock::time_point lastDistanceKeyPress;

    // Validation flags
    bool tabPressedFirst = false;
    bool f4PressedFirst = false;
    bool distanceValidationFailed = false;
    std::wstring validationErrorMessage = L"";
};

// Global application state
extern ApplicationState appState;

// Stronghold data
extern std::map<int, double> distanceProbabilities;
extern std::vector<StrongholdCell> strongholdCells;
extern std::vector<StrongholdCandidate> strongholdCandidates;
