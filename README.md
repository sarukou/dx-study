# dx-study


Week0
DX11 / DX12 の最小表示確認までを再現できる状態にする。

Week1
DX11 で三角形の描画、GPUに何を送っているか説明できる状態にする。


## Projects

- `projects/DirectX11/DirectX11` : DirectX11 最小（クリア色 + Present）

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

