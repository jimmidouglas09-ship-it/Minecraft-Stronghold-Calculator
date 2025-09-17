#pragma once
#define NOMINMAX
#include "common.h"

// Constants
const int DISTANCE_KEY_TIMEOUT_MS = 2000; // 2 seconds timeout between presses

// Function to handle F4 key press for distance calculation
void handleDistanceKey();