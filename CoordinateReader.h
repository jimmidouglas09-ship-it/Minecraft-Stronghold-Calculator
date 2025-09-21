#pragma once

#include "Common.h"

class CoordinateReader {
public:
    static unique_ptr<Bitmap> BitmapFromHWND(HWND hwnd);
    static int GetShownCoordinates(HWND hwnd, Vec3* coordinates);
    static void findMinecraftWindow();
}; 
