#include "Settings.h"
#include <fstream>
#include <shlobj.h>

HotkeySettings Settings::hotkeys;

void Settings::initialize() {
    loadSettings();
}

string Settings::getSettingsFilePath() {
    char appDataPath[MAX_PATH];
    if (SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appDataPath) == S_OK) {
        string settingsDir = string(appDataPath) + "\\StrongholdCalculator";
        CreateDirectoryA(settingsDir.c_str(), NULL);
        return settingsDir + "\\settings.ini";
    }
    return "settings.ini"; // Fallback to current directory
}

void Settings::loadSettings() {
    string settingsFile = getSettingsFilePath();
    ifstream file(settingsFile);

    if (!file.is_open()) {
        // File doesn't exist, use defaults
        return;
    }

    string line;
    while (getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        size_t equalPos = line.find('=');
        if (equalPos == string::npos) continue;

        string key = line.substr(0, equalPos);
        string value = line.substr(equalPos + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        if (key == "calibrateKey") hotkeys.calibrateKey = stringToVirtualKey(value);
        else if (key == "trackAngleKey") hotkeys.trackAngleKey = stringToVirtualKey(value);
        else if (key == "calculateKey") hotkeys.calculateKey = stringToVirtualKey(value);
        else if (key == "resetKey") hotkeys.resetKey = stringToVirtualKey(value);
        else if (key == "optionsKey") hotkeys.optionsKey = stringToVirtualKey(value);
        else if (key == "secondThrowKey") hotkeys.secondThrowKey = stringToVirtualKey(value);
    }

    file.close();
}

void Settings::saveSettings() {
    string settingsFile = getSettingsFilePath();
    ofstream file(settingsFile);

    if (!file.is_open()) return;

    file << "# Stronghold Calculator Settings\n";
    file << "# Hotkey assignments (use VK_ constants or key names)\n\n";
    file << "calibrateKey=" << virtualKeyToString(hotkeys.calibrateKey) << "\n";
    file << "trackAngleKey=" << virtualKeyToString(hotkeys.trackAngleKey) << "\n";
    file << "calculateKey=" << virtualKeyToString(hotkeys.calculateKey) << "\n";
    file << "resetKey=" << virtualKeyToString(hotkeys.resetKey) << "\n";
    file << "optionsKey=" << virtualKeyToString(hotkeys.optionsKey) << "\n";
    file << "secondThrowKey=" << virtualKeyToString(hotkeys.secondThrowKey) << "\n";

    file.close();
}

string Settings::virtualKeyToString(int vk) {
    switch (vk) {
        // Function keys
    case VK_F1: return "F1"; case VK_F2: return "F2"; case VK_F3: return "F3"; case VK_F4: return "F4";
    case VK_F5: return "F5"; case VK_F6: return "F6"; case VK_F7: return "F7"; case VK_F8: return "F8";
    case VK_F9: return "F9"; case VK_F10: return "F10"; case VK_F11: return "F11"; case VK_F12: return "F12";

        // Numbers
    case '0': return "0"; case '1': return "1"; case '2': return "2"; case '3': return "3"; case '4': return "4";
    case '5': return "5"; case '6': return "6"; case '7': return "7"; case '8': return "8"; case '9': return "9";

        // Letters
    case 'A': return "A"; case 'B': return "B"; case 'C': return "C"; case 'D': return "D"; case 'E': return "E";
    case 'F': return "F"; case 'G': return "G"; case 'H': return "H"; case 'I': return "I"; case 'J': return "J";
    case 'K': return "K"; case 'L': return "L"; case 'M': return "M"; case 'N': return "N"; case 'O': return "O";
    case 'P': return "P"; case 'Q': return "Q"; case 'R': return "R"; case 'S': return "S"; case 'T': return "T";
    case 'U': return "U"; case 'V': return "V"; case 'W': return "W"; case 'X': return "X"; case 'Y': return "Y"; case 'Z': return "Z";

        // Navigation
    case VK_HOME: return "HOME"; case VK_END: return "END"; case VK_PRIOR: return "PAGE UP"; case VK_NEXT: return "PAGE DOWN";
    case VK_INSERT: return "INSERT"; case VK_DELETE: return "DELETE";
    case VK_UP: return "UP ARROW"; case VK_DOWN: return "DOWN ARROW"; case VK_LEFT: return "LEFT ARROW"; case VK_RIGHT: return "RIGHT ARROW";

        // Special keys
    case VK_SPACE: return "SPACE"; case VK_TAB: return "TAB"; case VK_RETURN: return "ENTER"; case VK_BACK: return "BACKSPACE";
    case VK_ESCAPE: return "ESCAPE"; case VK_SHIFT: return "SHIFT"; case VK_CONTROL: return "CTRL"; case VK_MENU: return "ALT";

        // Numpad
    case VK_NUMPAD0: return "NUM 0"; case VK_NUMPAD1: return "NUM 1"; case VK_NUMPAD2: return "NUM 2"; case VK_NUMPAD3: return "NUM 3";
    case VK_NUMPAD4: return "NUM 4"; case VK_NUMPAD5: return "NUM 5"; case VK_NUMPAD6: return "NUM 6"; case VK_NUMPAD7: return "NUM 7";
    case VK_NUMPAD8: return "NUM 8"; case VK_NUMPAD9: return "NUM 9";
    case VK_MULTIPLY: return "NUM *"; case VK_ADD: return "NUM +"; case VK_SUBTRACT: return "NUM -"; case VK_DIVIDE: return "NUM /";
    case VK_DECIMAL: return "NUM ."; case VK_NUMLOCK: return "NUM LOCK";

        // Punctuation
    case VK_OEM_1: return ";"; case VK_OEM_PLUS: return "="; case VK_OEM_COMMA: return ","; case VK_OEM_MINUS: return "-";
    case VK_OEM_PERIOD: return "."; case VK_OEM_2: return "/"; case VK_OEM_3: return "`"; case VK_OEM_4: return "[";
    case VK_OEM_5: return "\\"; case VK_OEM_6: return "]"; case VK_OEM_7: return "'";

    default: return "F6"; // Default fallback
    }
}

int Settings::stringToVirtualKey(const string& str) {
    // Function keys
    if (str == "F1") return VK_F1; if (str == "F2") return VK_F2; if (str == "F3") return VK_F3; if (str == "F4") return VK_F4;
    if (str == "F5") return VK_F5; if (str == "F6") return VK_F6; if (str == "F7") return VK_F7; if (str == "F8") return VK_F8;
    if (str == "F9") return VK_F9; if (str == "F10") return VK_F10; if (str == "F11") return VK_F11; if (str == "F12") return VK_F12;

    // Numbers
    if (str == "0") return '0'; if (str == "1") return '1'; if (str == "2") return '2'; if (str == "3") return '3'; if (str == "4") return '4';
    if (str == "5") return '5'; if (str == "6") return '6'; if (str == "7") return '7'; if (str == "8") return '8'; if (str == "9") return '9';

    // Letters
    if (str == "A") return 'A'; if (str == "B") return 'B'; if (str == "C") return 'C'; if (str == "D") return 'D'; if (str == "E") return 'E';
    if (str == "F") return 'F'; if (str == "G") return 'G'; if (str == "H") return 'H'; if (str == "I") return 'I'; if (str == "J") return 'J';
    if (str == "K") return 'K'; if (str == "L") return 'L'; if (str == "M") return 'M'; if (str == "N") return 'N'; if (str == "O") return 'O';
    if (str == "P") return 'P'; if (str == "Q") return 'Q'; if (str == "R") return 'R'; if (str == "S") return 'S'; if (str == "T") return 'T';
    if (str == "U") return 'U'; if (str == "V") return 'V'; if (str == "W") return 'W'; if (str == "X") return 'X'; if (str == "Y") return 'Y'; if (str == "Z") return 'Z';

    // Navigation
    if (str == "HOME") return VK_HOME; if (str == "END") return VK_END; if (str == "PAGE UP") return VK_PRIOR; if (str == "PAGE DOWN") return VK_NEXT;
    if (str == "INSERT") return VK_INSERT; if (str == "DELETE") return VK_DELETE;
    if (str == "UP ARROW") return VK_UP; if (str == "DOWN ARROW") return VK_DOWN; if (str == "LEFT ARROW") return VK_LEFT; if (str == "RIGHT ARROW") return VK_RIGHT;

    // Special keys
    if (str == "SPACE") return VK_SPACE; if (str == "TAB") return VK_TAB; if (str == "ENTER") return VK_RETURN; if (str == "BACKSPACE") return VK_BACK;
    if (str == "ESCAPE") return VK_ESCAPE; if (str == "SHIFT") return VK_SHIFT; if (str == "CTRL") return VK_CONTROL; if (str == "ALT") return VK_MENU;

    // Numpad
    if (str == "NUM 0") return VK_NUMPAD0; if (str == "NUM 1") return VK_NUMPAD1; if (str == "NUM 2") return VK_NUMPAD2; if (str == "NUM 3") return VK_NUMPAD3;
    if (str == "NUM 4") return VK_NUMPAD4; if (str == "NUM 5") return VK_NUMPAD5; if (str == "NUM 6") return VK_NUMPAD6; if (str == "NUM 7") return VK_NUMPAD7;
    if (str == "NUM 8") return VK_NUMPAD8; if (str == "NUM 9") return VK_NUMPAD9;
    if (str == "NUM *") return VK_MULTIPLY; if (str == "NUM +") return VK_ADD; if (str == "NUM -") return VK_SUBTRACT; if (str == "NUM /") return VK_DIVIDE;
    if (str == "NUM .") return VK_DECIMAL; if (str == "NUM LOCK") return VK_NUMLOCK;

    // Punctuation
    if (str == ";") return VK_OEM_1; if (str == "=") return VK_OEM_PLUS; if (str == ",") return VK_OEM_COMMA; if (str == "-") return VK_OEM_MINUS;
    if (str == ".") return VK_OEM_PERIOD; if (str == "/") return VK_OEM_2; if (str == "`") return VK_OEM_3; if (str == "[") return VK_OEM_4;
    if (str == "\\") return VK_OEM_5; if (str == "]") return VK_OEM_6; if (str == "'") return VK_OEM_7;

    // Legacy compatibility
    if (str == "PAGEUP") return VK_PRIOR; if (str == "PAGEDOWN") return VK_NEXT;

    return VK_F6; // Default fallback
}