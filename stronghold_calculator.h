// stronghold_calculator.h - Simple header that uses common.h structures
#pragma once

#ifndef STRONGHOLD_CALCULATOR_H
#define STRONGHOLD_CALCULATOR_H

#include "common.h"

// Core function declarations - structures are already defined in common.h
void generateStrongholdCells();
void calculateStrongholdLocationWithDistance(double playerX, double playerZ, double eyeAngle, double targetDistance = -1.0);

// Enhanced calculation function declarations
double calculateMeasurementError(double alignError);
double calculateCoordPrecisionError(double distance);
double calculateFacingPrecisionError(double pixelError);
double calculateCombinedError(double measurementError, double precisionError);
std::pair<double, double> calculateEyeHoverPosition(double x1, double z1, double x2, double z2);

// Utility function declarations
double gaussianProbability(double x, double mean, double stdDev);

#endif // STRONGHOLD_CALCULATOR_H
