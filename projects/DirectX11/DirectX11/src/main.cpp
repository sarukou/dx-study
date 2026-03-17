#include "Window.h"
#include "Renderer.h"
#include "Shader.h"
#include "Mesh.h"
#include "Texture.h"
#include "Camera.h"
#include "ObjLoader.h"
#include "Types.h"
#include "Utility.h"

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#pragma comment(lib, "windowscodecs.lib")

using namespace Microsoft::WRL;
using namespace DirectX;


// 描画処理（更新）
static void Render(Renderer& renderer, Shader& shader, const Mesh& mesh, const Texture& texture, const ProjectSettings& settings, Camera& camera, float time)
{
    // ワールド行列を計算
    XMMATRIX world = XMMatrixIdentity();
    world *= XMMatrixScaling(1.0f, 1.0f, 1.0f);
    world *= XMMatrixRotationY(time);
    world *= XMMatrixTranslation(0.0f, 0.0f, 0.0f);
    // ビュー行列を計算
    XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&camera.GetPosition()), camera.GetForward(), XMLoadFloat3(&camera.GetUp()));
    // プロジェクション行列を計算
    camera.SetAspect((float)settings.width / (float)settings.height);
    XMMATRIX projection = XMMatrixPerspectiveFovLH(camera.GetFovY(), camera.GetAspect(), camera.GetNearZ(), camera.GetFarZ());

    // 転置して保存
    ConstantPerFrame constantPerFrame = {};
    XMStoreFloat4x4(&constantPerFrame.worldMatrix, XMMatrixTranspose(world));
    XMStoreFloat4x4(&constantPerFrame.viewMatrix, XMMatrixTranspose(view));
    XMStoreFloat4x4(&constantPerFrame.projectionMatrix, XMMatrixTranspose(projection));
    XMStoreFloat4x4(& constantPerFrame.worldViewProjectionMatrix,XMMatrixTranspose(world * view * projection));


    // ライト系
    constantPerFrame.directional = { 0.0f, -1.0f, 1.0f }; // 光が進む向き
    constantPerFrame.lightColor  = { 1.0f,  1.0f, 1.0f };
    constantPerFrame.ambient     = { 0.3f,  0.3f, 0.3f };


    // 定数バッファに書き込み（前の内容を捨てて新しい内容で全部上書き）
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    HRESULT hr = renderer.GetDeviceContext()->Map(renderer.GetConstantBuffer(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    ThrowIfFailed(hr, "Map Constant Buffer Failed");
    memcpy(mapped.pData, &constantPerFrame, sizeof(constantPerFrame));
    renderer.GetDeviceContext()->Unmap(renderer.GetConstantBuffer(), 0);


    // 画面クリア
    const float clearColor[4] = { 0 / 255.0f, 99 / 255.0f, 181 / 255.0f, 1.0f };
    renderer.GetDeviceContext()->ClearRenderTargetView(renderer.GetRenderTargetView(), clearColor);

    // 出力先（RTV）をセット
    ID3D11RenderTargetView* renderTargetView = renderer.GetRenderTargetView();
    renderer.GetDeviceContext()->OMSetRenderTargets(1, &renderTargetView, nullptr);

    // Input Assembler・シェーダー設定
    shader.Bind(renderer);

    mesh.Bind(renderer);

    renderer.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);    // 三角形

    // シェーダーにテクスチャとサンプラーを設定
    texture.Bind(renderer);

    // シェーダーに定数バッファを設定（HLSL 側で register(b0) にしたのでスロット 0 に入れる）
    ID3D11Buffer* constantBuffers[] = { renderer.GetConstantBuffer() };
    renderer.GetDeviceContext()->VSSetConstantBuffers(0, 1, constantBuffers);
    renderer.GetDeviceContext()->PSSetConstantBuffers(0, 1, constantBuffers);

    // 描き込み
    renderer.GetDeviceContext()->DrawIndexed(mesh.GetIndexCount(), 0, 0);

    // 表示
    renderer.GetSwapChain()->Present(1, 0);
}


// エントリーポイント
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    static const ProjectSettings settings = {
        .title = L"DirectX11-study",
        .width = 1280,
        .height = 720
    };
    MeshData meshData = {};

    Window window;
    Renderer renderer;
    Shader shader;
    Mesh mesh;
    Texture texture;
    Camera camera;
    
    // COM 初期化
    ThrowIfFailed(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED), "CoInitializeEx failed");

    try {
        // ウィンドウ初期化
        if (!window.Initialize(hInstance, nCmdShow, settings)) {
            CoUninitialize();
            return -1;
        }
        // D3D11初期化
        renderer.Initialize(window.GetHwnd(), settings);

        // シェーダー・インプットレイアウト作成
        shader.Initialize(renderer);

        // メッシュ作成
        meshData = LoadObj(L"model.obj");
        mesh.Initialize(renderer, meshData);

        // テクスチャ・サンプラーステート作成
        texture.Initialize(renderer, L"test.jpg");


        // デルタタイム計算用
        LARGE_INTEGER freq = {};
        QueryPerformanceFrequency(&freq);
        LARGE_INTEGER prev = {};
        QueryPerformanceCounter(&prev);

        // 必要な変数
        float time = 0.0f;
        int mouseDx = 0, mouseDy = 0;
        POINT cursor = {};
        GetCursorPos(&cursor);

        MSG msg = {};
        while (msg.message != WM_QUIT) {  // メインループ

            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
            else {
                // デルタタイム計算
                LARGE_INTEGER now = {};
                QueryPerformanceCounter(&now);
                float deltaTime = float(now.QuadPart - prev.QuadPart) / float(freq.QuadPart);
                prev = now;
                // デバッグ停止や負荷で跳ねた時の保険
                if (deltaTime > 0.1f) {
                    deltaTime = 0.1f;
                }

                time += deltaTime;

                // マウス移動量を計算
                if (GetForegroundWindow() == window.GetHwnd() && camera.GetMouseLook()) {
                    POINT currentCursor = {};
                    GetCursorPos(&currentCursor);

                    mouseDx = currentCursor.x - cursor.x;
                    mouseDy = currentCursor.y - cursor.y;

                    cursor = currentCursor;
                }

                camera.Update(mouseDx, mouseDy, deltaTime);
                Render(renderer, shader, mesh, texture, settings, camera, time);
            }
        }
        // COM 解除
        CoUninitialize();

        return (int)msg.wParam;
    }
    catch (const std::exception& e) {
        // COM 解除
        CoUninitialize();

        MessageBoxA(nullptr, e.what(), "Fatal Error", MB_ICONERROR | MB_OK);
        return -1;
    }
}
