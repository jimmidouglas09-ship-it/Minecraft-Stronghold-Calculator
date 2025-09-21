#include "Common.h"
#include "WindowProc.h"

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// Constants definition
const int WINDOW_WIDTH = 400;
const int WINDOW_HEIGHT = 550;
const double M_PI = 3.14159265358979323846;

// Global variables definition
AppState appState;
map<int, double> distanceProbabilities;
vector<StrongholdCell> strongholdCells;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Register window class
    const wchar_t CLASS_NAME[] = L"StrongholdCalculator";

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc::WindowProcedure;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClass(&wc);

    // Get screen dimensions for positioning
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Position window on the right side of screen
    int xPos = screenWidth - WINDOW_WIDTH - 20;
    int yPos = (screenHeight - WINDOW_HEIGHT) / 2;

    // Create window
    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        CLASS_NAME,
        L"Stronghold Calculator",
        WS_POPUP | WS_VISIBLE,
        xPos, yPos,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd == NULL) {
        GdiplusShutdown(gdiplusToken);
        return 0;
    }

    // Set window transparency
    SetLayeredWindowAttributes(hwnd, 0, 245, LWA_ALPHA);

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop
    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup
    GdiplusShutdown(gdiplusToken);
    return 0;
}