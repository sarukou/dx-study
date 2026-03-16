# dx-study


Week0
DX11 / DX12 の最小表示確認までを再現できる状態にする。

Week1
DX11 で三角形の描画、GPUに何を送っているか説明できる状態にする。

Week2
DX11 でWVP行列、定数バッファを用いて三角形が回転する状態にする。

Week3
DX11 でカメラ実装を行い、視点移動ができる状態にする。

Week4
DX11 でライティング基礎の実装を行い、Lambert + Normal で陰影がつく状態にする。

Week5
DX11 でモデル描画、テクスチャ実装ができている状態にする。（OBJファイル）


## Projects

- `projects/DirectX11/DirectX11` : DirectX11 OBJモデル表示（テクスチャ＋ライティング最小、Y軸回転、視点移動）

- `projects/DirectX12/DirectX12` : 

- `projects/DirectX12/D3D12HelloTriangle` : DirectX12 HelloTriangle（表示確認）



## Requirements

- Windows 10/11

- Visual Studio 2022

  - Workloads:

    - Desktop development with C++

    - (Recommended) Game development with C++

- Windows SDK (10/11)

- (Recommended) Optional Feature: Graphics Tools



## Build \& Run

### DX11

1. `projects/DirectX11/` の `.sln` を開く

2. `Debug | x64` にする

3. F5で実行



### DX12

1. `projects/DirectX12/` の `.sln` を開く

2. `Debug | x64` にする

3. F5で実行



## Notes / Troubleshooting

- x86ではなくx64でビルドする

- DX12 Debug Layer を使う場合は Graphics Tools が必要

