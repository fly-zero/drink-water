#include <Windows.h>
#include <WindowsX.h>
#include <tchar.h>

#include <cassert>

#include <chrono>
#include <filesystem>

#include "resource.h"

#if __cplusplus >= 202002L
#define attr_nodiscard    [[nodiscard]]
#define attr_maybe_unused [[maybe_unused]]
#define attr_likely(x)    (x) [[likely]]
#define attr_unlikely(x)  (x) [[likely]]
#else
#define attr_nodiscard
#define attr_maybe_unused
#define attr_likely
#define attr_likely
#endif

class DrinkWater
{
public:
    explicit DrinkWater(HINSTANCE hInstance);

    int Run(int nCmdShow, int drinkWaterInterval);

protected:
    static void RegisterWindowClass(HINSTANCE hInstance, PCTCH szWindowClass);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    LRESULT MyWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) const;

    static void OnPaint(HWND hWnd);

    static void OnClose(HWND hWnd);

    static void OnDestroy(HWND hWnd);

    void OnTimer(HWND hWnd, UINT_PTR nIdEvent) const;

    static std::pair<int, int> GetDesktopResolution();

    static int GetTaskBarHeight();

    static std::pair<int, int> GetWindowStartPosition(int windowWidth, int windowHeight);

    attr_nodiscard std::tuple<int, int, int, int> GetStaticTextPosition() const;

    static std::system_error SystemError(const char * what);

private:
    HINSTANCE hInstance_{ nullptr };
    HWND      hMainWnd_{ nullptr };
    HWND      hStaticText_{ nullptr };
    HFONT     hFont_{ nullptr };
    UINT_PTR  hTimer_{ 0 };

    static constexpr auto STATIC_TEXT{ TEXT("It's time to drink water!") };
    static constexpr auto WINDOW_CLASS{ TEXT("DrinkWater") };
    static constexpr auto WINDOW_WIDTH = 400;
    static constexpr auto WINDOW_HEIGHT = 300;
};

std::aligned_storage_t<sizeof (DrinkWater), alignof (DrinkWater)> s_drinkWaterStorage{ };

inline DrinkWater::DrinkWater(const HINSTANCE hInstance)
    : hInstance_{ hInstance }
{
    RegisterWindowClass(GetModuleHandle(nullptr), TEXT("DrinkWater"));
}

int DrinkWater::Run(int const nCmdShow, int const drinkWaterInterval)
{
    auto const [windowX, windowY] = GetWindowStartPosition(WINDOW_WIDTH, WINDOW_HEIGHT);
    constexpr auto dwStyle = WS_VISIBLE | WS_POPUPWINDOW | WS_CAPTION;

    // create main window
    hMainWnd_ = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
        WINDOW_CLASS,
        WINDOW_CLASS,
        dwStyle,
        windowX,
        windowY,
        WINDOW_WIDTH,
        WINDOW_HEIGHT,
        nullptr,
        nullptr,
        hInstance_,
        nullptr
    );

    if attr_unlikely(!hMainWnd_)
        throw SystemError("failed to create main window");

    // create font
    hFont_ = CreateFont(
        32, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
        DEFAULT_PITCH | FF_ROMAN,
        TEXT("Times New Roman")
    );

    if attr_unlikely(!hFont_)
        throw SystemError("failed to create font");

    // get static text position
    auto const [staticTextX, staticTextY, staticTextW, staticTextH] = GetStaticTextPosition();

    // create static text
    hStaticText_ = CreateWindowEx(
        0,
        TEXT("STATIC"),
        STATIC_TEXT,
        WS_CHILD | WS_VISIBLE | SS_CENTER,
        staticTextX, staticTextY, staticTextW, staticTextH,
        hMainWnd_,
        nullptr,
        hInstance_,
        nullptr
    );

    if attr_unlikely(!hStaticText_)
        throw SystemError("failed to create static control");

    // create timer
    const std::chrono::milliseconds interval{ std::chrono::minutes{ drinkWaterInterval } };
    hTimer_ = SetTimer(hMainWnd_, 1,  static_cast<unsigned>(interval.count()), nullptr);

    // set font
    SendMessage(hStaticText_, WM_SETFONT, reinterpret_cast<WPARAM>(hFont_), TRUE);

    // set window opacity
    SetLayeredWindowAttributes(hMainWnd_, 0, (255 * 70) / 100, LWA_ALPHA); // 70% opacity

    // set text color
    SetBkMode(GetDC(hStaticText_), TRANSPARENT);
    SetTextColor(GetDC(hStaticText_), RGB(255, 0, 0));

    // show window
    ShowWindow(hMainWnd_, nCmdShow);
    UpdateWindow(hMainWnd_);

    // message loop
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(& msg);
    }

    return static_cast<int>(msg.wParam);
}

void DrinkWater::RegisterWindowClass(const HINSTANCE hInstance, const PCTCH szWindowClass)
{
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof (WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_OWNDC;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof (LONG_PTR);
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APP));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>((COLOR_WINDOW + 1));  // NOLINT(performance-no-int-to-ptr)
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_APP));

    if attr_unlikely(RegisterClassEx(&wcex) == 0)
        throw SystemError("failed to register window class");
}

LRESULT CALLBACK DrinkWater::WndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam)
{
    const auto & drinkWater = reinterpret_cast<DrinkWater &>(s_drinkWaterStorage);
    auto const ret = drinkWater.MyWndProc(hWnd, message, wParam, lParam);
    return ret;
}

LRESULT DrinkWater::MyWndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam) const
{
    switch (message)
    {
    case WM_PAINT:
        return HANDLE_WM_PAINT(hWnd, wParam, lParam, OnPaint);
    case WM_CLOSE:
        return HANDLE_WM_CLOSE(hWnd, wParam, lParam, OnClose);
    case WM_DESTROY:
        return HANDLE_WM_DESTROY(hWnd, wParam, lParam, OnDestroy);
    case WM_TIMER:
        return HANDLE_WM_TIMER(hWnd, wParam, lParam, OnTimer);
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

inline void DrinkWater::OnPaint(const HWND hWnd)
{
    PAINTSTRUCT ps{};
    attr_maybe_unused auto hdc = BeginPaint(hWnd, &ps);
    EndPaint(hWnd, &ps);
}

inline void DrinkWater::OnClose(const HWND hWnd)
{
    ShowWindow(hWnd, SW_HIDE); // hide window
}

inline void DrinkWater::OnDestroy(HWND hWnd)
{
    PostQuitMessage(0);
}

inline void DrinkWater::OnTimer(const HWND hWnd, const UINT_PTR nIdEvent) const
{
    if attr_likely(nIdEvent == hTimer_)
        ShowWindow(hWnd, SW_SHOW); // show window
}

inline std::pair<int, int> DrinkWater::GetDesktopResolution()
{
    RECT desktop;
    const auto hDesktop = GetDesktopWindow();
    GetWindowRect(hDesktop, &desktop);
    return { desktop.right, desktop.bottom };
}

inline int DrinkWater::GetTaskBarHeight()
{
    APPBARDATA appBarData;
    appBarData.cbSize = sizeof(appBarData);
    SHAppBarMessage(ABM_GETTASKBARPOS, &appBarData);
    return appBarData.rc.bottom - appBarData.rc.top;
}

inline std::pair<int, int> DrinkWater::GetWindowStartPosition(const int windowWidth, const int windowHeight)
{
    auto const [desktopWidth, desktopHeight] = GetDesktopResolution();
    auto const taskBarHeight = GetTaskBarHeight();
    auto const windowX = desktopWidth - windowWidth;
    auto const windowY = desktopHeight - windowHeight - taskBarHeight;
    return std::make_pair(windowX, windowY);
}

std::tuple<int, int, int, int> DrinkWater::GetStaticTextPosition() const
{
    assert(hMainWnd_);

    auto const hdc = GetDC(hMainWnd_);
    SelectObject(hdc, hFont_);

    SIZE size;
    GetTextExtentPoint32(hdc, STATIC_TEXT, static_cast<int>(_tcslen(STATIC_TEXT)), &size);

    RECT rect;
    GetClientRect(hMainWnd_, &rect);

    auto const ret = std::make_tuple((rect.right - size.cx) / 2, (rect.bottom - size.cy) / 2, size.cx, size.cy);
    ReleaseDC(hMainWnd_, hdc);
    return ret;
}

inline std::system_error DrinkWater::SystemError(const char * what)
{
    return { static_cast<int>(GetLastError()), std::system_category(), what };
}

// Parse drink water interval from command line
static int ParseDrinkWaterInterval(const LPTSTR lpCmdLine)
{
    if (!lpCmdLine || *lpCmdLine == '\0')
        throw std::invalid_argument("lpCmdLine is null");

    auto const interval = _ttoi(lpCmdLine);
    if (interval <= 0)
        throw std::invalid_argument("invalid interval");

    return interval;
}

int WINAPI _tWinMain(const HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, const int nShowCmd)
{
    int ret;

    try
    {
        auto const drinkWaterInterval = ParseDrinkWaterInterval(lpCmdLine);
        auto const drinkWater = new (&s_drinkWaterStorage) DrinkWater{ hInstance };
        ret = drinkWater->Run(nShowCmd, drinkWaterInterval);
        drinkWater->~DrinkWater();
    }
    catch (std::system_error const & e)
    {
        MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
        ret = EXIT_FAILURE;
    }
    catch (std::invalid_argument const &)
    {
        char buff[4096];
        GetModuleFileNameA(nullptr, buff, sizeof buff);
        auto const exe = std::filesystem::path{ buff }.filename().string();
        [[maybe_unused]] auto const len = snprintf(buff, sizeof buff, "Usage: %s <interval in minutes>", exe.c_str());
        assert(len > 0 && static_cast<size_t>(len) < sizeof buff);
        MessageBoxA(nullptr, buff, "Error", MB_OK | MB_ICONERROR);
        ret = EXIT_FAILURE;
    }

    return ret;
}
