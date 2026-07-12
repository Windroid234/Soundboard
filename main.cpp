#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include "SoundManager.h"

#pragma comment(lib, "comctl32.lib")

SoundManager manager;
std::vector<HWND> buttons;
HWND hwndAddButton;
HWND hwndStopButton;
HWND hwndEditMode;
HWND hwndPassThroughMode;
HWND hwndVolume;
HWND hwndLocalVolume;
HWND hwndMicLabel;
HWND hwndLocalLabel;
HWND hwndDeviceCombo;

bool editMode = false;
std::vector<AudioDeviceInfo> deviceList;

#include <shellapi.h>
#define WM_TRAYICON (WM_USER + 1)
NOTIFYICONDATAW nid = {};

std::wstring GetHotkeyString(UINT modifiers, UINT vk) {
    if (vk == 0) return L"";
    std::wstring str = L"\n[";
    if (modifiers & HOTKEYF_CONTROL) str += L"CTRL + ";
    if (modifiers & HOTKEYF_SHIFT) str += L"SHIFT + ";
    if (modifiers & HOTKEYF_ALT) str += L"ALT + ";
    
    wchar_t name[64];
    if (GetKeyNameTextW(MapVirtualKeyW(vk, MAPVK_VK_TO_VSC) << 16, name, 64) > 0) {
        str += name;
    } else {
        str += std::to_wstring(vk);
    }
    str += L"]";
    return str;
}

bool passThrough = false;

void SaveCurrentState(HWND hwnd) {
    std::wstring deviceName = L"";
    LRESULT idx = SendMessage(hwndDeviceCombo, CB_GETCURSEL, 0, 0);
    if (idx > 0 && (size_t)(idx - 1) < deviceList.size()) {
        deviceName = deviceList[idx - 1].name;
    }
    LRESULT vol = SendMessage(hwndVolume, TBM_GETPOS, 0, 0);
    LRESULT localVol = SendMessage(hwndLocalVolume, TBM_GETPOS, 0, 0);
    manager.SetConfigState((float)vol / 100.0f, (float)localVol / 100.0f, passThrough, deviceName);
}

HHOOK hhkLowLevelKbd = NULL;
bool isKeyDown[256] = { false };

LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION) {
        KBDLLHOOKSTRUCT* p = (KBDLLHOOKSTRUCT*)lParam;
        
        if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            if (p->vkCode < 256) isKeyDown[p->vkCode] = false;
        } else if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            UINT currentModifiers = 0;
            if (GetAsyncKeyState(VK_MENU) & 0x8000) currentModifiers |= HOTKEYF_ALT;
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000) currentModifiers |= HOTKEYF_CONTROL;
            if (GetAsyncKeyState(VK_SHIFT) & 0x8000) currentModifiers |= HOTKEYF_SHIFT;
            
            const auto& sounds = manager.GetSounds();
            for (size_t i = 0; i < sounds.size(); ++i) {
                if (sounds[i]->vk != 0 && sounds[i]->vk == p->vkCode && sounds[i]->modifiers == currentModifiers) {
                    if (p->vkCode < 256 && !isKeyDown[p->vkCode]) {
                        isKeyDown[p->vkCode] = true;
                        manager.ToggleSoundItem(i);
                    }
                    if (!passThrough) {
                        return 1; // swallow
                    }
                }
            }
        }
    }
    return CallNextHookEx(hhkLowLevelKbd, nCode, wParam, lParam);
}

void RebuildUI(HWND hwnd);
void LayoutUI(HWND hwnd);

size_t currentEditIndex = 0;

LRESULT CALLBACK HotkeyPopupProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_COMMAND:
        if (LOWORD(wParam) == 1) { 
            HWND hHotkey = GetDlgItem(hwnd, 1001);
            LRESULT res = SendMessage(hHotkey, HKM_GETHOTKEY, 0, 0);
            manager.SetHotkey(currentEditIndex, HIBYTE(res), LOBYTE(res));
            RebuildUI(GetParent(hwnd));
            LayoutUI(GetParent(hwnd));
            DestroyWindow(hwnd);
        } else if (LOWORD(wParam) == 2) { 
            manager.SetHotkey(currentEditIndex, 0, 0);
            RebuildUI(GetParent(hwnd));
            LayoutUI(GetParent(hwnd));
            DestroyWindow(hwnd);
        }
        break;
    case WM_CLOSE:
        DestroyWindow(hwnd);
        break;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

void OpenHotkeyWindow(HWND parent, size_t index) {
    currentEditIndex = index;
    
    WNDCLASSW wc = {0};
    wc.lpfnWndProc = HotkeyPopupProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = L"HotkeyPopupClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    RegisterClassW(&wc);
    
    HWND hwnd = CreateWindowExW(WS_EX_DLGMODALFRAME, L"HotkeyPopupClass", L"Set Hotkey",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT, 250, 150, parent, NULL, GetModuleHandle(NULL), NULL);
        
    HWND hHotkey = CreateWindowExW(0, HOTKEY_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP,
        20, 20, 190, 25, hwnd, (HMENU)1001, GetModuleHandle(NULL), NULL);
        
    HWND hOk = CreateWindowW(L"BUTTON", L"Save",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        20, 60, 80, 30, hwnd, (HMENU)1, GetModuleHandle(NULL), NULL);
        
    HWND hClear = CreateWindowW(L"BUTTON", L"Clear",
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        130, 60, 80, 30, hwnd, (HMENU)2, GetModuleHandle(NULL), NULL);
        
    const auto& s = manager.GetSounds()[currentEditIndex];
    SendMessage(hHotkey, HKM_SETHOTKEY, MAKEWORD(s->vk, s->modifiers), 0);
    
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    SendMessage(hHotkey, WM_SETFONT, (WPARAM)hFont, 0);
    SendMessage(hOk, WM_SETFONT, (WPARAM)hFont, 0);
    SendMessage(hClear, WM_SETFONT, (WPARAM)hFont, 0);
}

void RebuildUI(HWND hwnd) {
    for (HWND btn : buttons) {
        DestroyWindow(btn);
    }
    buttons.clear();

    const auto& sounds = manager.GetSounds();
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    for (size_t i = 0; i < sounds.size(); ++i) {
        std::wstring text = sounds[i]->name;
        if (sounds[i]->vk != 0) {
            text += GetHotkeyString(sounds[i]->modifiers, sounds[i]->vk);
        }
        HWND btn = CreateWindowW(L"BUTTON", text.c_str(),
            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_MULTILINE,
            0, 0, 0, 0, hwnd, (HMENU)(UINT_PTR)(100 + i),
            (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), NULL);
        
        SendMessage(btn, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
        buttons.push_back(btn);
    }
}

void LayoutUI(HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    int bottomBarHeight = 110;
    int gridHeight = height - bottomBarHeight;
    if (gridHeight < 0) gridHeight = 0;

    if (hwndMicLabel) MoveWindow(hwndMicLabel, 10, height - 100, 60, 20, TRUE);
    if (hwndVolume) MoveWindow(hwndVolume, 70, height - 100, 140, 30, TRUE);
    if (hwndDeviceCombo) MoveWindow(hwndDeviceCombo, 220, height - 100, 300, 200, TRUE);

    if (hwndLocalLabel) MoveWindow(hwndLocalLabel, 10, height - 65, 60, 20, TRUE);
    if (hwndLocalVolume) MoveWindow(hwndLocalVolume, 70, height - 65, 140, 30, TRUE);

    if (hwndAddButton) MoveWindow(hwndAddButton, 10, height - 35, 100, 25, TRUE);
    if (hwndStopButton) MoveWindow(hwndStopButton, 120, height - 35, 100, 25, TRUE);
    if (hwndEditMode) MoveWindow(hwndEditMode, 230, height - 35, 150, 25, TRUE);
    if (hwndPassThroughMode) MoveWindow(hwndPassThroughMode, 390, height - 35, 160, 25, TRUE);

    if (buttons.empty()) return;

    int count = static_cast<int>(buttons.size());
    int cols = 1;
    while (cols * cols < count) cols++;
    int rows = (count + cols - 1) / cols;

    int btnWidth = width / cols;
    int btnHeight = gridHeight / rows;

    for (int i = 0; i < count; ++i) {
        int r = i / cols;
        int c = i % cols;
        MoveWindow(buttons[i], c * btnWidth, r * btnHeight, btnWidth, btnHeight, TRUE);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            INITCOMMONCONTROLSEX icex;
            icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
            icex.dwICC = ICC_BAR_CLASSES | ICC_HOTKEY_CLASS;
            InitCommonControlsEx(&icex);

            manager.Initialize();
            manager.LoadFromDirectory(manager.GetSoundsDir());
            passThrough = manager.GetSavedPassThrough();
            manager.SetVolume(manager.GetSavedMicVol());
            manager.SetLocalVolume(manager.GetSavedLocalVol());

            hwndMicLabel = CreateWindowW(L"STATIC", L"MIC VOL:", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            hwndLocalLabel = CreateWindowW(L"STATIC", L"MY VOL:", WS_CHILD | WS_VISIBLE, 0, 0, 0, 0, hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL);

            hwndVolume = CreateWindowW(TRACKBAR_CLASSW, L"Trackbar Control",
                WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
                0, 0, 0, 0, hwnd, (HMENU)10, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            SendMessage(hwndVolume, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
            SendMessage(hwndVolume, TBM_SETPOS, TRUE, (LRESULT)(manager.GetSavedMicVol() * 100));

            hwndLocalVolume = CreateWindowW(TRACKBAR_CLASSW, L"Trackbar Control",
                WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS,
                0, 0, 0, 0, hwnd, (HMENU)12, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            SendMessage(hwndLocalVolume, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));
            SendMessage(hwndLocalVolume, TBM_SETPOS, TRUE, (LRESULT)(manager.GetSavedLocalVol() * 100));

            hwndDeviceCombo = CreateWindowW(WC_COMBOBOXW, L"",
                CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
                0, 0, 0, 0, hwnd, (HMENU)11, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            
            deviceList = manager.GetPlaybackDevices();
            SendMessage(hwndDeviceCombo, CB_ADDSTRING, 0, (LPARAM)L"Default System Device");
            int selectedIdx = 0;
            for (size_t i = 0; i < deviceList.size(); ++i) {
                SendMessage(hwndDeviceCombo, CB_ADDSTRING, 0, (LPARAM)deviceList[i].name.c_str());
                if (deviceList[i].name == manager.GetSavedDeviceName()) {
                    selectedIdx = (int)i + 1;
                }
            }
            SendMessage(hwndDeviceCombo, CB_SETCURSEL, selectedIdx, 0);
            if (selectedIdx > 0) {
                manager.ChangeDevice(&deviceList[selectedIdx - 1].id);
            }

            hwndAddButton = CreateWindowW(L"BUTTON", L"ADD SOUND",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                0, 0, 0, 0, hwnd, (HMENU)1, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            hwndStopButton = CreateWindowW(L"BUTTON", L"STOP ALL",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                0, 0, 0, 0, hwnd, (HMENU)2, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            hwndEditMode = CreateWindowW(L"BUTTON", L"EDIT HOTKEYS: OFF",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                0, 0, 0, 0, hwnd, (HMENU)3, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
            hwndPassThroughMode = CreateWindowW(L"BUTTON", passThrough ? L"PASS THROUGH: ON" : L"PASS THROUGH: OFF",
                WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
                0, 0, 0, 0, hwnd, (HMENU)4, ((LPCREATESTRUCT)lParam)->hInstance, NULL);
                
            HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
            SendMessage(hwndDeviceCombo, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
            SendMessage(hwndMicLabel, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
            SendMessage(hwndLocalLabel, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
            SendMessage(hwndAddButton, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
            SendMessage(hwndStopButton, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
            SendMessage(hwndEditMode, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
            SendMessage(hwndPassThroughMode, WM_SETFONT, (WPARAM)hFont, MAKELPARAM(TRUE, 0));
            
            hhkLowLevelKbd = SetWindowsHookExW(WH_KEYBOARD_LL, LowLevelKeyboardProc, GetModuleHandle(NULL), 0);
            
            nid.cbSize = sizeof(nid);
            nid.hWnd = hwnd;
            nid.uID = 1001;
            nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
            nid.uCallbackMessage = WM_TRAYICON;
            nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
            wcscpy_s(nid.szTip, L"Soundboard");
            
            RebuildUI(hwnd);
            break;
        }
        case WM_SIZE:
            if (wParam == SIZE_MINIMIZED) {
                Shell_NotifyIconW(NIM_ADD, &nid);
                ShowWindow(hwnd, SW_HIDE);
            } else {
                LayoutUI(hwnd);
            }
            break;
        case WM_TRAYICON:
            if (lParam == WM_LBUTTONUP || lParam == WM_LBUTTONDBLCLK) {
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);
                Shell_NotifyIconW(NIM_DELETE, &nid);
            }
            break;
        case WM_HSCROLL:
            if (LOWORD(wParam) == TB_ENDTRACK) {
                SaveCurrentState(hwnd);
            } else {
                if ((HWND)lParam == hwndVolume) {
                    LRESULT pos = SendMessage(hwndVolume, TBM_GETPOS, 0, 0);
                    manager.SetVolume((float)pos / 100.0f);
                } else if ((HWND)lParam == hwndLocalVolume) {
                    LRESULT pos = SendMessage(hwndLocalVolume, TBM_GETPOS, 0, 0);
                    manager.SetLocalVolume((float)pos / 100.0f);
                }
            }
            break;
        case WM_COMMAND:
            if (HIWORD(wParam) == CBN_SELCHANGE && (HWND)lParam == hwndDeviceCombo) {
                LRESULT idx = SendMessage(hwndDeviceCombo, CB_GETCURSEL, 0, 0);
                if (idx == 0) {
                    manager.ChangeDevice(nullptr);
                } else if (idx > 0 && (size_t)(idx - 1) < deviceList.size()) {
                    manager.ChangeDevice(&deviceList[idx - 1].id);
                }
            } else if (LOWORD(wParam) == 1) {
                wchar_t filename[MAX_PATH] = { 0 };
                OPENFILENAMEW ofn = { 0 };
                ofn.lStructSize = sizeof(ofn);
                ofn.hwndOwner = hwnd;
                ofn.lpstrFilter = L"Audio Files\0*.wav;*.mp3;*.wma;*.ogg;*.flac\0All Files\0*.*\0";
                ofn.lpstrFile = filename;
                ofn.nMaxFile = MAX_PATH;
                ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
                if (GetOpenFileNameW(&ofn)) {
                    manager.AddSound(filename);
                    RebuildUI(hwnd);
                    LayoutUI(hwnd);
                }
            } else if (LOWORD(wParam) == 2) {
                manager.StopAll();
            } else if (LOWORD(wParam) == 3) {
                editMode = !editMode;
                SetWindowTextW(hwndEditMode, editMode ? L"EDIT HOTKEYS: ON" : L"EDIT HOTKEYS: OFF");
            } else if (LOWORD(wParam) == 4) {
                passThrough = !passThrough;
                SetWindowTextW(hwndPassThroughMode, passThrough ? L"PASS THROUGH: ON" : L"PASS THROUGH: OFF");
                SaveCurrentState(hwnd);
            } else if (LOWORD(wParam) >= 100) {
                size_t index = LOWORD(wParam) - 100;
                if (editMode) {
                    OpenHotkeyWindow(hwnd, index);
                } else {
                    manager.ToggleSoundItem(index);
                }
            } else if (HIWORD(wParam) == CBN_SELCHANGE && LOWORD(wParam) == 11) {
                LRESULT idx = SendMessage(hwndDeviceCombo, CB_GETCURSEL, 0, 0);
                if (idx == 0) {
                    manager.ChangeDevice(nullptr);
                } else if (idx > 0 && (size_t)(idx - 1) < deviceList.size()) {
                    manager.ChangeDevice(&deviceList[idx - 1].id);
                }
                SaveCurrentState(hwnd);
            }
            break;
        case WM_DESTROY:
            UnhookWindowsHookEx(hhkLowLevelKbd);
            PostQuitMessage(0);
            break;
        default:
            return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    WNDCLASSW wc = { 0 };
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"SoundboardClass";
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowW(L"SoundboardClass", L"SOUNDBOARD",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);

    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
