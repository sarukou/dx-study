#include "Window.h"

// ウィンドウプロシージャ
LRESULT CALLBACK Window::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        const auto createStruct = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
        return 0;
    }
    case WM_CLOSE: {
        if (MessageBoxW(hwnd, L"保存していないデータは破棄されます。", L"ゲームを終了しますか？", MB_YESNO) == IDYES) {
            DestroyWindow(hwnd);
        }
        return 0;
    }
    case WM_DESTROY: {
        PostQuitMessage(0);
        return 0;
    }
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}


// ウィンドウ初期化
bool Window::Initialize(HINSTANCE hInstance, int nCmdShow, const ProjectSettings& settings)
{
    // ウィンドウクラス登録（見た目や挙動のウィンドウを作ると OS に申請）
    WNDCLASSEXW windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;    // 描画の再要求方針（縦・横サイズ変更されたら描き直す）
    windowClass.lpfnWndProc = WndProc;
    windowClass.cbClsExtra = 0;         // クラスに追加する余分なメモリ領域サイズ
    windowClass.cbWndExtra = 0;         // ウィンドウインスタンスに追加する余分な領域
    windowClass.hInstance = hInstance;  // 実行モジュール（EXE/DLL）のインスタンスハンドル
    windowClass.hIcon = NULL;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);     // 背景を塗るブラシ
    windowClass.lpszMenuName = NULL;                            // メニューリソース名
    windowClass.lpszClassName = settings.title.c_str();         // ウィンドウクラス名（タイトルと同じだが概念的には別物）

    // ウィンドウクラスを OS に登録
    if (!RegisterClassExW(&windowClass)) {
        return false;
    }

    // ウィンドウ作成（クライアント領域を Width * Height にしたいから枠分を足し引き）
    RECT rect{ 0, 0, settings.width, settings.height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    m_hWnd = CreateWindowExW(0, windowClass.lpszClassName, settings.title.c_str(),
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, hInstance, this);

    if (!m_hWnd) {
        return false;
    }

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    return true;
}
