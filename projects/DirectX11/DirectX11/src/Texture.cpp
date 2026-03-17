#include "Texture.h"

#include "Renderer.h"
#include "Utility.h"

#include <vector>
#include <cstdint>

using namespace Microsoft::WRL;

void Texture::Initialize(Renderer& renderer, const wchar_t* filePath)
{
    CreateTextureFromFile(renderer, filePath);
    CreateSamplerState(renderer);
}

void Texture::CreateTextureFromFile(Renderer& renderer, const wchar_t* filePath)
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
        renderer.GetDevice()->CreateTexture2D(&textureDesc, &initData, &texture),
        "Create Texture2D Failed"
    );


    // SRV（シェーダーリソースビュー）を作成（シェーダーから読むためのビュー（HLSLで .Sample() が使える））
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = textureDesc.Format;                    // どのフォーマットとしてシェーダーから見るか
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;  // 2D, 2DArray, Cube, 3D など、いろいろな見え方の指定
    srvDesc.Texture2D.MostDetailedMip = 0;                  // ミップレベル（0が細かくて元画像）
    srvDesc.Texture2D.MipLevels = 1;                        // ミップマップ段数

    ThrowIfFailed(
        renderer.GetDevice()->CreateShaderResourceView(texture.Get(), &srvDesc, m_textureSRV.GetAddressOf()),
        "Create ShaderResorceView Failed"
    );
}

void Texture::CreateSamplerState(Renderer& renderer)
{
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;   // 補間方法（縮小時、拡大時、ミップ切り替え時の場面に対してLinear）
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;      // U方向（横方向）で UV が 0～1 を超えたときの扱い（WARP は繰り返し）
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;      // V方向（縦方向）
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;       // W方向（3Dテクスチャ用）
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
        renderer.GetDevice()->CreateSamplerState(&samplerDesc, m_samplerState.GetAddressOf()),
        "Create SamplerState Failed"
    );
}

void Texture::Bind(Renderer& renderer, UINT slot) const
{
    ID3D11ShaderResourceView* srv = m_textureSRV.Get();
    renderer.GetDeviceContext()->PSSetShaderResources(slot, 1, &srv);

    ID3D11SamplerState* sampler = m_samplerState.Get();
    renderer.GetDeviceContext()->PSSetSamplers(slot, 1, &sampler);
}