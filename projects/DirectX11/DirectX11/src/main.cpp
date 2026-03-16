#include <windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <DirectXMath.h>

#include <wrl/client.h>
#include <stdexcept>

#include <iterator>
#include <string>
#include <vector>
#include <array>

#include <fstream>
#include <sstream>
#include <cstdint>

#include <wincodec.h>

#include "BasicVertexShader.h"	// シェーダーをコンパイルしたヘッダーファイル
#include "BasicPixelShader.h"


#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#pragma comment(lib, "windowscodecs.lib")

using namespace Microsoft::WRL;
using namespace DirectX;


// プロジェクト設定
struct ProjectSettings
{
    const std::wstring title = L"DirectX11-study";
    const int width = 1280;
    const int height = 720;
};

// COMオブジェクトの構造体
struct Dx11Context
{
    ComPtr<ID3D11Device>           device;              // GPUリソース（バッファ、テクスチャ、シェーダーなど）を作る
    ComPtr<ID3D11DeviceContext>    deviceContext;       // 作ったリソースを使って描画命令を発行する
    ComPtr<IDXGISwapChain>         swapChain;           // 画面に出すための表裏（バックバッファ）の入れ替え役
    ComPtr<ID3D11RenderTargetView> renderTargetView;    // 書き込み先の窓口（バックバッファに直接は書かず View を作ってOMに渡す）
};

// 頂点情報（CPU→GPUに渡す形）
struct Vertex
{
    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT2 uv;
};

// メッシュデータ
struct MeshData
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

// シェーダー構造体
struct Shaders
{
    ComPtr<ID3D11VertexShader> vertexShader;
    ComPtr<ID3D11PixelShader> pixelShader;
};

// 定数バッファ用構造体
struct ConstantPerFrame
{
    // WVP 行列
    DirectX::XMFLOAT4X4 worldMatrix;
    DirectX::XMFLOAT4X4 viewMatrix;
    DirectX::XMFLOAT4X4 projectionMatrix;
    DirectX::XMFLOAT4X4 worldViewProjectionMatrix;

    // ライト
    DirectX::XMFLOAT3 directional; float padding0;
    DirectX::XMFLOAT3 lightColor;  float padding1;
    DirectX::XMFLOAT3 ambient;     float padding2;
};
// デバッグ用
static_assert((sizeof(ConstantPerFrame) % 16) == 0, "Constant buffer size must be 16-byte aligned.");

// カメラ構造体
struct Camera
{
    XMFLOAT3 position  = { 0.0f, 1.0f, -5.0f };
    XMFLOAT3 up = { 0.0f, 1.0f, 0.0f };

    float fovY = XMConvertToRadians(60.0f);
    float aspect = 1280.0f / 720.0f;
    float nearZ = 0.1f;
    float farZ = 1000.0f;

    float yaw = 0.0f;
    float pitch = 0.0f;

    float moveSpeed = 3.0f;

    // マウス視点操作
    bool mouseLook = false;
    float mouseSensitivity = 0.0025f;


    // 真上を向けないようにクランプ
    static float ClampPitch(float pitch)
    {
        const float limit = XMConvertToRadians(89.0f);

        if (pitch > limit) {
            pitch = limit;
        }
        if (pitch < -limit) {
            pitch = -limit;
        }

        return pitch;
    }

    // 前方向取得
    XMVECTOR GetForward() const
    {
        const float cosPitch = cosf(pitch);
        const float sinPitch = sinf(pitch);
        const float cosYaw = cosf(yaw);
        const float sinYaw = sinf(yaw);

        // LH 想定
        XMVECTOR forward = XMVectorSet(cosPitch * sinYaw, sinPitch, cosPitch * cosYaw, 0.0f);
        return XMVector3Normalize(forward);
    }
};

// OBJファイル用
struct ObjIndex
{
    int positionIndex = 0;
    int uvIndex = 0;
    int normalIndex = 0;
};

// ウィンドウハンドル
HWND g_hWnd;


// 例外処理
static void ThrowIfFailed(HRESULT hr, const char* msg)
{
    if (FAILED(hr)) {
        throw std::runtime_error(msg);
    }
}

// ウィンドウプロシージャ
static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE: {
        const auto createStruct = reinterpret_cast<LPCREATESTRUCTW>(lParam);
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(createStruct->lpCreateParams));
        return 0;
    }
    case WM_CLOSE:
        if (MessageBoxW(hwnd, L"保存していないデータは破棄されます。", L"ゲームを終了しますか？", MB_YESNO) == IDYES) {
            DestroyWindow(hwnd);
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }
}


// ウィンドウ初期化
static void InitWindow(HINSTANCE hInstance, int nCmdShow, const ProjectSettings& settings)
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
    RegisterClassExW(&windowClass);

    // ウィンドウ作成（クライアント領域を Width * Height にしたいから枠分を足し引き）
    RECT rect{ 0, 0, settings.width, settings.height };
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

    g_hWnd = CreateWindowExW(0, windowClass.lpszClassName, L"DirectX11-study",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_hWnd) return;

    ShowWindow(g_hWnd, nCmdShow);
    UpdateWindow(g_hWnd);
}


// 初期化処理
static void InitD3D11(HWND hwnd, const ProjectSettings& settings, Dx11Context& dx)
{
    // 表示の仕様書のようなもの（バックバッファの形式や何枚持つか？）
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferDesc.Width = settings.width;
    swapChainDesc.BufferDesc.Height = settings.height;
    swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;   // 一般的なRGBA
    swapChainDesc.BufferDesc.RefreshRate = { 60, 1 };               // リフレッシュレート

    swapChainDesc.SampleDesc.Count = 1;     // MSAAなし（マルチサンプル・アンチエイリアス）
    swapChainDesc.SampleDesc.Quality = 0;   // MSAAの品質（使う場合をデバイス対応の品質レベルを調べる必要あり）

    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;    // バッファ（バックバッファ）の用途
    swapChainDesc.BufferCount = 2;          // 2枚のバッファ
    swapChainDesc.OutputWindow = hwnd;
    swapChainDesc.Windowed = TRUE;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;    // Present後の裏バッファの中身は保証しない
    swapChainDesc.Flags = 0;

    // デバッグレイヤー（間違った使い方をした際に出力してくれる）
    UINT createFlags = 0;
#if defined(_DEBUG)
    createFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // 現在のGPU（ドライバ）でどの機能レベルが使えるか
    D3D_FEATURE_LEVEL featureLevels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0
    };
    D3D_FEATURE_LEVEL createdFeatureLevel = D3D_FEATURE_LEVEL_11_0;

    // Device/DeviceContext/SwapChain を生成
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createFlags,
        featureLevels,
        _countof(featureLevels),
        D3D11_SDK_VERSION,
        &swapChainDesc,
        dx.swapChain.ReleaseAndGetAddressOf(),
        dx.device.ReleaseAndGetAddressOf(),
        &createdFeatureLevel,
        dx.deviceContext.ReleaseAndGetAddressOf()
    );
    ThrowIfFailed(hr, "D3D11CreateDeviceAndSwapChain Failed");

    // SwapChain が内部に持っているバックバッファを取り出す
    ComPtr<ID3D11Texture2D> backBuffer;
    hr = dx.swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
    ThrowIfFailed(hr, "Get BackBuffer Failed");

    // RenderTargetView 作成（バックバッファに対して書き込める窓口）
    hr = dx.device->CreateRenderTargetView(backBuffer.Get(), nullptr, dx.renderTargetView.ReleaseAndGetAddressOf());
    ThrowIfFailed(hr, "Create RenderTargetView Failed");

    // OM に出力先（RTV）を設定
    dx.deviceContext->OMSetRenderTargets(1, dx.renderTargetView.GetAddressOf(), nullptr);

    // ビューポート設定（クリップ空間の結果を画面上のどの領域に写すか）
    D3D11_VIEWPORT viewPort = {};
    viewPort.Width = (float)settings.width;
    viewPort.Height = (float)settings.height;
    viewPort.MinDepth = 0.0f;
    viewPort.MaxDepth = 1.0f;
    dx.deviceContext->RSSetViewports(1, &viewPort);
}


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

// ConstantBuffer 作成
ComPtr<ID3D11Buffer> g_constantBuffer;
static void CreateConstantBuffer(ID3D11Device* device)
{
    D3D11_BUFFER_DESC bufferDesc = {};
    bufferDesc.ByteWidth = sizeof(ConstantPerFrame);
    bufferDesc.Usage = D3D11_USAGE_DYNAMIC;             // 毎フレーム書き換える
    bufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;  // 定数バッファ
    bufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE; // CPU から書き込む
    bufferDesc.MiscFlags = 0;
    bufferDesc.StructureByteStride = 0;

    ThrowIfFailed(device->CreateBuffer(&bufferDesc, nullptr, g_constantBuffer.GetAddressOf()), "Create Constant Buffer Failed");
}

// VertexShader/PixelShader 作成
static void CreateShaders(Shaders& shaders, Dx11Context& dx)
{
    // VertexShader
    HRESULT hr = dx.device->CreateVertexShader(
        g_BasicVertexShader, std::size(g_BasicVertexShader), NULL, shaders.vertexShader.GetAddressOf());
    ThrowIfFailed(hr, "Create VertexShader Failed");

    // PixelShader
    hr = dx.device->CreatePixelShader(
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


// OBJ の 1要素を読む
static ObjIndex ParseObjVertexToken(const std::string& token)
{
    ObjIndex result = {};

    std::stringstream ss(token);
    std::string part;

    if (!std::getline(ss, part, '/')) {
        throw std::runtime_error("OBJ face parse failed: position index missing.");
    }
    result.positionIndex = std::stoi(part);

    if (!std::getline(ss, part, '/')) {
        throw std::runtime_error("OBJ face parse failed: uv index missing.");
    }
    result.uvIndex = std::stoi(part);

    if (!std::getline(ss, part, '/')) {
        throw std::runtime_error("OBJ face parse failed: normal index missing.");
    }
    result.normalIndex = std::stoi(part);

    return result;
}

// OBJ 読み込み
static MeshData LoadObj(const std::wstring& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open OBJ file.");
    }

    std::vector<XMFLOAT3> positions;
    std::vector<XMFLOAT2> uvs;
    std::vector<XMFLOAT3> normals;

    MeshData meshData = {};

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "v") {
            XMFLOAT3 p = {};
            ss >> p.x >> p.y >> p.z;
            positions.push_back(p);
        }
        else if (type == "vt") {
            XMFLOAT2 uv = {};
            ss >> uv.x >> uv.y;

            uvs.push_back(uv);
        }
        else if (type == "vn") {
            XMFLOAT3 n = {};
            ss >> n.x >> n.y >> n.z;
            normals.push_back(n);
        }
        else if (type == "f") {
            std::array<std::string, 3> tokens = {};
            ss >> tokens[0] >> tokens[1] >> tokens[2];

            for (int i = 0; i < 3; ++i) {
                ObjIndex objIndex = ParseObjVertexToken(tokens[i]);

                if (objIndex.positionIndex <= 0 || objIndex.positionIndex > static_cast<int>(positions.size())) {
                    throw std::runtime_error("OBJ position index out of range.");
                }
                if (objIndex.uvIndex <= 0 || objIndex.uvIndex > static_cast<int>(uvs.size())) {
                    throw std::runtime_error("OBJ uv index out of range.");
                }
                if (objIndex.normalIndex <= 0 || objIndex.normalIndex > static_cast<int>(normals.size())) {
                    throw std::runtime_error("OBJ normal index out of range.");
                }

                Vertex vertex = {};
                vertex.position = positions[objIndex.positionIndex - 1];
                vertex.uv = uvs[objIndex.uvIndex - 1];
                vertex.normal = normals[objIndex.normalIndex - 1];

                meshData.vertices.push_back(vertex);
                meshData.indices.push_back(static_cast<uint32_t>(meshData.indices.size()));
            }
        }
    }

    if (meshData.vertices.empty() || meshData.indices.empty()) {
        throw std::runtime_error("OBJ mesh is empty.");
    }

    return meshData;
}


// 描画処理（更新）
static void Render(Dx11Context& dx, const MeshData& meshData, Shaders& shaders, const ProjectSettings& settings, Camera& camera, float time)
{
    // ワールド行列を計算
    XMMATRIX world = XMMatrixIdentity();
    world *= XMMatrixScaling(1.0f, 1.0f, 1.0f);
    world *= XMMatrixRotationY(time);
    world *= XMMatrixTranslation(0.0f, 0.0f, 0.0f);
    // ビュー行列を計算
    XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&camera.position), camera.GetForward(), XMLoadFloat3(&camera.up));
    // プロジェクション行列を計算
    camera.aspect = (float)settings.width / (float)settings.height;
    XMMATRIX projection = XMMatrixPerspectiveFovLH(camera.fovY, camera.aspect, camera.nearZ, camera.farZ);

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
    HRESULT hr = dx.deviceContext->Map(g_constantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
    ThrowIfFailed(hr, "Map Constant Buffer Failed");
    memcpy(mapped.pData, &constantPerFrame, sizeof(constantPerFrame));
    dx.deviceContext->Unmap(g_constantBuffer.Get(), 0);


    // 画面クリア
    const float clearColor[4] = { 0 / 255.0f, 99 / 255.0f, 181 / 255.0f, 1.0f };
    dx.deviceContext->ClearRenderTargetView(dx.renderTargetView.Get(), clearColor);

    // 出力先（RTV）をセット
    dx.deviceContext->OMSetRenderTargets(1,dx.renderTargetView.GetAddressOf(), nullptr);

    // IA（Input Assembler）設定：Layout / VertexBuffer / IndexBuffer / Topology
    dx.deviceContext->IASetInputLayout(g_inputLayout.Get());

    UINT stride = sizeof(Vertex);
    UINT offset = 0;
    dx.deviceContext->IASetVertexBuffers(0, 1, g_vertexBuffer.GetAddressOf(), &stride, &offset);

    dx.deviceContext->IASetIndexBuffer(g_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

    dx.deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);    // 三角形

    // シェーダー設定
    dx.deviceContext->VSSetShader(shaders.vertexShader.Get(), nullptr, 0);
    dx.deviceContext->PSSetShader(shaders.pixelShader.Get(), nullptr, 0);

    // シェーダーにテクスチャとサンプラーを設定
    dx.deviceContext->PSSetShaderResources(0, 1, g_textureSRV.GetAddressOf());
    dx.deviceContext->PSSetSamplers(0, 1, g_samplerState.GetAddressOf());

    // シェーダーに定数バッファを設定（HLSL 側で register(b0) にしたのでスロット 0 に入れる）
    ID3D11Buffer* constantBuffers[] = { g_constantBuffer.Get() };
    dx.deviceContext->VSSetConstantBuffers(0, 1, constantBuffers);
    dx.deviceContext->PSSetConstantBuffers(0, 1, constantBuffers);

    // 描き込み
    dx.deviceContext->DrawIndexed(static_cast<UINT>(meshData.indices.size()), 0, 0);

    // 表示
    dx.swapChain->Present(1, 0);
}


// カメラ操作
static void UpdateCamera(Camera& camera, int mouseDx, int mouseDy, float deltaTime)
{
    // 見回し（マウス操作）
    camera.yaw += mouseDx * camera.mouseSensitivity;
    camera.pitch += -mouseDy * camera.mouseSensitivity;
    camera.pitch = camera.ClampPitch(camera.pitch);

    // 移動
    XMVECTOR position = XMLoadFloat3(&camera.position);
    XMVECTOR up = XMLoadFloat3(&camera.up);
    XMVECTOR forward = camera.GetForward();
    XMVECTOR right = XMVector3Normalize(XMVector3Cross(up, forward));

    const float velocity = camera.moveSpeed * deltaTime;

    if (GetAsyncKeyState('W') & 0x8000) {
        position += forward * velocity;
    }
    if (GetAsyncKeyState('S') & 0x8000) {
        position -= forward * velocity;
    }
    if (GetAsyncKeyState('D') & 0x8000) {
        position += right * velocity;
    }
    if (GetAsyncKeyState('A') & 0x8000) {
        position -= right * velocity;
    }

    if (GetAsyncKeyState('E') & 0x8000) {
        position += up * velocity;
    }
    if (GetAsyncKeyState('Q') & 0x8000) {
        position -= up * velocity;
    }

    XMStoreFloat3(&camera.position, position);
}


// エントリーポイント
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
    static const ProjectSettings settings = {
                .title = L"DirectX11-study",
                .width = 1280,
                .height = 720
    };
    Dx11Context dx = {};
    MeshData meshData = {};
    Shaders shaders = {};
    Camera camera = {};
    
    // COM 初期化
    ThrowIfFailed(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED), "CoInitializeEx failed");

    try {
        // ウィンドウ初期化
        InitWindow(hInstance, nCmdShow, settings);
        // D3D11初期化
        InitD3D11(g_hWnd, settings, dx);

        // シェーダー作成
        CreateShaders(shaders, dx);
        // インプットレイアウト作成
        CreateInputLayout(dx.device.Get());

        // メッシュ作成
        meshData = LoadObj(L"model.obj");

        // 頂点バッファ作成
        CreateVertexBuffer(dx.device.Get(), meshData);
        // インデックスバッファ作成
        CreateIndexBuffer(dx.device.Get(), meshData);
        // 定数バッファ作成
        CreateConstantBuffer(dx.device.Get());

        // テクスチャ作成
        CreateTextureFromFile(dx.device.Get(), L"test.jpg");
        // サンプラーステート作成
        CreateSamplerState(dx.device.Get());


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
                if (GetForegroundWindow() == g_hWnd && camera.mouseLook) {
                    POINT currentCursor = {};
                    GetCursorPos(&currentCursor);

                    mouseDx = currentCursor.x - cursor.x;
                    mouseDy = currentCursor.y - cursor.y;

                    cursor = currentCursor;
                }

                UpdateCamera(camera, mouseDx, mouseDy, deltaTime);
                Render(dx, meshData, shaders, settings, camera, time);
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
