#include <windows.h>
#include <cstdio>

// Keys to track and their delay times (ms)
const int keysToTrack[] = { 'T', VK_F9, 'A' };
const int delayTimes[] = { 5000, 30000, 60000 };
const int numKeys = sizeof(keysToTrack) / sizeof(keysToTrack[0]);

// Timer IDs
const int TIMER_ID_BASE = 100;
const int KEY_CHECK_TIMER_ID = 1;
const int POPUP_CLOSE_TIMER_ID = 2;

// Track state per key
bool keyTimerActive[numKeys] = { false };
bool keyWasDown[numKeys] = { false };
HWND hwndPopups[numKeys] = { nullptr };

HINSTANCE g_hInstance;

LRESULT CALLBACK PopupWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_TIMER:
            if (wParam == POPUP_CLOSE_TIMER_ID) {
                DestroyWindow(hwnd);
            }
            break;
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rc;
            GetClientRect(hwnd, &rc);
            const char* msg = (const char*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
            if (msg) {
                DrawTextA(hdc, msg, -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_DESTROY:
            SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)nullptr);
            break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_TIMER: {
            if (wParam == KEY_CHECK_TIMER_ID) {
                if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) {
                    PostQuitMessage(0);
                    return 0;
                }

                for (int i = 0; i < numKeys; i++) {
                    bool isDown = (GetAsyncKeyState(keysToTrack[i]) & 0x8000) != 0;

                    if (isDown && !keyWasDown[i] && !keyTimerActive[i]) {
                        SetTimer(hwnd, TIMER_ID_BASE + i, delayTimes[i], NULL);
                        keyTimerActive[i] = true;
                    } else if (!isDown && keyWasDown[i] && keyTimerActive[i]) {
                        KillTimer(hwnd, TIMER_ID_BASE + i);
                        keyTimerActive[i] = false;
                    }

                    keyWasDown[i] = isDown;
                }
                return 0;
            } else if (wParam >= TIMER_ID_BASE && wParam < TIMER_ID_BASE + numKeys) {
                int keyIndex = wParam - TIMER_ID_BASE;
                keyTimerActive[keyIndex] = false;

                // Destroy previous popup if still active
                if (hwndPopups[keyIndex]) {
                    DestroyWindow(hwndPopups[keyIndex]);
                    hwndPopups[keyIndex] = nullptr;
                }

                char* msg = new char[128];
                sprintf(msg, "Key '%c' held for %d seconds!", keysToTrack[keyIndex], delayTimes[keyIndex] / 1000);

                hwndPopups[keyIndex] = CreateWindowExA(
                    WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                    "PopupClass",
                    NULL,
                    WS_POPUP | WS_BORDER | WS_VISIBLE,
                    200, 200 + keyIndex * 100, 300, 80,
                    NULL, NULL, g_hInstance, NULL
                );

                if (hwndPopups[keyIndex]) {
                    SetWindowLongPtr(hwndPopups[keyIndex], GWLP_USERDATA, (LONG_PTR)msg);
                    ShowWindow(hwndPopups[keyIndex], SW_SHOWNOACTIVATE);
                    SetTimer(hwndPopups[keyIndex], POPUP_CLOSE_TIMER_ID, 2000, NULL);
                } else {
                    delete[] msg;
                }

                return 0;
            }
            break;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {
    g_hInstance = hInstance;

    WNDCLASSA mainClass = {};
    mainClass.lpfnWndProc = MainWndProc;
    mainClass.hInstance = hInstance;
    mainClass.lpszClassName = "MainWindowClass";
    RegisterClassA(&mainClass);

    WNDCLASSA popupClass = {};
    popupClass.lpfnWndProc = PopupWndProc;
    popupClass.hInstance = hInstance;
    popupClass.lpszClassName = "PopupClass";
    popupClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    RegisterClassA(&popupClass);

    HWND hwnd = CreateWindowExA(0, "MainWindowClass", "KeyTracker", 0,
        0, 0, 0, 0, NULL, NULL, hInstance, NULL);
    if (!hwnd) return 0;

    SetTimer(hwnd, KEY_CHECK_TIMER_ID, 100, NULL);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    // Cleanup popups
    for (int i = 0; i < numKeys; i++) {
        if (hwndPopups[i]) {
            DestroyWindow(hwndPopups[i]);
        }
    }

    return 0;
}
