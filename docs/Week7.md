# Week7 log


## Done

- 法線マッピングを追加して細かい凹凸を表現できる状態にする。


## Environment

- OS: Windows 11

- VS: Visual Studio 2022

- SDK: Windows SDK 10.0.22621.0


## Issues & Fixes


## Learning

1. 法線マッピングとは何か

法線マッピングは、テクスチャに記録された法線方向を使って、ピクセル単位でライティング用の法線を変化させる手法。
実際にメッシュの形状を変えているわけではなく、光の当たり方だけを変えることで凹凸があるように見せている。

通常のライティングでは頂点法線を補間した法線を使うため、表面は滑らかにしか見えない。
しかし normal map を使うと、表面の細かい傷やレンガの凹凸のような情報を、追加の頂点なしで表現できる。

2. なぜ Tangent が必要なのか

normal map に入っている法線は、ワールド空間やモデル空間の法線ではなく、Tangent Space（接空間） で表現されている。
そのため normal map の値をそのままライティングに使うことはできない。

Tangent Space では、各頂点ごとに次の3本の軸を基準にする。

Tangent: UV の U 方向に対応する3D空間の向き
Bitangent: UV の V 方向に対応する3D空間の向き
Normal: 面の法線方向

normal map の RGB は、この Tangent Space 上の法線ベクトルを表している。
つまり、normal map から取り出した法線をワールド空間のライティングに使うには、
Tangent / Bitangent / Normal を使って座標変換する必要がある。

このため、頂点に tangent を持たせる必要があった。

3. TBN 行列とは何か

TBN 行列は、Tangent Space のベクトルをワールド空間（またはモデル空間）へ変換するための基底行列。

T = Tangent
B = Bitangent
N = Normal

の3本を並べた行列を作り、normal map から取り出した法線ベクトルに掛けることで、
ライティングに使える空間の法線へ変換する。

今回の実装では、bitangent は頂点に持たず、シェーダー内で

float3 B = normalize(cross(N, T));

から求めた。
その後、

float3x3 TBN = float3x3(T, B, N);
float3 bumpedNormal = normalize(mul(normalSample, TBN));

の形で接空間法線をワールド空間法線へ変換した。

4. normal map の値をそのまま使えない理由

normal map の画素値は色として保存されているため、範囲は [0, 1]。
一方、法線ベクトルの成分は [-1, 1] を取る必要がある。

そのため、シェーダーでは

normalSample = normalSample * 2.0f - 1.0f;

として変換した。
この処理を入れないと、法線ベクトルとして意味のある値にならず、ライティング結果が壊れる。

5. 今回の実装内容

今回の実装では以下を行った。

Vertex / MeshData に tangent を追加
ObjLoader で三角形ごとの tangent を計算
各三角形の tangent を共有頂点へ加算し、最後に正規化
tangent を InputLayout と HLSL の TANGENT semantic に追加
albedo texture と normal map を別スロットで bind
Pixel Shader で normal map をサンプリングし、TBN 行列で変換
normal map の ON / OFF を切り替えられるようにした
6. tangent の計算方法

tangent は、三角形の位置差分と UV 差分から計算した。
考え方としては、UV の U 方向へ進んだときに、3D空間上でどの方向へ進むか を求めている。

1つの三角形について

edge1 = p1 - p0
edge2 = p2 - p0
deltaUV1 = uv1 - uv0
deltaUV2 = uv2 - uv0

を使い、そこから tangent を計算した。
さらに、頂点は複数の三角形で共有されるため、面ごとの tangent を各頂点に加算し、最後に正規化した。

また、normal と tangent が完全に直交しない場合があるため、
Gram-Schmidt の形で tangent から normal 成分を除去して直交化した。

7. 実装中に起きた問題と解決
問題1: normal map を読んでも見た目が変わらない

原因
normal map を texture として読み込んでも、Pixel Shader 側で法線として使っていなかった。

解決
normal map を t1 に bind し、Pixel Shader 側でサンプリングして TBN 変換した法線をライティングに使うようにした。

問題2: normal map を表示すると青紫の画像が出るだけだった

原因
これは異常ではなく、normal map をそのまま色として表示した状態だった。
normal map は色画像ではなく、法線方向を RGB にエンコードしたデータ。

解決
normalSample * 2 - 1 でベクトルへ変換し、さらに TBN でワールド空間へ変換した上でライティングに使用した。

問題3: 凹凸が逆に見える可能性があった

原因
bitangent の向きや normal map の Y 方向の流儀が素材と一致していないと、出っ張りとへこみが逆転することがある。

解決
必要に応じて

cross(N, T) と cross(T, N) を見直す
normalSample.y = -normalSample.y; を試す

ことで調整できるように理解した。

問題4: tangent が不安定になる可能性があった

原因
UV 差分がほぼゼロの面では tangent の計算が不安定になる。
また、面ごとに計算した tangent をそのまま使うと共有頂点で不連続になりやすい。

解決
分母が極端に小さい場合は計算をスキップし、
共有頂点へ加算平均したあとに正規化した。
さらに normal と直交化して安定させた。

8. ON / OFF 比較で確認できたこと

normal map を OFF にすると、頂点法線ベースの滑らかな陰影になる。
ON にすると、メッシュ形状自体は変わっていないのに、表面に細かい凹凸があるような陰影変化が現れた。

この比較により、法線マッピングが形状そのものではなくライティング結果を変える技術であることを確認できた。

9. 今回学んだこと
normal map の法線はワールド空間ではなく Tangent Space にある
そのため tangent が必要になる
TBN 行列は Tangent Space とワールド空間をつなぐ基底変換
normal map の RGB は色ではなく法線方向のエンコード値
実装では「数式を書くこと」よりも、「空間をそろえること」が重要
ON / OFF 比較を作ることで、実装確認と理解の両方がしやすくなる
10. 今後改善したいこと
bitangent の符号まで含めたより厳密な tangent 管理
normal / tangent の変換を、非一様スケールを考慮した形へ改善
Material クラスを導入して albedo / normal などを整理
PBR へ進んだときに roughness / metallic map とあわせて扱える構成へ発展させる