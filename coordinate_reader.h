#pragma once
#define NOMINMAX
#include "common.h"

// Function to capture bitmap from window
std::unique_ptr<Bitmap> BitmapFromHWND(HWND hwnd);

// Function to read coordinates from Minecraft window
int GetShownCoordinates(HWND hwnd, Vec3* coordinates);

// Utility function for angle calculation
double angleBetween(double x1, double y1, double x2, double y2); 
