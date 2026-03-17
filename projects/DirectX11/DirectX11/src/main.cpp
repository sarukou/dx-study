#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <DirectXMath.h>
#include <wrl/client.h>

#include <string>
#include <cstdint>
#include <wincodec.h>

#include "Window.h"
#include "Renderer.h"
#include "Types.h"
#include "Utility.h"
#include "Camera.h"
#include "ObjLoader.h"

#include "BasicVertexShader.h"	// シェーダーをコンパイルしたヘッダーファイル
#include "BasicPixelShader.h"


#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#pragma comment(lib, "windowscodecs.lib")

using namespace Microsoft::WRL;
using namespace DirectX;


// シェーダー構造体
struct Shaders
{
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader;
};


// VertexBuffer 作成
ComPtr<ID3D11Buffer> g_vertexBuffer;
static void CreateVertexBuffer(ID3D11Device* device, const MeshData& meshData)
{
    // 各設定（どんな用途・性質）
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = static_cast<UINT>(sizeof(Vertex) * meshData.vertices.size());            // バッファーサイズ（バイト数）
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;             // バッファの使われ方（今回は「 GPU が主に使う」）
    bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;    // このバッファを何としてパイプラインにバインドするか（頂点バッファ）
    bufferDesc.CPUAccessFlags = 0;                      // CPU がこのバッファにアクセスできるか（ 0 はできない）
    bufferDesc.MiscFlags = 0;                           // 特殊な用途の追加フラグ（なし）
    bufferDesc.StructureByteStride = 0;                 // 特殊フラグの要素サイズ（使わないのでもちろんなし）

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = meshData.vertices.data();

    ThrowIfFailed(device->CreateBuffer(&bufferDesc, &initData, g_vertexBuffer.GetAddressOf()), "Create Vertex Buffer Failed");
}

// IndexBuffer 作成
ComPtr<ID3D11Buffer> g_indexBuffer;
static void CreateIndexBuffer(ID3D11Device* device, const MeshData& meshData)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = static_cast<UINT>(sizeof(uint32_t) * meshData.indices.size());
    bufferDesc.Usage = D3D11_USAGE_DEFAULT;
    bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    bufferDesc.CPUAccessFlags = 0;
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = meshData.indices.data();

    ThrowIfFailed(device->CreateBuffer(&bufferDesc, &initData, g_indexBuffer.GetAddressOf()), "Create Index Buffer Failed");
}

// VertexShader/PixelShader 作成
static void CreateShaders(Shaders& shaders, Renderer& renderer)
{
    // VertexShader
    HRESULT hr = renderer.GetDevice()->CreateVertexShader(
        g_BasicVertexShader, std::size(g_BasicVertexShader), NULL, shaders.vertexShader.GetAddressOf());
    ThrowIfFailed(hr, "Create VertexShader Failed");

    // PixelShader
    hr = renderer.GetDevice()->CreatePixelShader(
        g_BasicPixelShader, std::size(g_BasicPixelShader), NULL, shaders.pixelShader.GetAddressOf());
    ThrowIfFailed(hr, "Create PixelShader Failed");
}

// InputLayout 作成
ComPtr<ID3D11InputLayout> g_inputLayout;
static void CreateInputLayout(ID3D11Device* device)
{
    // HLSL ファイルと同じ形式（セマンティクス）
    D3D11_INPUT_ELEMENT_DESC layout[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",    0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    HRESULT hr = device->CreateInputLayout(layout, _countof(layout),
        g_BasicVertexShader, std::size(g_BasicVertexShader), &g_inputLayout);   // 生成されたヘッダーファイルを利用してバイトコードなどを渡す
    ThrowIfFailed(hr, "Create InputLayout Failed");
}

// Texture作成
ComPtr<ID3D11ShaderResourceView> g_textureSRV;
static void CreateTextureFromFile(ID3D11Device* device, const wchar_t* filePath)
{
    // WIC（画像読み込みライブラリ）のファクトリ生成
    ComPtr<IWICImagingFactory> factory;
    ThrowIfFailed(
        CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(factory.GetAddressOf())),
        "CoCreateInstance For WIC Factory Failed"
    );

    // ファイルをデコード
    ComPtr<IWICBitmapDecoder> decoder;
    ThrowIfFailed(
        factory->CreateDecoderFromFilename(filePath, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnLoad, decoder.GetAddressOf()),
        "Create DecoderFromFilename Failed"
    );

    // フレーム取得（通常画像は1フレーム）
    ComPtr<IWICBitmapFrameDecode> frame;
    ThrowIfFailed(
        decoder->GetFrame(0, frame.GetAddressOf()),
        "GetFrame Failed"
    );

    // RGBA32に変換
    ComPtr<IWICFormatConverter> converter;
    ThrowIfFailed(
        factory->CreateFormatConverter(converter.GetAddressOf()),
        "Create FormatConverter Failed"
    );
    ThrowIfFailed(
        converter->Initialize(frame.Get(), GUID_WICPixelFormat32bppRGBA, WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom),
        "WIC Format Converter Initialize Failed"
    );

    // サイズ取得
    UINT width = 0; UINT height = 0;
    ThrowIfFailed(
        converter->GetSize(&width, &height),
        "Get Size Failed"
    );

    // ピクセルバッファに読み込み
    const UINT bytesPerPixel = 4;
    const UINT rowPitch = width * bytesPerPixel;
    const UINT imageSize = rowPitch * height;
    std::vector<std::uint8_t> pixels(imageSize);
    ThrowIfFailed(
        converter->CopyPixels(nullptr, rowPitch, imageSize, pixels.data()),
        "Copy Pixels Failed"
    );


    // テクスチャを作成
    D3D11_TEXTURE2D_DESC textureDesc = {};
    textureDesc.Width = width;
    textureDesc.Height = height;
    textureDesc.MipLevels = 1;                          // ミップマップ段数（縮小表示の品質向上に使われる）
    textureDesc.ArraySize = 1;                          // テクスチャ配列の枚数
    textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;    // GPU側でのピクセル形式
    textureDesc.SampleDesc.Count = 1;                   // MSAA
    textureDesc.SampleDesc.Quality = 0;                 // MSAAの品質
    textureDesc.Usage = D3D11_USAGE_DEFAULT;            // GPUで普通に使う標準的なリソース
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE; // どういう用途で使用するか（シェーダーから読む）
    textureDesc.CPUAccessFlags = 0;                     // CPUから直接触るか
    textureDesc.MiscFlags = 0;                          // 特殊な用途（ミップ自動生成やキューブマップなら変わる可能性あり）

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = pixels.data();       // CPU側の元データ先頭ポインタ
    initData.SysMemPitch = rowPitch;        // 1行あたりのバイト数（2Dテクスチャでは重要）
    initData.SysMemSlicePitch = imageSize;  // 1スライスあたりのサイズ（3Dテクスチャで特に重要）

    ComPtr<ID3D11Texture2D> texture;
    ThrowIfFailed(
        device->CreateTexture2D(&textureDesc, &initData, &texture),
        "Create Texture2D Failed"
    );


    // SRV（シェーダーリソースビュー）を作成（シェーダーから読むためのビュー（HLSLで .Sample() が使える））
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;                    // どのフォーマットとしてシェーダーから見るか
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;  // 2D, 2DArray, Cube, 3D など、いろいろな見え方の指定
    srvDesc.Texture2D.MostDetailedMip = 0;                  // ミップレベル（0が細かくて元画像）
    srvDesc.Texture2D.MipLevels = 1;                        // ミップマップ段数

    ThrowIfFailed(
        device->CreateShaderResourceView(texture.Get(), &srvDesc, g_textureSRV.GetAddressOf()),
        "Create ShaderResorceView Failed"
    );
}

// SamplerState（テクスチャの読み方ルール）作成
ComPtr<ID3D11SamplerState> g_samplerState;
static void CreateSamplerState(ID3D11Device* device)
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;   // 補間方法（縮小時、拡大時、ミップ切り替え時の場面に対してLinear）
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;      // U方向（横方向）で UV が 0～1 を超えたときの扱い（WARP は繰り返し）
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;      // V方向（縦方向）
    samplerDesc.AddressW= D3D11_TEXTURE_ADDRESS_WRAP;       // W方向（3Dテクスチャ用）
    samplerDesc.MipLODBias = 0.0f;                          // どのミップを選ぶかに対しての補正値（負の値ほど細かく、正の値ほど粗い）
    samplerDesc.MaxAnisotropy = 1;                          // 異方性フィルタリングの強さ（斜め方向のきれいさが変わる）
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;    // 比較サンプラー用の設定
    samplerDesc.BorderColor[0] = 0.0f;                      // 境界モードが BORDER の時に使う色（WARP のため使われない）
    samplerDesc.BorderColor[1] = 0.0f;
    samplerDesc.BorderColor[2] = 0.0f;
    samplerDesc.BorderColor[3] = 0.0f;
    samplerDesc.MinLOD = 0;                 // 使う最小LOD（Level of Detail）
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX; // 使う最大LOD

    ThrowIfFailed(
        device->CreateSamplerState(&samplerDesc, g_samplerState.GetAddressOf()),
        "Create SamplerState Failed"
    );
}


// 描画処理（更新）
static void Render(Renderer& renderer, const MeshData& meshData, Shaders& shaders, const ProjectSettings& settings, Camera& camera, float time)
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

    // IA（Input Assembler）設定：Layout / VertexBuffer / IndexBuffer / Topology
    renderer.GetDeviceContext()->IASetInputLayout(g_inputLayout.Get());

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    renderer.GetDeviceContext()->IASetVertexBuffers(0, 1, g_vertexBuffer.GetAddressOf(), &stride, &offset);

    renderer.GetDeviceContext()->IASetIndexBuffer(g_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    renderer.GetDeviceContext()->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);    // 三角形

    // シェーダー設定
    renderer.GetDeviceContext()->VSSetShader(shaders.vertexShader.Get(), nullptr, 0);
    renderer.GetDeviceContext()->PSSetShader(shaders.pixelShader.Get(), nullptr, 0);

    // シェーダーにテクスチャとサンプラーを設定
    renderer.GetDeviceContext()->PSSetShaderResources(0, 1, g_textureSRV.GetAddressOf());
    renderer.GetDeviceContext()->PSSetSamplers(0, 1, g_samplerState.GetAddressOf());

    // シェーダーに定数バッファを設定（HLSL 側で register(b0) にしたのでスロット 0 に入れる）
    ID3D11Buffer* constantBuffers[] = { renderer.GetConstantBuffer() };
    renderer.GetDeviceContext()->VSSetConstantBuffers(0, 1, constantBuffers);
    renderer.GetDeviceContext()->PSSetConstantBuffers(0, 1, constantBuffers);

    // 描き込み
    renderer.GetDeviceContext()->DrawIndexed(static_cast<UINT>(meshData.indices.size()), 0, 0);

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
    Shaders shaders = {};

    Window window;
    Renderer renderer;
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

        // シェーダー作成
        CreateShaders(shaders, renderer);
        // インプットレイアウト作成
        CreateInputLayout(renderer.GetDevice());

        // メッシュ作成
        meshData = LoadObj(L"model.obj");

        // 頂点バッファ作成
        CreateVertexBuffer(renderer.GetDevice(), meshData);
        // インデックスバッファ作成
        CreateIndexBuffer(renderer.GetDevice(), meshData);

        // テクスチャ作成
        CreateTextureFromFile(renderer.GetDevice(), L"test.jpg");
        // サンプラーステート作成
        CreateSamplerState(renderer.GetDevice());


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
                Render(renderer, meshData, shaders, settings, camera, time);
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
