#pragma once

#include <windows.h>

#include "Window.h"
#include "Renderer.h"
#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "Camera.h"
#include "Types.h"

class Application
{
public:
    // アプリ全体の初期化
    bool Initialize(HINSTANCE hInstance, int nCmdShow);
    // メッセージループ
    int Run();

private:
    // 更新処理
    void Update(float deltaTime);
    // 描画処理
    void Render();
    // マウス移動量を計算
    void UpdateMouseDelta();
    // デルタタイム計算
    float CalculateDeltaTime();

private:
    ProjectSettings m_settings = {
        .title = L"DirectX11-study",
        .width = 1280,
        .height = 720
    };

    Window m_window;
    Renderer m_renderer;
    Shader m_shader;
    Mesh m_mesh;
    Texture m_texture;
    Camera m_camera;

    float m_time = 0.0f;
    int m_mouseDx = 0;
    int m_mouseDy = 0;
    POINT m_prevCursor = {};

    LARGE_INTEGER m_frequency = {};
    LARGE_INTEGER m_prevCounter = {};
};