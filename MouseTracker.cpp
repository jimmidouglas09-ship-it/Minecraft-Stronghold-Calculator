#include "MouseTracker.h"

MouseAngleTracker* mouseTracker = nullptr;

MouseAngleTracker::MouseAngleTracker(HWND window) : hwnd(window) {
    registerRawInput();
}

void MouseAngleTracker::registerRawInput() {
    RAWINPUTDEVICE rid;
    rid.usUsagePage = 0x01;
    rid.usUsage = 0x02;
    rid.dwFlags = RIDEV_INPUTSINK;
    rid.hwndTarget = hwnd;

    if (RegisterRawInputDevices(&rid, 1, sizeof(rid))) {
        rawInputRegistered = true;
    }
}

void MouseAngleTracker::processRawInput(LPARAM lParam) {
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

void MouseAngleTracker::startTracking() {
    totalMouseMovement = 0.0;
    tracking = true;
}

double MouseAngleTracker::stopTrackingAndGetDistance() {
    tracking = false;
    return totalMouseMovement;
}

double MouseAngleTracker::stopTrackingAndGetAngle() {
    tracking = false;
    if (sensitivityUnitsPerDegree <= 0) return 0.0;
    return totalMouseMovement / sensitivityUnitsPerDegree;
}

void MouseAngleTracker::setSensitivity(double unitsPerDegree) {
    sensitivityUnitsPerDegree = unitsPerDegree;
}

bool MouseAngleTracker::isTracking() const {
    return tracking;
}

bool MouseAngleTracker::isCalibrated() const {
    return sensitivityUnitsPerDegree > 0;
}

double MouseAngleTracker::getCurrentDistance() const {
    return totalMouseMovement;
}

double MouseAngleTracker::getCurrentAngle() const {
    if (sensitivityUnitsPerDegree <= 0) return 0.0;
    return totalMouseMovement / sensitivityUnitsPerDegree;
}