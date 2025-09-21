#pragma once

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

using namespace Gdiplus;
using namespace std;

// Constants
extern const int WINDOW_WIDTH;
extern const int WINDOW_HEIGHT;
extern const double M_PI;

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
    double probability = 1.0; // Calculated probability
    bool isTriangulated = false; // Whether this was found via triangulation
};

struct ThrowData {
    Vec3 position;
    double angle;
    bool isValid = false;
};

enum AppMode {
    MODE_READY,
    MODE_CALIBRATING,
    MODE_TRACKING_ANGLE,
    MODE_ANGLE_CAPTURED,
    MODE_RESULTS,
    MODE_SECOND_THROW_READY,
    MODE_SECOND_TRACKING_ANGLE,
    MODE_SECOND_ANGLE_CAPTURED
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

    // Triangulation support
    ThrowData firstThrow;
    ThrowData secondThrow;
    bool usingTriangulation = false;
    bool showOptionsWindow = false;
};

// Global state
extern AppState appState;
extern map<int, double> distanceProbabilities;
extern vector<StrongholdCell> strongholdCells;
// Forward declarations
class MouseAngleTracker;

// External declarations
extern MouseAngleTracker* mouseTracker;