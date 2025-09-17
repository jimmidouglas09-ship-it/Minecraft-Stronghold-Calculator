#define NOMINMAX
#include <windows.h>
#include "common.h"
#include "coordinate_reader.h"
#include "stronghold_calculator.h"
#include "distance_calculator.h"
#include "overlay_window.h"
#include "main_window.h"

// Global variables
HINSTANCE hInst;
WCHAR szTitle[] = L"Minecraft Coord Viewer";
WCHAR szWindowClass[] = L"MCViewerClass";
ULONG_PTR gdiplusToken;

// Application state
ApplicationState appState;

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPWSTR lpCmdLine,
    _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // Initialize GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // Initialize stronghold cells
    generateStrongholdCells();

    // Register window classes
    MyRegisterClass(hInstance);
    MyRegisterOverlayClass(hInstance);

    // Initialize windows
    if (!InitInstance(hInstance, nCmdShow)) return FALSE;
    if (!InitOverlay(hInstance)) return FALSE;

    // Message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);
    return (int)msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex = {};
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszClassName = szWindowClass;
    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow) {
    hInst = hInstance;
    HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME,
        CW_USEDEFAULT, 0, 500, 700, nullptr, nullptr, hInstance, nullptr);
    if (!hWnd) return FALSE;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    return TRUE;
}