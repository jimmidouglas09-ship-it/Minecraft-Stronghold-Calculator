#pragma once
#include "Settings.h"
#include <windows.h>
#include <string>

using namespace std;

class OptionsWindow {
public:
    static void showOptionsDialog(HWND parent);
    static void updateControls();

    // Legacy functions - kept for compatibility but not used
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);
    static void populateControls(HWND hDlg);
    static void saveFromControls(HWND hDlg);
    static string getKeyName(int vk);

private:
    static void createOptionsWindow(HWND parent);
};