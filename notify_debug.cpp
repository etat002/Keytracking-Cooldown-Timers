#include <windows.h>
#include <stdio.h>

const char POPUP_CLASS_NAME[] = "PopupNotification";
const char MAIN_CLASS_NAME[] = "MainWindowClass";

const int keysToTrack[] = { 'T' };
const int delays[] = { 3000 }; // 3 seconds for quicker test
const int numKeys = sizeof(keysToTrack) / sizeof(keysToTrack[0]);

const int TIMER_INTERVAL_MS = 100;
const int TIMER_BASE_DELAY = 100;
const int TIMER_POPUP_CLOSE = 9999;

bool keyWasDown[numKeys] = { false };
bool waitingForPopup[numKeys] = { false };
HWND hwndPopups[numKeys] = { nullptr };
HINSTANCE g_hInstance = nullptr;

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_TIMER: {
            if (wParam == 1) {
                OutputDebugStringA("Polling keys...\n");
                for (int i = 0; i < numKeys; ++i) {
                    bool isDown = (GetAsyncKeyState(keysToTrack[i]) & 0x8000) != 0;
                    char sb[100];
                    snprintf(sb, sizeof(sb), "Key %c isDown=%d wasDown=%d waiting=%d\n",
                             keysToTrack[i], isDown, keyWasDown[i], waitingForPopup[i]);
                    OutputDebugStringA(sb);

                    if (isDown && !keyWasDown[i] && !waitingForPopup[i]) {
                        snprintf(sb, sizeof(sb), "Starting delay timer for %c\n", keysToTrack[i]);
                        OutputDebugStringA(sb);
                        SetTimer(hwnd, TIMER_BASE_DELAY + i, delays[i], NULL);
                        waitingForPopup[i] = true;
                    }
                    else if (!isDown && keyWasDown[i] && waitingForPopup[i]) {
                        OutputDebugStringA("Key released early, cancelling timer\n");
                        KillTimer(hwnd, TIMER_BASE_DELAY + i);
                        waitingForPopup[i] = false;
                    }
                    keyWasDown[i] = isDown;
                }
            }
            else if (wParam >= TIMER_BASE_DELAY && wParam < TIMER_BASE_DELAY + numKeys) {
                int index = wParam - TIMER_BASE_DELAY;
                waitingForPopup[index] = false;
                char sb[100];
                snprintf(sb, sizeof(sb), "Delay timer fired for key %c\n", keysToTrack[index]);
                OutputDebugStringA(sb);

                if (hwndPopups[index] && IsWindow(hwndPopups[index])) {
                    DestroyWindow(hwndPopups[index]);
                }

                char title[100];
                snprintf(title, sizeof(title), "Key '%c' Held", keysToTrack[index]);
                hwndPopups[index] = CreateWindowExA(
                    WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                    POPUP_CLASS_NAME, title,
                    WS_POPUP | WS_BORDER | WS_VISIBLE,
                    200, 200, 300, 80,
                    NULL, NULL, g_hInstance, NULL);
                if (!hwndPopups[index]) {
                    MessageBoxA(NULL, "Popup create failed", "Error", MB_OK);
                } else {
                    ShowWindow(hwndPopups[index], SW_SHOWNOACTIVATE);
                    SetTimer(hwndPopups[index], TIMER_POPUP_CLOSE, 2000, NULL);
                }
            }
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcA(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK PopupWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch(uMsg) {
        case WM_TIMER:
            if (wParam == TIMER_POPUP_CLOSE) {
                DestroyWindow(hwnd);
            }
            return 0;
        default:
            return DefWindowProcA(hwnd, uMsg, wParam, lParam);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    g_hInstance = hInstance;
    WNDCLASSA wc = {};
    wc.lpfnWndProc = MainWindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = MAIN_CLASS_NAME;
    if (!RegisterClassA(&wc)) {
        MessageBoxA(NULL, "RegisterClassA failed", "Error", MB_OK);
        return 1;
    }
    WNDCLASSA wcp = {};
    wcp.lpfnWndProc = PopupWindowProc;
    wcp.hInstance = hInstance;
    wcp.lpszClassName = POPUP_CLASS_NAME;
    wcp.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    if (!RegisterClassA(&wcp)) {
        MessageBoxA(NULL, "RegisterClassA popup failed", "Error", MB_OK);
        return 1;
    }
    HWND hm = CreateWindowExA(0, MAIN_CLASS_NAME, "Hidden", 0, 0,0,0,0, NULL, NULL, hInstance, NULL);
    if (!hm) {
        MessageBoxA(NULL, "CreateWindowExA failed", "Error", MB_OK);
        return 1;
    }
    SetTimer(hm, 1, TIMER_INTERVAL_MS, NULL);
    MSG msg = {};
    while (GetMessageA(&msg, NULL, 0,0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    return 0;
}
