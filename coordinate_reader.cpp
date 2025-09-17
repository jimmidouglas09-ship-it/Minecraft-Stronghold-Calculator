#define NOMINMAX
#include "coordinate_reader.h"

std::unique_ptr<Bitmap> BitmapFromHWND(HWND hwnd) {
    if (IsIconic(hwnd)) ShowWindow(hwnd, SW_RESTORE);
    RECT rc; GetWindowRect(hwnd, &rc);
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;

    HDC hdc = GetDC(hwnd);
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, width, height);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);

    PrintWindow(hwnd, memDC, PW_RENDERFULLCONTENT);
    auto pBitmap = std::make_unique<Bitmap>(hBitmap, nullptr);

    SelectObject(memDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(memDC);
    ReleaseDC(hwnd, hdc);

    return pBitmap;
}

int GetShownCoordinates(HWND hwnd, Vec3* coordinates) {
    auto pBitmap = BitmapFromHWND(hwnd);
    int width = pBitmap->GetWidth();
    int height = pBitmap->GetHeight();
    int searchWidth = std::max(width / 3, std::min(125, width));
    int searchHeight = height / 3;

    Gdiplus::BitmapData bitmapData;
    Gdiplus::Rect rect(0, 0, searchWidth, searchHeight);
    pBitmap->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData);

    int startTextX = 0, startTextY = 0, streak = 0;
    int stride = bitmapData.Stride / sizeof(ARGB);
    ARGB* pixels = static_cast<ARGB*>(bitmapData.Scan0);

    for (int y = 30; y < searchHeight; y++) {
        for (int x = 8; x < searchWidth; x++) {
            if (pixels[y * stride + x] == 0xFFFFFFFF) {
                if (!startTextX) { startTextX = x; startTextY = y; }
                streak++;
            }
            else if (streak < 4) streak = 0;
            else if (streak >= 4) break;
        }
        if (streak >= 4) break;
    }

    if (streak < 4) return 0;
    int scale = streak / 4;
    startTextX += 44 * scale;

    int coords[3] = { 0, 0, 0 };
    int index = 0;
    bool isSigned = false;

    while (startTextX < searchWidth) {
        unsigned int columnMask = 0;
        for (int dy = 0; dy < 7; dy++) {
            columnMask <<= 1;
            if (pixels[(startTextY + dy * scale) * stride + startTextX] == 0xFFFFFFFF)
                columnMask |= 1;
        }

        int digit = -1;
        switch (columnMask) {
        case 0b0111110: digit = 0; break;
        case 0b0000001: digit = 1; break;
        case 0b0100011: digit = 2; break;
        case 0b0100010: digit = 3; break;
        case 0b0001100: digit = 4; break;
        case 0b1110010: digit = 5; break;
        case 0b0011110: digit = 6; break;
        case 0b1100000: digit = 7; break;
        case 0b0110110: digit = 8; break;
        case 0b0110000: digit = 9; break;
        case 0b0001000: isSigned = true; break;
        case 0b0000011:
            if (isSigned) coords[index] *= -1;
            if (++index > 2) break;
            isSigned = false;
            break;
        default:
            if (index >= 2) break;
            if (isSigned) coords[index] *= -1;
            break;
        }

        if (digit != -1)
            coords[index] = coords[index] * 10 + digit;

        startTextX += 6 * scale;
    }

    if (isSigned && index <= 2) {
        coords[index] *= -1;
    }

    pBitmap->UnlockBits(&bitmapData);
    coordinates->x = coords[0];
    coordinates->y = coords[1];
    coordinates->z = coords[2];
    return 1;
}

double angleBetween(double x1, double y1, double x2, double y2) {
    double dx = x2 - x1;
    double dz = y2 - y1;
    double angle = std::atan2(dx, -dz) * 180.0 / M_PI; // note the -dz
    if (angle < 0) angle += 360.0;
    return angle;
}