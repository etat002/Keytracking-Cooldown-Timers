#include <windows.h>
#include <stdio.h>
#include <cstdlib>
#include <ctime>

//compile with: g++ -mwindows notify.cpp -lgdi32 -o notify.exe

const wchar_t MAIN_CLASS_NAME[]  = L"MainWindowClass";
const wchar_t POPUP_CLASS_NAME[] = L"PopupClass";

// Settings
const int keysToTrack[] = { 'T', 'P', 'Y' };
const int delays[]      = { 5000, 10000, 5000 };  // in milliseconds
const int numKeys       = sizeof(keysToTrack) / sizeof(keysToTrack[0]);

const int TIMER_POLLING       = 1;
const int TIMER_BASE_DELAY    = 100;
const int TIMER_POPUP_CLOSE   = 9999;

bool timerStarted[numKeys]      = { false };
HWND hwndPopups[numKeys]        = { nullptr };
HINSTANCE g_hInstance           = nullptr;

LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK PopupWndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int) {

    g_hInstance = hInstance;

    WNDCLASSW wc = {};
    wc.lpfnWndProc   = MainWndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = MAIN_CLASS_NAME;
    RegisterClassW(&wc);

    wc.lpfnWndProc   = PopupWndProc;
    wc.lpszClassName = POPUP_CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_INFOBK + 1);
    RegisterClassW(&wc);

    HWND hwndMain = CreateWindowExW(0, MAIN_CLASS_NAME, L"Main", 0,
                                    0, 0, 0, 0, nullptr, nullptr, hInstance, nullptr);

    SetTimer(hwndMain, TIMER_POLLING, 50, nullptr);  // Fast polling

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}

LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TIMER:
        if ((GetAsyncKeyState(VK_ESCAPE) & 0x8000) != 0) {
            PostQuitMessage(0);
            return 0;
        }
        if (wParam == TIMER_POLLING) {
            for (int i = 0; i < numKeys; ++i) {
                SHORT state = GetAsyncKeyState(keysToTrack[i]);
                bool isDown = (state & 0x8000) != 0;

                // If key is pressed and timer hasn't started yet, start the timer
                if (isDown && !timerStarted[i]) {

                    // Start the timer for the delay
                    SetTimer(hwnd, TIMER_BASE_DELAY + i, delays[i], nullptr);
                    timerStarted[i] = true;  // Mark the timer as started
                }
            }
        }
        else if (wParam >= TIMER_BASE_DELAY && wParam < TIMER_BASE_DELAY + numKeys) {
	    int i = wParam - TIMER_BASE_DELAY;
            // Destroy any previous popup
            if (hwndPopups[i] && IsWindow(hwndPopups[i])) {
                DestroyWindow(hwndPopups[i]);
            }

            // Create the popup window
	    if(timerStarted[i]){
            wchar_t title[64];
            _snwprintf(title, 64, L"Key '%lc' Pressed", (wchar_t)keysToTrack[i]);

            hwndPopups[i] = CreateWindowExW(WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                                            POPUP_CLASS_NAME, title,
                                            WS_POPUP | WS_BORDER | WS_VISIBLE,
                                            100, 100 + i * 100, 200, 50,
                                            nullptr, nullptr, g_hInstance, nullptr);

            if (hwndPopups[i]) {
                SetWindowLongPtrW(hwndPopups[i], GWLP_USERDATA, (LONG_PTR)keysToTrack[i]);
                ShowWindow(hwndPopups[i], SW_SHOWNOACTIVATE);
                SetTimer(hwndPopups[i], TIMER_POPUP_CLOSE, 2000, nullptr); // Close after 2 seconds
            }
            timerStarted[i] = false;  // Reset the timer flag when the timer finishes
	    }
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK PopupWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_TIMER:
        if (wParam == TIMER_POPUP_CLOSE) {
            // Destroy the popup
            DestroyWindow(hwnd);

            // Reset the timer flag immediately after popup closes
            int key = (int)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
            for (int i = 0; i < numKeys; ++i) {
                if (keysToTrack[i] == key) {
                    timerStarted[i] = false;  // Reset the timer flag to allow key press again
                }
            }
        }
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rect;
        GetClientRect(hwnd, &rect);
	
	// Seed random number generator if not already done
        static bool isSeeded = false;
        if (!isSeeded) {
            srand(static_cast<unsigned int>(time(0)));  // Seed based on current time
            isSeeded = true;
        }

        // Generate random RGB values for background color
        int r = (rand() % 106)+150;  // Random value between 0 and 255
        int g = (rand() % 106)+150;  // Random value between 0 and 255
        int b = (rand() % 106)+150;  // Random value between 0 and 255
	
	 // Create a solid brush with the random color
    	HBRUSH hBrush = CreateSolidBrush(RGB(r, g, b));

    	// Fill the background with the random color
    	FillRect(hdc, &rect, hBrush);

    	// Cleanup the brush
    	DeleteObject(hBrush);
	
        int key = (int)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
        wchar_t msg[64];
        _snwprintf(msg, 64, L"Key '%lc' cooldown complete!", (wchar_t)key);

        SetTextColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        DrawTextW(hdc, msg, -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_NCHITTEST:
        return HTTRANSPARENT;

    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}
