#pragma once
#include "Common.h"

struct HotkeySettings {
    int calibrateKey = VK_F6;
    int trackAngleKey = VK_F7;
    int calculateKey = VK_F8;
    int resetKey = VK_F9;
    int optionsKey = VK_F10;
    int secondThrowKey = VK_F11;
};

class Settings {
public:
    static void initialize();
    static void loadSettings();
    static void saveSettings();
    static string virtualKeyToString(int vk);  // Move this to public

    static HotkeySettings hotkeys;

private:
    static string getSettingsFilePath();
    static int stringToVirtualKey(const string& str);  // Keep this private
};
