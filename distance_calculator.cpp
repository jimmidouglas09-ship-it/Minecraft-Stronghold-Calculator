#define NOMINMAX
#include "distance_calculator.h"
#include "overlay_window.h"

void handleDistanceKey() {
    // If TAB was pressed first, ignore F4 presses
    if (appState.tabPressedFirst) {
        return;
    }

    // Mark that F4 was pressed first
    if (appState.distanceKeyPresses == 0 && appState.capturePhase == 0) {
        appState.f4PressedFirst = true;
        ShowOverlay(); // Show overlay when F4 is first pressed
    }

    auto now = std::chrono::steady_clock::now();

    // Check if this is a new sequence (timeout exceeded)
    if (appState.distanceKeyPresses > 0) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - appState.lastDistanceKeyPress);
        if (elapsed.count() > DISTANCE_KEY_TIMEOUT_MS) {
            appState.distanceKeyPresses = 0;
        }
    }

    appState.distanceKeyPresses++;
    appState.lastDistanceKeyPress = now;

    // Calculate distance: 3655 / (presses / 2)
    double divisor = appState.distanceKeyPresses / 2.0;
    appState.calculatedDistance = 3655.0 / divisor;

    UpdateOverlay();
}