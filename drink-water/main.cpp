#include <Windows.h>
#include <WindowsX.h>
#include <tchar.h>

#include <chrono>

#include "resource.h"

class DrinkWater
{
public:
    explicit DrinkWater(HINSTANCE hInstance);

    int Run(int nCmdShow);

protected:
    static void RegisterWindowClass(HINSTANCE hInstance, PCTCH szWindowClass);

    static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    static LRESULT MyWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

    static BOOL OnCreate(HWND hWnd, LPCREATESTRUCT lpCreateStruct);

    static void OnPaint(HWND hWnd);

    static void OnDestroy(HWND hWnd);

    static std::pair<int, int> GetDesktopResolution();

    static int GetTaskBarHeight();

    static std::pair<int, int> GetWindowStartPosition(const int windowWidth, const int windowHeight);

private:
    HINSTANCE hInstance_{ nullptr };
    HWND      hWnd_{ nullptr };

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

int DrinkWater::Run(int const nCmdShow)
{
    auto const [windowX, windowY] = GetWindowStartPosition(WINDOW_WIDTH, WINDOW_HEIGHT);

    constexpr auto dwStyle = WS_VISIBLE | WS_POPUPWINDOW | WS_CAPTION;

    hWnd_ = CreateWindowEx(
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

    ShowWindow(hWnd_, nCmdShow);

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

    if (RegisterClassEx(&wcex) == 0)
        throw std::system_error{ static_cast<int>(GetLastError()), std::system_category() };
}

LRESULT CALLBACK DrinkWater::WndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam)
{
    auto & drinkWater = reinterpret_cast<DrinkWater &>(s_drinkWaterStorage);
    auto const ret = drinkWater.MyWndProc(hWnd, message, wParam, lParam);
    return ret;
}

LRESULT DrinkWater::MyWndProc(const HWND hWnd, const UINT message, const WPARAM wParam, const LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE:
        return HANDLE_WM_CREATE(hWnd, wParam, lParam, OnCreate);
    case WM_PAINT:
        return HANDLE_WM_PAINT(hWnd, wParam, lParam, OnPaint);
    case WM_DESTROY:
        return HANDLE_WM_DESTROY(hWnd, wParam, lParam, OnDestroy);
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
}

inline BOOL DrinkWater::OnCreate(const HWND hWnd, LPCREATESTRUCT lpCreateStruct)
{
    SetLayeredWindowAttributes(hWnd, 0, (255 * 70) / 100, LWA_ALPHA);
    return TRUE;
}

inline void DrinkWater::OnPaint(const HWND hWnd)
{
    PAINTSTRUCT ps{};
    auto hdc = BeginPaint(hWnd, &ps);
    EndPaint(hWnd, &ps);
}

inline void DrinkWater::OnDestroy(HWND hWnd)
{
    PostQuitMessage(0);
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

int WINAPI _tWinMain(const HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, const int nCmdShow)
{
    int ret;

    try
    {
        auto const drinkWater = new (&s_drinkWaterStorage) DrinkWater{ hInstance };
        ret = drinkWater->Run(nCmdShow);
        drinkWater->~DrinkWater();
    }
    catch (std::system_error const& e)
    {
        MessageBoxA(nullptr, e.what(), "Error", MB_OK | MB_ICONERROR);
        ret = EXIT_FAILURE;
    }

    return ret;
}
