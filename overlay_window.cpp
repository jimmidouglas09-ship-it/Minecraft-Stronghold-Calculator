#define NOMINMAX
#include "overlay_window.h"
#include "stronghold_calculator.h"

WCHAR szOverlayClass[] = L"MCOverlayClass";
HWND hOverlayWnd = NULL;
bool overlayVisible = false;

// Variables for dragging
bool isDragging = false;
POINT dragOffset = { 0, 0 };

ATOM MyRegisterOverlayClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = OverlayProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    wcex.lpszClassName = szOverlayClass;
    return RegisterClassExW(&wcex);
}

BOOL InitOverlay(HINSTANCE hInstance) {
    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Position overlay in top-right corner - made smaller but with clear fonts
    int overlayWidth = 380;  // Reduced from 450
    int overlayHeight = 320; // Reduced from 400
    int x = screenWidth - overlayWidth - 20;
    int y = 20;

    hOverlayWnd = CreateWindowExW(
        WS_EX_TOPMOST | WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
        szOverlayClass,
        L"MC Overlay",
        WS_POPUP,
        x, y, overlayWidth, overlayHeight,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!hOverlayWnd) {
        MessageBox(NULL, L"Failed to create overlay window", L"Error", MB_OK);
        return FALSE;
    }

    // Make window semi-transparent with black as transparent color
    SetLayeredWindowAttributes(hOverlayWnd, RGB(0, 0, 0), 200, LWA_COLORKEY | LWA_ALPHA);

    // Don't show the window initially - it starts hidden
    overlayVisible = false;

    return TRUE;
}

void UpdateOverlay() {
    if (hOverlayWnd && overlayVisible) {
        InvalidateRect(hOverlayWnd, NULL, TRUE);
    }
}

void ShowOverlay() {
    if (hOverlayWnd && !overlayVisible) {
        ShowWindow(hOverlayWnd, SW_SHOW);
        SetWindowPos(hOverlayWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        overlayVisible = true;
        UpdateOverlay();
    }
}

void HideOverlay() {
    if (hOverlayWnd && overlayVisible) {
        ShowWindow(hOverlayWnd, SW_HIDE);
        overlayVisible = false;
        isDragging = false; // Reset dragging state when hiding
    }
}

LRESULT CALLBACK OverlayProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_LBUTTONDOWN:
    {
        // Start dragging
        isDragging = true;
        SetCapture(hWnd);

        POINT cursorPos;
        GetCursorPos(&cursorPos);

        RECT windowRect;
        GetWindowRect(hWnd, &windowRect);

        dragOffset.x = cursorPos.x - windowRect.left;
        dragOffset.y = cursorPos.y - windowRect.top;
    }
    break;

    case WM_LBUTTONUP:
    {
        // Stop dragging
        if (isDragging) {
            isDragging = false;
            ReleaseCapture();
        }
    }
    break;

    case WM_MOUSEMOVE:
    {
        if (isDragging) {
            POINT cursorPos;
            GetCursorPos(&cursorPos);

            int newX = cursorPos.x - dragOffset.x;
            int newY = cursorPos.y - dragOffset.y;

            // Keep window on screen
            int screenWidth = GetSystemMetrics(SM_CXSCREEN);
            int screenHeight = GetSystemMetrics(SM_CYSCREEN);

            RECT windowRect;
            GetWindowRect(hWnd, &windowRect);
            int windowWidth = windowRect.right - windowRect.left;
            int windowHeight = windowRect.bottom - windowRect.top;

            if (newX < 0) newX = 0;
            if (newY < 0) newY = 0;
            if (newX + windowWidth > screenWidth) newX = screenWidth - windowWidth;
            if (newY + windowHeight > screenHeight) newY = screenHeight - windowHeight;

            SetWindowPos(hWnd, NULL, newX, newY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }
    break;

    case WM_SETCURSOR:
    {
        // Show move cursor when hovering over window
        SetCursor(LoadCursor(NULL, IDC_SIZEALL));
        return TRUE;
    }
    break;

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);

        Graphics graphics(hdc);
        graphics.SetSmoothingMode(SmoothingModeAntiAlias);

        // Use dark background
        graphics.Clear(Color(240, 15, 20, 35)); // Dark blue-gray background

        // Draw border with grab handle indication
        Pen borderPen(Color(255, 100, 150, 200), 2);
        RECT rect;
        GetClientRect(hWnd, &rect);
        graphics.DrawRectangle(&borderPen, 1, 1, rect.right - 2, rect.bottom - 2);

        // Draw small grip indicator in top-left corner
        SolidBrush gripBrush(Color(255, 100, 150, 200));
        graphics.FillRectangle(&gripBrush, 3, 3, 8, 8);

        // Improved fonts - made slightly larger for smaller window
        FontFamily fontFamily(L"Consolas"); // Monospace font for better alignment
        Font headerFont(&fontFamily, 15, FontStyleBold, UnitPixel);   // Increased from 16
        Font coordFont(&fontFamily, 13, FontStyleBold, UnitPixel);    // Increased from 14
        Font infoFont(&fontFamily, 12, FontStyleRegular, UnitPixel);  // Same
        Font smallFont(&fontFamily, 11, FontStyleRegular, UnitPixel); // Same

        // Better color scheme
        SolidBrush whiteBrush(Color(255, 255, 255, 255));
        SolidBrush lightGrayBrush(Color(255, 200, 200, 200));
        SolidBrush greenBrush(Color(255, 120, 255, 120));
        SolidBrush yellowBrush(Color(255, 255, 220, 100));
        SolidBrush orangeBrush(Color(255, 255, 165, 0));
        SolidBrush redBrush(Color(255, 255, 120, 120));
        SolidBrush cyanBrush(Color(255, 100, 200, 255)); // For nether coords

        int y = 12;
        int x = 18; // Slightly more margin to account for grip

        // Title with better styling - more compact
        graphics.DrawString(L"Stronghold Finder", -1, &headerFont,
            PointF((REAL)x, (REAL)y), &whiteBrush);
        y += 22; // Reduced spacing

        // Status line - more compact
        if (appState.f4PressedFirst && appState.distanceKeyPresses > 0) {
            std::wstringstream ss;
            ss << L"Dist: " << std::fixed << std::setprecision(0)
                << appState.calculatedDistance << L" blocks (x" << appState.distanceKeyPresses << L")";
            graphics.DrawString(ss.str().c_str(), -1, &smallFont,
                PointF((REAL)x, (REAL)y), &yellowBrush);
            y += 18;
        }
        else if (appState.tabPressedFirst) {
            graphics.DrawString(L"Distance calculation skipped", -1, &smallFont,
                PointF((REAL)x, (REAL)y), &lightGrayBrush);
            y += 18;
        }
        else if (appState.capturePhase == 0) {
            graphics.DrawString(L"Press hotkeys to start", -1, &smallFont,
                PointF((REAL)x, (REAL)y), &lightGrayBrush);
            y += 18;
        }

        // Show validation error if present
        if (appState.distanceValidationFailed) {
            graphics.DrawString(L"⚠ DISTANCE MISMATCH!", -1, &smallFont,
                PointF((REAL)x, (REAL)y), &redBrush);
            y += 18;
        }

        // Show current phase
        if (appState.capturePhase == 1) {
            graphics.DrawString(L"→ Press direction key at 2nd point", -1, &smallFont,
                PointF((REAL)x, (REAL)y), &orangeBrush);
            y += 18;
        }
        else if (appState.capturePhase == 2 && !strongholdCandidates.empty()) {
            std::wstringstream ss;
            ss << L"Direction: " << std::fixed << std::setprecision(1) << appState.lastAngle << L"°";
            graphics.DrawString(ss.str().c_str(), -1, &infoFont,
                PointF((REAL)x, (REAL)y), &lightGrayBrush);
            y += 22;

            // Header for coordinates - more compact
            graphics.DrawString(L"Overworld      Nether      Prob", -1, &smallFont,
                PointF((REAL)x, (REAL)y), &lightGrayBrush);
            y += 16;

            // Show top stronghold locations (up to 6 for smaller window)
            int maxCandidates = std::min(6, (int)strongholdCandidates.size());
            for (int i = 0; i < maxCandidates; i++) {
                const auto& candidate = strongholdCandidates[i];

                // Color coding based on probability
                SolidBrush* brush;
                if (i == 0) {
                    brush = appState.distanceValidationFailed ? &redBrush : &greenBrush;
                }
                else if (candidate.conditionalProb > 0.1) {
                    brush = &yellowBrush;
                }
                else {
                    brush = &lightGrayBrush;
                }

                // Format coordinates more compactly
                std::wstringstream coordSS;
                coordSS << std::setw(5) << candidate.projectionX
                    << L"," << std::setw(5) << candidate.projectionZ;

                std::wstringstream netherSS;
                netherSS << std::setw(4) << candidate.netherX
                    << L"," << std::setw(4) << candidate.netherZ;

                std::wstringstream probSS;
                probSS << std::fixed << std::setprecision(0)
                    << (candidate.conditionalProb * 100.0) << L"%";

                // Draw overworld coordinates
                graphics.DrawString(coordSS.str().c_str(), -1, &coordFont,
                    PointF((REAL)x, (REAL)y), brush);

                // Draw nether coordinates
                graphics.DrawString(netherSS.str().c_str(), -1, &coordFont,
                    PointF((REAL)(x + 110), (REAL)y), &cyanBrush);

                // Draw probability
                graphics.DrawString(probSS.str().c_str(), -1, &coordFont,
                    PointF((REAL)(x + 200), (REAL)y), brush);

                // Show distance for top candidate
                if (i == 0) {
                    std::wstringstream distSS;
                    distSS << L" " << candidate.distance << L"m";
                    graphics.DrawString(distSS.str().c_str(), -1, &smallFont,
                        PointF((REAL)(x + 240), (REAL)y + 1), &lightGrayBrush);
                }

                y += 20; // Slightly more compact
            }

            y += 8;
            graphics.DrawString(L"Drag to move • Reset with hotkey", -1, &smallFont,
                PointF((REAL)x, (REAL)y), &lightGrayBrush);
        }
        else if (appState.capturePhase == 0 && appState.f4PressedFirst) {
            graphics.DrawString(L"→ Now press direction key twice", -1, &smallFont,
                PointF((REAL)x, (REAL)y), &orangeBrush);
        }

        EndPaint(hWnd, &ps);
    }
    break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
