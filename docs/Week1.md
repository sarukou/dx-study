# Week1 log


## Done

- DX11: 三角形描画、シェーダー最小構成


## Environment

- OS: Windows 11

- VS: Visual Studio 2022

- SDK: Windows SDK 10.0.22621.0


## Issues & Fixes

- 背景色は変わるが、三角形が表示されない

　　特定手順：Viewportを毎フレーム設定にしても変化なし→RS/Viewport が原因ではない

　　　　　　　SV_VertexIDで頂点バッファ/InputLayout を無視した描画と試す→表示された

　　　　　　　VertexBuffer/InputLayout 側が原因と確定

　　原因：CreateBuffer(&bufferDesc, nullptr, &g_vertexBuffer)にしており初期データを渡していなかった。

　　　　　バッファ内容が未定義のため頂点がクリップ外/NaN等になり、描画されない。

　　修正手順：D3D11_SUBRESOURCE_DATA の pSysMem にvertics を代入

　　　　　　　CreateBuffer() でデータを渡す

　　　　　　　代替案（UpdateSubResource や Map/Unmap）


## Learning

- SV_VertexID

　　SV_VertexIDを使って処理をする頂点を指定することで頂点頂点バッファ（VB）とInputLayout を丸ごと無視できる。

　　今回のような問題の原因が入力（VB/InputLayout）側なのかそれ以外（Viewport/OM/Shader）なのかを切り分けることができる。

- InputLayout

　　頂点バッファの生データ（バイト列）を頂点シェーダーが期待する入力（セマンティクス付きの変数）に変換するための変換表/解釈ルール。

　　Vertex構造体の並び（バイトの並び）のどこが POSITIONで どこが COLOR なのかを VS に教えるのがInputLayout

　　バイトコードから入力シグネチャを読み取り、Layout 側の型が合っているか、セマンティクス名・インデックスが合っているかをチェックし、正しく繋げられる時だけInputLayout が作成される。
　　

