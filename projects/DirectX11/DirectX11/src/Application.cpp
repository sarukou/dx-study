#include "Application.h"

#include "ObjLoader.h"
#include "Utility.h"

#include <DirectXMath.h>

using namespace DirectX;

bool Application::Initialize(HINSTANCE hInstance, int nCmdShow)
{
    if (!m_window.Initialize(hInstance, nCmdShow, m_settings)) {
        return false;
    }

    m_renderer.Initialize(m_window.GetHwnd(), m_settings);
    m_shader.Initialize(m_renderer);

    MeshData meshData = LoadObj(L"model.obj");
    m_mesh.Initialize(m_renderer, meshData);

    m_albedoTexture.Initialize(m_renderer, L"test_albedo.png");
    m_normalTexture.Initialize(m_renderer, L"test_normal.png");

    QueryPerformanceFrequency(&m_frequency);
    QueryPerformanceCounter(&m_prevCounter);

    GetCursorPos(&m_prevCursor);

    return true;
}

int Application::Run()
{
    MSG msg = {};

    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else {
            float deltaTime = CalculateDeltaTime();
            Update(deltaTime);
            Render();
        }
    }

    return static_cast<int>(msg.wParam);
}

void Application::UpdateMouseDelta()
{
    m_mouseDx = 0;
    m_mouseDy = 0;

    if (GetForegroundWindow() == m_window.GetHwnd() && m_camera.GetMouseLook()) {
        POINT currentCursor = {};
        GetCursorPos(&currentCursor);

        m_mouseDx = currentCursor.x - m_prevCursor.x;
        m_mouseDy = currentCursor.y - m_prevCursor.y;

        m_prevCursor = currentCursor;
    }
    else {
        GetCursorPos(&m_prevCursor);
    }
}

float Application::CalculateDeltaTime()
{
    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);

    float deltaTime = float(now.QuadPart - m_prevCounter.QuadPart) / float(m_frequency.QuadPart);
    m_prevCounter = now;

    // デバッグ停止や負荷で跳ねた時の保険
    if (deltaTime > 0.1f) {
        deltaTime = 0.1f;
    }

    return deltaTime;
}

void Application::Update(float deltaTime)
{
    // タイム更新
    m_time += deltaTime;

    UpdateMouseDelta();

    // NormalMap切り替え
    bool currentToggleKey = (GetAsyncKeyState('N') & 0x8000) != 0;
    if (currentToggleKey && !m_prevNKey) {
        m_useNormalMap = !m_useNormalMap;
        OutputDebugStringA(m_useNormalMap ? "NormalMap ON\n" : "NormalMap OFF\n");
    }
    m_prevNKey = currentToggleKey;
    // Matellic切り替え
    bool currentMKey = (GetAsyncKeyState('M') & 0x8000) != 0;
    if (currentMKey && !m_prevMKey) {
        m_metallicPresetIndex = (m_metallicPresetIndex + 1) % 5;

        switch (m_metallicPresetIndex) {
            case 0: m_metallic = 0.0f;
                break;
            case 1: m_metallic = 0.25f;
                break;
            case 2: m_metallic = 0.5f;
                break;
            case 3: m_metallic = 0.75f;
                break;
            case 4: m_metallic = 1.0f;
                break;
        }
        
        char buffer[128];
        sprintf_s(buffer, "Metallic = %.2f\n", m_metallic);
        OutputDebugStringA(buffer);
    }
    m_prevMKey = currentMKey;
    // Roughness切り替え
    bool currentRKey = (GetAsyncKeyState('R') & 0x8000) != 0;
    if (currentRKey && !m_prevRKey) {
        m_roughnessPresetIndex = (m_roughnessPresetIndex + 1) % 10;

        switch (m_roughnessPresetIndex) {
            case 0: m_roughness = 0.1f;
                break;
            case 1: m_roughness = 0.2f;
                break;
            case 2: m_roughness = 0.3f;
                break;
            case 3: m_roughness = 0.4f;
                break;
            case 4: m_roughness = 0.5f;
                break;
            case 5: m_roughness = 0.6f;
                break;
            case 6: m_roughness = 0.7f;
                break;
            case 7: m_roughness = 0.8f;
                break;
            case 8: m_roughness = 0.9f;
                break;
            case 9: m_roughness = 1.0f;
                break;
        }

        char buffer[128];
        sprintf_s(buffer, "Roughness = %.2f\n", m_roughness);
        OutputDebugStringA(buffer);
    }
    m_prevRKey = currentRKey;
    // BaseColor切り替え
    bool currentCKey = (GetAsyncKeyState('C') & 0x8000) != 0;
    if (currentCKey && !m_prevCKey) {
        m_colorPresetIndex = (m_colorPresetIndex + 1) % 3;

        switch (m_colorPresetIndex) {
            case 0: m_baseColor = { 1.0f, 1.0f, 1.0f };     // 白
                break;
            case 1: m_baseColor = { 1.0f, 0.2f, 0.2f };     // 赤系
                break;
            case 2: m_baseColor = { 1.0f, 0.85f, 0.2f };    // 黄系
                break;
        }

        char buffer[128];
        sprintf_s(
            buffer,
            "BaseColor = (%.2f, %.2f, %.2f)\n",
            m_baseColor.x, m_baseColor.y, m_baseColor.z
        );
        OutputDebugStringA(buffer);
    }
    m_prevCKey = currentCKey;

    // カメラ更新
    m_camera.Update(m_mouseDx, m_mouseDy, deltaTime);
}

void Application::Render()
{
    // 通常描画用の World / View / Projection を計算 //
    XMMATRIX world = XMMatrixIdentity();
    world *= XMMatrixScaling(1.0f, 1.0f, 1.0f);
    //world *= XMMatrixRotationY(m_time);
    world *= XMMatrixTranslation(0.0f, 0.0f, 0.0f);
    XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&m_camera.GetPosition()), m_camera.GetForward(), XMLoadFloat3(&m_camera.GetUp()));
    m_camera.SetAspect(static_cast<float>(m_settings.width) / static_cast<float>(m_settings.height));
    XMMATRIX projection = XMMatrixPerspectiveFovLH(m_camera.GetFovY(), m_camera.GetAspect(), m_camera.GetNearZ(), m_camera.GetFarZ());

    // ShadowPass //
    RenderShadowPass(world);

    // MainPass //
    RenderMainPass(world, view, projection);

    // DebugShadowMapPass //
    RenderDebugShadowMapPass();

    // 表示 //
    m_renderer.GetSwapChain()->Present(1, 0);
}

void Application::RenderShadowPass(const XMMATRIX& world)
{
    // ライト視点のWVPを作る //
    XMMATRIX lightViewProjection = XMLoadFloat4x4(&m_renderer.GetLightViewProjectionMatrix());
    XMMATRIX shadowWorldViewProjection = world * lightViewProjection;

    // ShadowPass用の定数バッファを作る //
    ConstantPerFrame constantPerFrame = {};
    XMStoreFloat4x4(&constantPerFrame.worldMatrix, XMMatrixTranspose(world));
    XMStoreFloat4x4(&constantPerFrame.worldViewProjectionMatrix, XMMatrixTranspose(shadowWorldViewProjection));
    XMStoreFloat4x4(&constantPerFrame.lightViewProjectionMatrix, XMMatrixTranspose(lightViewProjection));

    // 定数バッファに書き込み（前の内容を捨てて新しい内容で全部上書き）//
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = m_renderer.GetDeviceContext()->Map(m_renderer.GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    ThrowIfFailed(hr, "Map ShadowPass Constant Buffer Failed");
    memcpy(mapped.pData, &constantPerFrame, sizeof(constantPerFrame));
    m_renderer.GetDeviceContext()->Unmap(m_renderer.GetConstantBuffer(), 0);

    // ShadowPass開始 //
    m_renderer.BeginShadowPass();

    // ShadowPass用シェーダーを設定
    m_shader.BindShadowPass(m_renderer);
    // Meshを設定
    m_mesh.Bind(m_renderer);
    m_renderer.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);   // 三角形

    // シェーダーに定数バッファを設定（HLSL側で register(b0) にしたのでスロット0 に入れる）
    ID3D11Buffer* constantBuffers[] = { m_renderer.GetConstantBuffer() };
    m_renderer.GetDeviceContext()->VSSetConstantBuffers(0, 1, constantBuffers);     // ShadowPassではVSだけが定数バッファを使う

    // 描き込み （深度だけ描画）
    m_renderer.GetDeviceContext()->DrawIndexed(m_mesh.GetIndexCount(), 0, 0);
}

void Application::RenderMainPass(const XMMATRIX& world, const XMMATRIX& view, const XMMATRIX& projection)
{
    // MainPass用ConstantBuffer作成
    ConstantPerFrame constantPerFrame = {};
    XMStoreFloat4x4(&constantPerFrame.worldMatrix, XMMatrixTranspose(world));
    XMStoreFloat4x4(&constantPerFrame.viewMatrix, XMMatrixTranspose(view));
    XMStoreFloat4x4(&constantPerFrame.projectionMatrix, XMMatrixTranspose(projection));
    XMStoreFloat4x4(&constantPerFrame.worldViewProjectionMatrix, XMMatrixTranspose(world * view * projection));
    XMMATRIX lightViewProjection = XMLoadFloat4x4(&m_renderer.GetLightViewProjectionMatrix());
    XMStoreFloat4x4(&constantPerFrame.lightViewProjectionMatrix, XMMatrixTranspose(lightViewProjection));

    // カメラ
    constantPerFrame.cameraPosition = m_camera.GetPosition();

    // ライト系
    constantPerFrame.directional = m_directional; // 光が進む向き
    constantPerFrame.lightColor = m_lightColor;
    constantPerFrame.ambient = m_ambient;

    // NormalMap
    constantPerFrame.useNormalMap = m_useNormalMap ? 1 : 0;

    // PBRマテリアル
    constantPerFrame.baseColor = m_baseColor;
    constantPerFrame.metallic = m_metallic;
    constantPerFrame.roughness = m_roughness;


    // 定数バッファに書き込み（前の内容を捨てて新しい内容で全部上書き）//
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = m_renderer.GetDeviceContext()->Map(m_renderer.GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    ThrowIfFailed(hr, "Map MainPass Constant Buffer Failed");
    memcpy(mapped.pData, &constantPerFrame, sizeof(constantPerFrame));
    m_renderer.GetDeviceContext()->Unmap(m_renderer.GetConstantBuffer(), 0);

    // MainPass開始 //
    const float clearColor[4] = { 0 / 255.0f, 99 / 255.0f, 181 / 255.0f, 1.0f };
    m_renderer.BeginMainPass(clearColor);

    // シェーダーを設定
    m_shader.Bind(m_renderer);
    // メッシュを設定
    m_mesh.Bind(m_renderer);
    // シェーダーにテクスチャとサンプラーを設定
    m_albedoTexture.Bind(m_renderer, 0);
    m_normalTexture.Bind(m_renderer, 1);

    m_renderer.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);    // 三角形

    // シェーダーに定数バッファを設定（HLSL側で register(b0) にしたのでスロット0 に入れる）
    ID3D11Buffer* constantBuffers[] = { m_renderer.GetConstantBuffer() };
    m_renderer.GetDeviceContext()->VSSetConstantBuffers(0, 1, constantBuffers);
    m_renderer.GetDeviceContext()->PSSetConstantBuffers(0, 1, constantBuffers);

    // 描き込み
    m_renderer.GetDeviceContext()->DrawIndexed(m_mesh.GetIndexCount(), 0, 0);
}

void Application::RenderDebugShadowMapPass()
{
    // Debug表示用の出力先をセット
    m_renderer.BeginDebugShadowMapPass();

    // Debug表示用シェーダーをセット
    m_shader.BindDebugShadowMap(m_renderer);

    // ShadowMapをt2にセット
    ID3D11ShaderResourceView* shadowMapSRV = m_renderer.GetShadowMapSRV();
    m_renderer.GetDeviceContext()->PSSetShaderResources(2, 1, &shadowMapSRV);

    // ShadowMap用Samplerをs2にセット
    ID3D11SamplerState* shadowSampler = m_renderer.GetShadowSampler();
    m_renderer.GetDeviceContext()->PSSetSamplers(2, 1, &shadowSampler);

    // VertexBuffer / IndexBuffer は使わない
    m_renderer.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    // SV_VertexID で 6 頂点を生成して矩形を描く
    m_renderer.GetDeviceContext()->Draw(6, 0);

    // 後続の Pass で SRV/DSV 衝突しないように外しておく
    ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
    m_renderer.GetDeviceContext()->PSSetShaderResources(2, 1, nullSRV);
}