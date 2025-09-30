#define UNICODE
#define _UNICODE
#include <windows.h>

// Global variables
const int TIMER_ID_KEYCHECK = 1;
const int TIMER_ID_DELAY = 2;
const int TIMER_ID_POPUP_CLOSE = 3;
const int TIMER_INTERVAL_MS = 100;  // .1 seconds
const int DELAY_MS = 5000;	    // Delay before showing popup (5 seconds)
const int TARGET_KEY = 'T';         // Key to detect (T)

// Forward declarations
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK PopupWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// Popup class name
const wchar_t POPUP_CLASS_NAME[] = L"PopupNotification";

int main() {
    HINSTANCE hInstance = GetModuleHandle(nullptr);
    // Register main window class
    const wchar_t MAIN_CLASS_NAME[] = L"MainWindowClass";

    WNDCLASSW wcMain = {};
    wcMain.lpfnWndProc = MainWindowProc;
    wcMain.hInstance = hInstance;
    wcMain.lpszClassName = MAIN_CLASS_NAME;

    if (!RegisterClassW(&wcMain)) {
        MessageBoxW(nullptr, L"Failed to register main window class.", L"Error", MB_ICONERROR);
        return -1;
    }

    // Register popup window class
    WNDCLASSW wcPopup = {};
    wcPopup.lpfnWndProc = PopupWindowProc;
    wcPopup.hInstance = hInstance;
    wcPopup.lpszClassName = POPUP_CLASS_NAME;
    wcPopup.hbrBackground = (HBRUSH)(COLOR_INFOBK + 1); // light background for popup

    if (!RegisterClassW(&wcPopup)) {
        MessageBoxW(nullptr, L"Failed to register popup window class.", L"Error", MB_ICONERROR);
        return -1;
    }

    // Create main (hidden) window
    HWND hwndMain = CreateWindowExW(
        0,
        MAIN_CLASS_NAME,
        L"Main Hidden Window",
        0, 0, 0, 0, 0,
        nullptr, nullptr, hInstance, nullptr);

    if (!hwndMain) {
        MessageBoxW(nullptr, L"Failed to create main window.", L"Error", MB_ICONERROR);
        return -1;
    }

    // Run message loop
    MSG msg = {};
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}

// Main window procedure
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    bool waitingForPopup = false;  // Is the delay timer running?
    bool wasKeyDownLastCheck = false;
    static HWND hwndPopup = nullptr;
    static HINSTANCE hInstance = nullptr;

    switch (uMsg) {
    case WM_CREATE:
        hInstance = ((LPCREATESTRUCT)lParam)->hInstance;
        // Start a timer that fires every 5 seconds
        SetTimer(hwnd, TIMER_ID_KEYCHECK, TIMER_INTERVAL_MS, nullptr);
        return 0;

case WM_TIMER: {
    if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0) {
        PostQuitMessage(0);
        return 0;
    }

    if (wParam == TIMER_ID_KEYCHECK) {
        bool isKeyDownNow = (GetAsyncKeyState(TARGET_KEY) & 0x8000) != 0;

        // Start delay timer when key is first pressed
        if (isKeyDownNow && !wasKeyDownLastCheck && !waitingForPopup) {
            SetTimer(hwnd, TIMER_ID_DELAY, DELAY_MS, nullptr);
            waitingForPopup = true;
        }

        // If key released before delay ends, cancel popup
        if (!isKeyDownNow && wasKeyDownLastCheck && waitingForPopup) {
            KillTimer(hwnd, TIMER_ID_DELAY);
            waitingForPopup = false;
        }

        wasKeyDownLastCheck = isKeyDownNow;
    }
    else if (wParam == TIMER_ID_DELAY) {
        // Time to show the popup!
        if (hwndPopup == nullptr || !IsWindow(hwndPopup)) {
            hwndPopup = CreateWindowExW(
                WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                POPUP_CLASS_NAME,
                L"Key Press Notification",
                WS_POPUP | WS_BORDER | WS_VISIBLE,
                100, 100, 350, 80,
                nullptr, nullptr, hInstance, nullptr);

            if (hwndPopup) {
                ShowWindow(hwndPopup, SW_SHOWNOACTIVATE);
                SetTimer(hwndPopup, TIMER_ID_POPUP_CLOSE, 2000, nullptr);
            }
        }
        waitingForPopup = false;
        KillTimer(hwnd, TIMER_ID_DELAY);
    }
    else if (wParam == TIMER_ID_POPUP_CLOSE) {
        DestroyWindow(hwndPopup);
        hwndPopup = nullptr;
    }
    return 0;
}
   

    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID_KEYCHECK);
        PostQuitMessage(0);
        return 0;

    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}

// Popup window procedure
LRESULT CALLBACK PopupWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_TIMER:
        if (wParam == TIMER_ID_POPUP_CLOSE) {
            KillTimer(hwnd, TIMER_ID_POPUP_CLOSE);
            DestroyWindow(hwnd);
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rect;
        GetClientRect(hwnd, &rect);

        // Fill background (optional - since we set hbrBackground)
        // FillRect(hdc, &rect, (HBRUSH)(COLOR_INFOBK + 1));

        // Draw centered text
        const wchar_t* text = L"You pressed F9!";
        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        DrawTextW(hdc, text, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_NCHITTEST:
        // Make the popup click-through so it doesn't steal focus
        return HTTRANSPARENT;

    case WM_DESTROY:
        KillTimer(hwnd, TIMER_ID_POPUP_CLOSE);
        return 0;

    default:
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
}
