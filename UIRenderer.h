#pragma once
#include "Common.h"

class UIRenderer {
public:
    static void renderUI(HDC hdc, const RECT& rect);

private:
    static HFONT createFont(int size, int weight = FW_NORMAL, const char* fontName = "Segoe UI");
    static void drawTitle(HDC hdc, const RECT& rect, int& yPos);
    static void drawModeIndicator(HDC hdc, const RECT& rect, int& yPos);
    static void drawRealTimeTracking(HDC hdc, const RECT& rect, int& yPos);
    static void drawCalibrationStatus(HDC hdc, const RECT& rect, int& yPos);
    static void drawCurrentCoordinates(HDC hdc, const RECT& rect, int& yPos);
    static void drawTriangulationStatus(HDC hdc, const RECT& rect, int& yPos);
    static void drawStatus(HDC hdc, const RECT& rect, int& yPos);
    static void drawHotkeys(HDC hdc, const RECT& rect, int& yPos);
    static void drawResults(HDC hdc, const RECT& rect, int& yPos);
    static void drawWorkflowInstructions(HDC hdc, const RECT& rect, int& yPos);
};