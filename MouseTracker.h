#pragma once

#include "Common.h"

class MouseAngleTracker {
private:
    HWND hwnd;
    bool tracking = false;
    double totalMouseMovement = 0.0;
    bool rawInputRegistered = false;
    double sensitivityUnitsPerDegree = 0.0;

public:
    MouseAngleTracker(HWND window);
    void registerRawInput();
    void processRawInput(LPARAM lParam);
    void startTracking();
    double stopTrackingAndGetDistance();
    double stopTrackingAndGetAngle();
    void setSensitivity(double unitsPerDegree);

    bool isTracking() const;
    bool isCalibrated() const;
    double getCurrentDistance() const;
    double getCurrentAngle() const;
};

extern MouseAngleTracker* mouseTracker;