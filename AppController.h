#pragma once
#include "Common.h"
#include "StrongholdCalculator.h"
#include "CoordinateReader.h"
#include "Settings.h"

class AppController {
public:
    static void initialize();
    static void startCalibration();
    static void finishCalibration();
    static void startAngleTracking();
    static void finishAngleTracking();
    static void readCoordinatesAndCalculate();
    static void startSecondThrow();
    static void resetApp();
    static void showOptions();
};