# Week2 log


## Done

- DX11: WVP行列/定数バッファを用いて三角形回転


## Environment

- OS: Windows 11

- VS: Visual Studio 2022

- SDK: Windows SDK 10.0.22621.0


## Issues & Fixes

　　今回の実装では特に問題はなかった。


## Learning

- cbuffer/register(b0)/SV_POSITION/SV_TARGET について

　　cbuffer：

　　　　GPU に毎フレーム/毎秒がで渡したい定数をまとめる入れ物。（WVP、カメラ位置、ライト、時間、色など）

　　　　GPU 側から見ると守りの塊（Buffer）で、シェーダーはそこを読み取りに行く。

　　　　CPU（C++）側でID3D11Buffer を作ってUpdateSubResource やMap/Unmap で中身を書き換える。

　　register(b0)：

　　　　リソースをどのスロット番号に結び付けるかの指定。

　　　　cbuffer は bレジスタにバインドされる。（b0,b1, ...）

　　　　テクスチャはt0、サンプラはs0、UAV はu0 のように種類ごとに接頭辞が違う。

　　　　C++ 側でVSSetConstantBuffers(0, 1, &buffer); のように番号を一致させる必要がある。（第一引数）

　　SV_POSITION：

　　　　頂点シェーダーが出力するクリップ空間の座標。（これがないとGPU が画面のどこに描くか決められない。）

　　　　ラスタライズが参照する公式な位置情報であり、画面にどう投影するかの基準。

　　　　値は通常クリップ空間で出し、その後GPU が透視除算してNDC（正規化デバイス座標）→Viewport へ変換する。

　　SV_TARGET：

　　　　ピクセルシェーダーが出力する描画結果の色。

　　　　多くの場合バックバッファ（あるいはGBuffer 等のRenderTarget）に書き込まれる。

　　　　RenderTarget が複数ある場合（Multiple Render Target）はSV_TARGET0、SV_TARGET1 のように指定する。

- WVP 行列について

　　World 行列がやっていること：

　　　　ローカル座標をワールド座標に移す変換。主に三つの変換からなる

　　　　Scale（拡大縮小）、Rotation（回転）、Translation（平行移動）

　　　　行列は掛け算順序が変わると意味が変わる。World 行列では S * R * T が基本

　　　　掛け算順序が変わると回転軸のずれや移動ベクトルが拡大される問題が起こる。

　　View 行列がやっていること：

　　　　ワールド空間の点をカメラから見た座標系（視点座標系）に変換する。

　　　　World では物体を動かすことだったが、View はCameraWorld の逆行列であり、数学的にはカメラを動かすのではなく世界をカメラを基準に世界を逆に動かすこと。

　　　　XMMatrixLookAtLHではeye（カメラ位置）、at（注視点）、up（上方向の基準）を指定し、カメラの座標系を作ってそこへ座標変換をしている。

　　　　forward（z軸）、right（x軸）、up（y軸）の3つの軸を作りカメラの向きを決め、その後点P（ワールド）をカメラ原点から見た相対位置にし、それをforward/right/up方向に射影（内積）してカメラ空間座標を得る。

　　Projection 行列がやっていること：

　　　　カメラ区間の3D座標をクリップ空間に移して透視（遠いほど小さい）と、描画範囲（near/far・画角）を決める。

　　　　XMMatrixPerspectiveFovLH（透視投影行列）はFOV（画角）とAspect（縦横比）、Near/Far（描画距離）を固定する。

　　　　Projection行列では視錐台を正規化された箱（クリップ空間）に押し込む、w 成分にz を混ぜ、その後（GPU側で）にw割りをすることで透視を実現している。

　　　　FOV：

　　　　　　大きくすると広角、小さくすると望遠（near面の大きさ）

　　　　Aspect：

　　　　　　画面比率で言え方が変わるのを整える

　　　　Near/Far：

　　　　　　（1）見える範囲を切る（クリッピング）

　　　　　　　　z < near のものはカメラに近すぎるので描かない、z > far のものは遠すぎて描かない。

　　　　　　（2）深度バッファの精度配分を決める。

　　　　　　　　深度バッファは z の値をそのまま保存していない（深度値は距離 z に対して線形じゃない）

　　　　　　　　基本的に近距離は深度の分解能が密集し、遠距離では分解能がスカスカ

　　　　　　　　そのため近くは細かく区別できるけど遠くは同じ深度につぶれやすい。

　　　　　　　　near が極端に小さいと near 付近に制度が吸われて中～遠距離の精度が足りなくなる。これが Z-fighting（ちらつき）の原因の一つ。

- Map/Unmap

　　ID3D11Buffer（GPUリソース）は基本的に GPU 側のメモリ（VRAM やそれに近い領域）にあり、CPU がそこに勝手に memcpy できるわけではない。

　　そのため CPU が書き込める一時的な窓口（ポインタ）をもらうのが Map、書き込みが終わったことを知らせて窓口を閉じるのが Unmap

　　Map() では書き込みたい対象、サブリソース、中身をどうするか（今回は全面書き換え）、フラグ、出力先を指定する。

　　memcpy() で実際に CPU がバッファに書き込みをする。

　　最後に Unmap() で書き込み終了を通知して確定させる。（Unmap しないと GPU 側から見て行進が確定しない。）

　　この Map/Unmap が成立するためにはバッファが作成時に Usage = D3D11_USAGE_DYNAMIC、CPUAccessFlags = D3D11_CPU_ACCESS_WRITE のようになっている必要がある。
　　　　　　　　　　　　　　　　　　　　　　　 
