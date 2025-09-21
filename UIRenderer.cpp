#include "UIRenderer.h"
#include "MouseTracker.h"
#include "Settings.h"

HFONT UIRenderer::createFont(int size, int weight, const char* fontName) {
    return CreateFontA(size, 0, 0, 0, weight, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, fontName);
}

void UIRenderer::renderUI(HDC hdc, const RECT& rect) {
    // Background
    HBRUSH bgBrush = CreateSolidBrush(RGB(15, 15, 20));
    FillRect(hdc, &rect, bgBrush);
    DeleteObject(bgBrush);

    SetTextColor(hdc, RGB(255, 255, 255));
    SetBkMode(hdc, TRANSPARENT);

    int yPos = 10;

    drawTitle(hdc, rect, yPos);
    drawModeIndicator(hdc, rect, yPos);
    drawRealTimeTracking(hdc, rect, yPos);
    drawCalibrationStatus(hdc, rect, yPos);
    drawCurrentCoordinates(hdc, rect, yPos);
    drawTriangulationStatus(hdc, rect, yPos);
    drawStatus(hdc, rect, yPos);
    drawHotkeys(hdc, rect, yPos);
    drawResults(hdc, rect, yPos);
    drawWorkflowInstructions(hdc, rect, yPos);
}

void UIRenderer::drawTitle(HDC hdc, const RECT& rect, int& yPos) {
    HFONT titleFont = createFont(20, FW_BOLD);
    SelectObject(hdc, titleFont);
    SetTextColor(hdc, RGB(100, 200, 255));
    RECT titleRect = { 10, yPos, rect.right - 10, yPos + 30 };
    DrawTextA(hdc, "Stronghold Calculator", -1, &titleRect, DT_CENTER);
    yPos += 40;
    DeleteObject(titleFont);
}

void UIRenderer::drawModeIndicator(HDC hdc, const RECT& rect, int& yPos) {
    HFONT bigFont = createFont(16, FW_BOLD);
    SelectObject(hdc, bigFont);

    switch (appState.mode) {
    case MODE_CALIBRATING:
        SetTextColor(hdc, RGB(255, 200, 0));
        break;
    case MODE_TRACKING_ANGLE:
    case MODE_SECOND_TRACKING_ANGLE:
        SetTextColor(hdc, RGB(0, 255, 100));
        break;
    case MODE_ANGLE_CAPTURED:
    case MODE_SECOND_ANGLE_CAPTURED:
        SetTextColor(hdc, RGB(255, 150, 0));
        break;
    case MODE_RESULTS:
        SetTextColor(hdc, RGB(100, 255, 100));
        break;
    case MODE_SECOND_THROW_READY:
        SetTextColor(hdc, RGB(255, 100, 255));
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
    case MODE_SECOND_TRACKING_ANGLE: modeText = "TRACKING 2ND"; break;
    case MODE_ANGLE_CAPTURED: modeText = "ANGLE READY"; break;
    case MODE_SECOND_ANGLE_CAPTURED: modeText = "2ND ANGLE READY"; break;
    case MODE_SECOND_THROW_READY: modeText = "2ND THROW READY"; break;
    case MODE_RESULTS: modeText = "RESULTS"; break;
    default: modeText = "READY"; break;
    }
    DrawTextA(hdc, modeText.c_str(), -1, &modeRect, DT_CENTER);
    yPos += 30;
    DeleteObject(bigFont);
}

void UIRenderer::drawRealTimeTracking(HDC hdc, const RECT& rect, int& yPos) {
    HFONT normalFont = createFont(14, FW_NORMAL);

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

    if ((appState.mode == MODE_TRACKING_ANGLE || appState.mode == MODE_SECOND_TRACKING_ANGLE) &&
        mouseTracker && appState.mouseSensitivityCalibrated) {
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

    if (appState.mode == MODE_SECOND_ANGLE_CAPTURED) {
        SelectObject(hdc, normalFont);
        SetTextColor(hdc, RGB(255, 200, 100));

        ostringstream ss;
        ss << "2nd Captured: " << fixed << setprecision(1) << appState.secondThrow.angle << "°";
        RECT capturedRect = { 10, yPos, rect.right - 10, yPos + 20 };
        DrawTextA(hdc, ss.str().c_str(), -1, &capturedRect, DT_CENTER);
        yPos += 30;
    }

    DeleteObject(normalFont);
}

void UIRenderer::drawCalibrationStatus(HDC hdc, const RECT& rect, int& yPos) {
    HFONT smallFont = createFont(12, FW_NORMAL);
    SelectObject(hdc, smallFont);

    if (appState.mouseSensitivityCalibrated) {
        SetTextColor(hdc, RGB(100, 255, 100));
        ostringstream ss;
        ss << "Calibrated: " << fixed << setprecision(2)
            << appState.mouseSensitivityUnitsPerDegree << " u/deg";
        RECT calibRect = { 10, yPos, rect.right - 10, yPos + 18 };
        DrawTextA(hdc, ss.str().c_str(), -1, &calibRect, DT_LEFT);
    }
    else {
        SetTextColor(hdc, RGB(255, 150, 150));
        RECT calibRect = { 10, yPos, rect.right - 10, yPos + 18 };
        DrawTextA(hdc, "Not calibrated", -1, &calibRect, DT_LEFT);
    }
    yPos += 25;
    DeleteObject(smallFont);
}

void UIRenderer::drawCurrentCoordinates(HDC hdc, const RECT& rect, int& yPos) {
    if (appState.currentCoordinates.x != 0 || appState.currentCoordinates.z != 0) {
        HFONT smallFont = createFont(12, FW_NORMAL);
        SelectObject(hdc, smallFont);
        SetTextColor(hdc, RGB(200, 200, 255));
        ostringstream ss;
        ss << "Position: " << appState.currentCoordinates.x << ", " << appState.currentCoordinates.z;
        RECT posRect = { 10, yPos, rect.right - 10, yPos + 18 };
        DrawTextA(hdc, ss.str().c_str(), -1, &posRect, DT_LEFT);
        yPos += 25;
        DeleteObject(smallFont);
    }
}

void UIRenderer::drawTriangulationStatus(HDC hdc, const RECT& rect, int& yPos) {
    if (appState.firstThrow.isValid) {
        HFONT smallFont = createFont(12, FW_NORMAL);
        SelectObject(hdc, smallFont);

        if (appState.usingTriangulation) {
            SetTextColor(hdc, RGB(100, 255, 100));
            RECT triRect = { 10, yPos, rect.right - 10, yPos + 18 };
            DrawTextA(hdc, "Using triangulation (2 throws)", -1, &triRect, DT_LEFT);
        }
        else {
            SetTextColor(hdc, RGB(200, 200, 100));
            RECT triRect = { 10, yPos, rect.right - 10, yPos + 18 };
            DrawTextA(hdc, "First throw saved (single angle)", -1, &triRect, DT_LEFT);
        }
        yPos += 25;
        DeleteObject(smallFont);
    }
}

void UIRenderer::drawStatus(HDC hdc, const RECT& rect, int& yPos) {
    HFONT normalFont = createFont(14, FW_NORMAL);
    SelectObject(hdc, normalFont);
    SetTextColor(hdc, RGB(255, 255, 150));
    RECT statusRect = { 10, yPos, rect.right - 10, yPos + 40 };
    DrawTextA(hdc, appState.status.c_str(), -1, &statusRect, DT_LEFT | DT_WORDBREAK);
    yPos += 50;
    DeleteObject(normalFont);
}

void UIRenderer::drawHotkeys(HDC hdc, const RECT& rect, int& yPos) {
    HFONT smallFont = createFont(12, FW_NORMAL);
    SelectObject(hdc, smallFont);
    SetTextColor(hdc, RGB(150, 150, 200));

    string calibrateKeyName = Settings::virtualKeyToString(Settings::hotkeys.calibrateKey);
    string trackKeyName = Settings::virtualKeyToString(Settings::hotkeys.trackAngleKey);
    string calculateKeyName = Settings::virtualKeyToString(Settings::hotkeys.calculateKey);
    string resetKeyName = Settings::virtualKeyToString(Settings::hotkeys.resetKey);
    string optionsKeyName = Settings::virtualKeyToString(Settings::hotkeys.optionsKey);
    string secondThrowKeyName = Settings::virtualKeyToString(Settings::hotkeys.secondThrowKey);

    RECT hotkey1 = { 10, yPos, rect.right - 10, yPos + 16 };
    string hotkey1Text = calibrateKeyName + " - Calibrate (90° turn)";
    DrawTextA(hdc, hotkey1Text.c_str(), -1, &hotkey1, DT_LEFT);
    yPos += 18;

    RECT hotkey2 = { 10, yPos, rect.right - 10, yPos + 16 };
    string hotkey2Text = trackKeyName + " - Track angle to stronghold";
    DrawTextA(hdc, hotkey2Text.c_str(), -1, &hotkey2, DT_LEFT);
    yPos += 18;

    RECT hotkey3 = { 10, yPos, rect.right - 10, yPos + 16 };
    string hotkey3Text = calculateKeyName + " - Read coords & calculate";
    DrawTextA(hdc, hotkey3Text.c_str(), -1, &hotkey3, DT_LEFT);
    yPos += 18;

    RECT hotkey4 = { 10, yPos, rect.right - 10, yPos + 16 };
    string hotkey4Text = secondThrowKeyName + " - Second throw (triangulate)";
    DrawTextA(hdc, hotkey4Text.c_str(), -1, &hotkey4, DT_LEFT);
    yPos += 18;

    RECT hotkey5 = { 10, yPos, rect.right - 10, yPos + 16 };
    string hotkey5Text = resetKeyName + " - Reset";
    DrawTextA(hdc, hotkey5Text.c_str(), -1, &hotkey5, DT_LEFT);
    yPos += 18;

    RECT hotkey6 = { 10, yPos, rect.right - 10, yPos + 16 };
    string hotkey6Text = optionsKeyName + " - Options";
    DrawTextA(hdc, hotkey6Text.c_str(), -1, &hotkey6, DT_LEFT);
    yPos += 30;

    DeleteObject(smallFont);
}

void UIRenderer::drawResults(HDC hdc, const RECT& rect, int& yPos) {
    if (appState.showResults && !appState.strongholdCandidates.empty()) {
        HFONT bigFont = createFont(16, FW_BOLD);
        HFONT normalFont = createFont(14, FW_NORMAL);
        HFONT smallFont = createFont(12, FW_NORMAL);

        SelectObject(hdc, bigFont);
        SetTextColor(hdc, RGB(100, 255, 100));
        RECT resultTitleRect = { 10, yPos, rect.right - 10, yPos + 20 };

        string titleText = "STRONGHOLD:";
        if (appState.usingTriangulation) {
            titleText = "STRONGHOLD (TRIANGULATED):";
        }
        DrawTextA(hdc, titleText.c_str(), -1, &resultTitleRect, DT_CENTER);
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

            // Distance and probability info
            SelectObject(hdc, smallFont);
            SetTextColor(hdc, RGB(180, 180, 180));
            ostringstream distance;
            distance << "Distance: " << (int)candidate.distance << " blocks";
            if (!candidate.isTriangulated && candidate.probability > 0) {
                distance << " (prob: " << fixed << setprecision(1) << (candidate.probability * 100) << "%)";
            }
            RECT distRect = { 15, yPos, rect.right - 10, yPos + 16 };
            DrawTextA(hdc, distance.str().c_str(), -1, &distRect, DT_LEFT);
            yPos += 20;

            if (i < appState.strongholdCandidates.size() - 1) yPos += 10;
        }

        yPos += 15;

        // Instructions
        SelectObject(hdc, smallFont);
        SetTextColor(hdc, RGB(150, 255, 150));
        RECT instrRect = { 10, yPos, rect.right - 10, yPos + 50 };
        string instructions = "Go to these coordinates and dig down!\n";
        if (!appState.usingTriangulation && appState.firstThrow.isValid) {
            instructions += "Press " + Settings::virtualKeyToString(Settings::hotkeys.secondThrowKey) +
                " for second throw or " + Settings::virtualKeyToString(Settings::hotkeys.resetKey) + " to reset.";
        }
        else {
            instructions += "Press " + Settings::virtualKeyToString(Settings::hotkeys.resetKey) + " to reset for new calculation.";
        }
        DrawTextA(hdc, instructions.c_str(), -1, &instrRect, DT_CENTER);

        DeleteObject(bigFont);
        DeleteObject(normalFont);
        DeleteObject(smallFont);
    }
}

void UIRenderer::drawWorkflowInstructions(HDC hdc, const RECT& rect, int& yPos) {
    if (!appState.showResults && yPos < rect.bottom - 120) {
        HFONT smallFont = createFont(12, FW_NORMAL);
        SelectObject(hdc, smallFont);
        SetTextColor(hdc, RGB(120, 120, 120));

        string calibrateKey = Settings::virtualKeyToString(Settings::hotkeys.calibrateKey);
        string trackKey = Settings::virtualKeyToString(Settings::hotkeys.trackAngleKey);
        string calculateKey = Settings::virtualKeyToString(Settings::hotkeys.calculateKey);
        string secondThrowKey = Settings::virtualKeyToString(Settings::hotkeys.secondThrowKey);

        RECT workflowRect = { 10, rect.bottom - 110, rect.right - 10, rect.bottom - 95 };
        DrawTextA(hdc, "WORKFLOW:", -1, &workflowRect, DT_LEFT);

        RECT step1Rect = { 10, rect.bottom - 90, rect.right - 10, rect.bottom - 75 };
        string step1 = "1. " + calibrateKey + ": Calibrate (turn exactly 90°)";
        DrawTextA(hdc, step1.c_str(), -1, &step1Rect, DT_LEFT);

        RECT step2Rect = { 10, rect.bottom - 70, rect.right - 10, rect.bottom - 55 };
        string step2 = "2. " + trackKey + ": Track angle (face stronghold)";
        DrawTextA(hdc, step2.c_str(), -1, &step2Rect, DT_LEFT);

        RECT step3Rect = { 10, rect.bottom - 50, rect.right - 10, rect.bottom - 35 };
        string step3 = "3. " + calculateKey + ": Calculate location";
        DrawTextA(hdc, step3.c_str(), -1, &step3Rect, DT_LEFT);

        RECT step4Rect = { 10, rect.bottom - 30, rect.right - 10, rect.bottom - 15 };
        string step4 = "4. " + secondThrowKey + ": Optional second throw";
        DrawTextA(hdc, step4.c_str(), -1, &step4Rect, DT_LEFT);

        DeleteObject(smallFont);
    }
}