#pragma once
#define NOMINMAX
#include "common.h"

// Generate all possible stronghold cells
void generateStrongholdCells();

// Calculate stronghold locations based on player position and eye angle
void calculateStrongholdLocationWithDistance(double playerX, double playerZ, double eyeAngle, double targetDistance = -1);