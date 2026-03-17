#pragma once

#include "Types.h"

#include <windows.h>

class Window
{
public:
    // ウィンドウ初期化
    bool Initialize(HINSTANCE hInstance, int nCmdShow, const ProjectSettings& settings);

    HWND GetHwnd() const { return m_hWnd; }

private:
    // ウィンドウプロシージャ
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
    // ウィンドウハンドル
    HWND m_hWnd = nullptr;
};