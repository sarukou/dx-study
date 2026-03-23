# Week7 log


## Done

- 法線マッピングを追加して細かい凹凸を表現できる状態にする。


## Environment

- OS: Windows 11

- VS: Visual Studio 2022

- SDK: Windows SDK 10.0.22621.0


## Issues & Fixes


## Learning

- 法線マッピング

　　法線マッピングは、テクスチャに記録された法線方向を使って、ピクセル単位でライティング用の法線を変化させる手法。

　　実際にメッシュの形状を変えているわけではなく、光の当たり方だけを変えることで凹凸があるように見せている。

　　通常のライティングでは頂点法線を補間した法線を使うため、表面は滑らかにしか見えないが、normal map を使うと、表面の細かい傷やレンガの凹凸のような情報を、追加の頂点なしで表現できる。

　　なぜ Tangent が必要か：

　　　　normal map に入っている法線は、ワールド空間やモデル空間の法線ではなく、Tangent Space（接空間） で表現されているため、normal map の値をそのままライティングに使うことはできない。

　　　　Tangent Space では、各頂点ごとに以下の3本の軸を基準にする。

　　　　・Tangent　UV の U 方向に対応する3D空間の向き

　　　　・Bitangent　UV の V 方向に対応する3D空間の向き

　　　　・Normal　面の法線方向

　　　　normal map の RGB は、この Tangent Space 上の法線ベクトルを表している。

　　　　つまり、normal map から取り出した法線をワールド空間のライティングに使うには、Tangent / Bitangent / Normal を使って座標変換する必要がある。

　　　　このため、頂点に tangent を持たせる必要があった。

　　TBN 行列：

　　　　TBN 行列は、Tangent Space のベクトルをワールド空間（またはモデル空間）へ変換するための基底行列。

　　　　Tangent / Bitangent / Normal の3本を並べた行列を作り、normal map から取り出した法線ベクトルに掛けることで、
ライティングに使える空間の法線へ変換する。

　　normal map の値をそのまま使えない理由：

　　　　normal map の画素値は色として保存されているため、範囲は [0, 1]。

　　　　一方、法線ベクトルの成分は [-1, 1] を取る必要があるため、シェーダーでは normalSample = normalSample * 2.0f - 1.0f; として変換した。

　　　　この処理を入れないと、法線ベクトルとして意味のある値にならず、ライティング結果が壊れる。

