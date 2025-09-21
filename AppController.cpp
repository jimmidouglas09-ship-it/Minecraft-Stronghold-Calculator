#include "AppController.h"
#include "OptionsWindow.h"
#include "MouseTracker.h"  // Add this line
#include "StrongholdCalculator.h"
#include "CoordinateReader.h"
#include "Settings.h"

void AppController::initialize() {
    Settings::initialize();
    StrongholdCalculator::initializeDistanceProbabilities();
    StrongholdCalculator::generateStrongholdCells();
}

void AppController::startCalibration() {
    if (mouseTracker) {
        mouseTracker->startTracking();
        appState.mode = MODE_CALIBRATING;
        appState.status = "CALIBRATING: Turn exactly 90°, then press the calibrate key again";
    }
}

void AppController::finishCalibration() {
    if (mouseTracker && appState.mode == MODE_CALIBRATING) {
        double totalDistance = mouseTracker->stopTrackingAndGetDistance();
        if (totalDistance > 0) {
            appState.mouseSensitivityUnitsPerDegree = totalDistance / 90.0;
            mouseTracker->setSensitivity(appState.mouseSensitivityUnitsPerDegree);
            appState.mouseSensitivityCalibrated = true;
            appState.mode = MODE_READY;
            ostringstream ss;
            ss << "CALIBRATED: " << fixed << setprecision(3) << appState.mouseSensitivityUnitsPerDegree
                << " units/degree. Press track angle key to start.";
            appState.status = ss.str();
        }
        else {
            appState.mode = MODE_READY;
            appState.status = "Calibration failed - no mouse movement detected";
        }
    }
}

void AppController::startAngleTracking() {
    if (mouseTracker && appState.mouseSensitivityCalibrated) {
        mouseTracker->startTracking();

        if (appState.mode == MODE_SECOND_THROW_READY) {
            appState.mode = MODE_SECOND_TRACKING_ANGLE;
            appState.status = "TRACKING SECOND ANGLE: Face stronghold from new position, then press track key again";
        }
        else {
            appState.mode = MODE_TRACKING_ANGLE;
            appState.status = "TRACKING: Turn to face stronghold, then press track angle key again";
        }
    }
}

void AppController::finishAngleTracking() {
    if (mouseTracker && (appState.mode == MODE_TRACKING_ANGLE || appState.mode == MODE_SECOND_TRACKING_ANGLE)) {
        double capturedAngle = mouseTracker->stopTrackingAndGetAngle();

        if (appState.mode == MODE_SECOND_TRACKING_ANGLE) {
            appState.mode = MODE_SECOND_ANGLE_CAPTURED;
            appState.secondThrow.angle = capturedAngle;
            ostringstream ss;
            ss << "SECOND ANGLE CAPTURED: " << fixed << setprecision(1) << capturedAngle
                << "° - Press calculate key to triangulate";
            appState.status = ss.str();
        }
        else {
            appState.capturedAngle = capturedAngle;
            appState.mode = MODE_ANGLE_CAPTURED;
            ostringstream ss;
            ss << "ANGLE CAPTURED: " << fixed << setprecision(1) << capturedAngle
                << "° - Press calculate key to read coordinates";
            appState.status = ss.str();
        }
    }
}

void AppController::readCoordinatesAndCalculate() {
    CoordinateReader::findMinecraftWindow();

    if (appState.mode == MODE_SECOND_ANGLE_CAPTURED) {
        // Handle second throw triangulation
        if (!appState.minecraftWindow) {
            appState.status = "Minecraft not found - enter coordinates manually for second throw";
            return;
        }

        Vec3 coords;
        if (CoordinateReader::GetShownCoordinates(appState.minecraftWindow, &coords)) {
            appState.secondThrow.position = coords;
            appState.secondThrow.isValid = true;

            // Now we have both throws, calculate via triangulation
            if (appState.firstThrow.isValid && appState.secondThrow.isValid) {
                StrongholdCalculator::calculateStrongholdFromTriangulation(appState.firstThrow, appState.secondThrow);
                appState.usingTriangulation = true;
            }
            else {
                appState.status = "Error: Missing first throw data";
            }
        }
        else {
            appState.status = "Could not read coordinates for second throw";
        }
        return;
    }

    // Handle first throw or single throw calculation
    if (!appState.minecraftWindow) {
        // Try anyway with last known coordinates
        if (appState.currentCoordinates.x == 0 && appState.currentCoordinates.z == 0) {
            appState.status = "Minecraft not found - using default position (0,0)";
            appState.currentCoordinates = { 0, 64, 0 };
        }
    }
    else {
        Vec3 coords;
        if (CoordinateReader::GetShownCoordinates(appState.minecraftWindow, &coords)) {
            appState.currentCoordinates = coords;
        }
        else {
            appState.status = "Could not read coordinates - using last known position";
        }
    }

    // Store first throw data for potential triangulation
    appState.firstThrow.position = appState.currentCoordinates;
    appState.firstThrow.angle = appState.capturedAngle;
    appState.firstThrow.isValid = true;

    // Calculate stronghold location from single angle
    StrongholdCalculator::calculateStrongholdFromAngle(
        appState.currentCoordinates.x,
        appState.currentCoordinates.z,
        appState.capturedAngle
    );

    appState.usingTriangulation = false;
}

void AppController::startSecondThrow() {
    if (appState.firstThrow.isValid && appState.mouseSensitivityCalibrated) {
        appState.mode = MODE_SECOND_THROW_READY;
        appState.status = "SECOND THROW: Move to different position, then press track angle key";
    }
    else if (!appState.mouseSensitivityCalibrated) {
        appState.status = "Must calibrate mouse sensitivity first";
    }
    else {
        appState.status = "No first throw data available - calculate first throw first";
    }
}

void AppController::resetApp() {
    appState.strongholdCandidates.clear();
    appState.showResults = false;
    appState.mode = MODE_READY;
    appState.capturedAngle = 0.0;
    appState.usingTriangulation = false;
    appState.firstThrow = ThrowData{};
    appState.secondThrow = ThrowData{};

    if (appState.mouseSensitivityCalibrated) {
        appState.status = "Ready - Press track angle key to start";
    }
    else {
        appState.status = "Press calibrate key to calibrate mouse first";
    }
}

void AppController::showOptions() {
    appState.showOptionsWindow = true;
}