# Week8 log



## Done

- 簡易PBR（GGX/Fresnel/Roughness/Metallic）を実装できている状態にする。


- PBR用のマテリアル値を追加する

- Lambert をやめて、PBR風の Diffuse + Specular に置き換える

- Fresnel を入れる

- Roughness を使ってハイライトの広がりを変える

- Geometry項を入れて簡易Cook-Torranceにする

- Metallic で Diffuse と Specular の配分を変える

- 調整用のキー切り替えを用意する

最終的に、NormalMap / Metallic / Roughness / BaseColor を切り替えながら、見た目がどのように変化するか確認できるようにした。


## Environment

- OS: Windows 11
- VS: Visual Studio 2022
- SDK: Windows SDK 10.0.22621.0



## Issues \& Fixes

- Lambert から PBRへの置き換え方が分かりにくかった

　　最初は Lambert の NdotL ベースの考え方に慣れていたため、PBRでなぜ V や H が必要なのかが分かりにくかった。

　　対処：まずは一気に完全なPBRに行かず、

　　　　Lambert, Diffuse + Specular, Fresnel,Roughness,Geometry, Metallic という順番で段階的に追加した。

　　　　この進め方にしたことで、「どの要素が見た目に何を与えているのか」が理解しやすくなった。

- worldPos / cameraPosition が必要になる点

　　Specular を視線方向ベースで計算するためには、

　　V = cameraPosition − worldPos が必要になる。

　　そのため、頂点シェーダーから worldPos を渡し、定数バッファに cameraPosition を追加する必要があった。

　　対処：VSOutput に worldPos を追加、ConstantBuffer に cameraPosition を追加、CPU側で毎フレームcameraPosition を更新という形に整理した。

- Roughness の扱い

　　Roughness を 0 に近づけすぎると、ハイライトが極端になったり数値が不安定になったりした。

　　対処：roughness = clamp(Roughness, 0.05f, 1.0f) として、安全な範囲に丸めた。

- 分母が 0 に近づく問題

　　Cook-Torrance の式では、4 * NdotV * NdotL が分母に来るため、角度によっては 0 に近づいて不安定になる可能性がある。

　　対処：max(4.0f \\\* NdotV \\\* NdotL, 0.0001f) として、最小値を設けた。

- Diffuse の扱い

　　最初は diffuse = albedo のようにしていたが、PBRらしくするには Lambert の拡散反射を albedo / PI にする必要があった。

　　対処：diffuse = albedo / π に修正した。

　　この時、「PI は何か？」という疑問が出たが、円周率 π であり、PBR式の正規化に必要な定数であると理解した。（後述）


## Learning

- Lambert（おさらい）：

　　Lambert は、法線と光方向の内積を使って「どれだけ光が当たっているか」を計算する。

　　diffuse = albedo \* lightColor \* saturate(dot(N, L))

　　N : 法線ベクトル

　　L : 光方向ベクトル

　　dot(N, L) : 法線と光の向きの一致度

　　saturate : 0〜1 に丸める

　　この方式は実装が簡単で、物体が明るくなる向き・暗くなる向きを理解しやすいが、表面の「材質らしさ」までは表現しにくい。

　　（例）金属らしい反射、表面のツルツル感 / ザラザラ感、視線角による反射の変化など

などは Lambert 単体では表現しづらい。

- PBR風 Diffuse + Specular

　　Lambert から次に進めるため、まずライティングを Diffuse, Specular の2成分に分けた。

　　Lo = Diffuse + Specular

　　これにより、「表面色による反射」と「鏡面反射」を分離して扱えるようになった。

　　ベクトル：

　　　　この段階で、視線方向 V、光方向 L、ハーフベクトル H を導入した。

　　　　V：カメラ方向 

　　　　　　V = normalize(cameraPosition - worldPos)

　　　　L : ライト方向

　　　　　　L = normalize(-Directional)

　　　　H : 光方向と視線方向の中間方向

　　　　　　H = normalize(L + V)

- ピクセルシェーダーでやっていること（確認）

　　現在のピクセルシェーダーでは、概ね以下の順序で処理している。

　　アルベドの取得：

　　　　アルベドテクスチャをサンプリングし、さらに BaseColor を掛けて最終的なベース色を作る。

　　　　albedo = textureColor.rgb * BaseColor

　　法線の決定：

　　　　頂点シェーダーから受け取った法線を正規化する。

　　　　normal = normalize(vsOutput.normal)

　　　　NormalMap を使う場合は、テクスチャから法線を取り出して [-1, 1] に変換する。

　　　　normalSample = normalTexture * 2 - 1

　　　　その後、TBN 行列で tangent space の法線を world space に変換する。

　　　　TBN = [T, B, N]

　　　　finalNormal = normalize(normalSample * TBN)

　　　　T : tangent

　　　　B : bitangent

　　　　N : normal

　　　　これにより、NormalMap の凹凸情報をライティングに反映できるようになる。

　　各種ベクトルの計算：

　　　　法線 N、光方向 L、視線方向 V、ハーフベクトル H を求め、内積を計算する。

　　　　N = normalize(finalNormal)

　　　　L = normalize(-Directional)

　　　　V = normalize(CameraPosition - worldPos)

　　　　H = normalize(L + V)

　　　　NdotL = saturate(dot(N, L))

　　　　　　面の法線とライト方向がどれくらい近いか。光が表面にどれくらい効くかを表す。物理的に自然な明るさにするために必要。

　　　　NdotV = saturate(dot(N, V))

　　　　　　面の法線と視線方向がどれくらい近いか。視線方向から見たときに、マイクロファセット反射がどれくらい見えるかに関係する。視線角度によるスペキュラの見え方・遮断・分母補正に必要。

　　　　NdotH = saturate(dot(N, H))

　　　　　　面の法線とハーフベクトルがどれくらい近いか。分布関数Dに必要。スペキュラハイライトの中心・形・鋭さに大きく関わる。

　　　　VdotH = saturate(dot(V, H))

　　　　　　視線方向とハーフベクトルがどれくらい近いか。Fresnel項Fに必要。見る角度によって反射の仕方が変わるという実装の近似に使う。

- Fresnel

　　Fresnel は正面から見ると反射は比較的弱く、斜めから見ると反射が強くなるという「視線角によって反射率が変わる現象」を表す。

　　現実の材質ではこの性質がとても重要で、PBRでも必須の要素の1つ。

　　今回は Schlick 近似を使って実装した。

　　FresnelSchlick の式：

　　　　F = F0 + (1 - F0)(1 - VdotH)^5

　　　　F0 : 正面から見たときの基本反射率

　　　　VdotH : 視線とハーフベクトルの内積

　　F0 の計算：

　　　　F0 = lerp((0.04, 0.04, 0.04), albedo, metallic)

　　　　非金属では F0 ≒ 0.04 を使うことが多い。

　　　　金属ではアルベド色が反射色に近づくため、metallic に応じて albedo を混ぜている。

　　Fresnel を入れることで、正面では落ち着いた反射・斜めでは反射が強く見えるようになり、見た目が一気に自然になる。

- Roughness

　　Roughness は表面の粗さを表す値である。

　　roughness = 0 に近い → ツルツル → ハイライトが鋭い

　　roughness = 1 に近い → ザラザラ → ハイライトが広がる

　　実装では、極端な不安定さを避けるために clamp を入れた。

　　roughness = clamp(Roughness, 0.05, 1.0)

　　この値は GGX 分布関数や Geometry 項の中で使われる。

- Geometry 項

　　Geometry 項は、マイクロファセット同士の自己遮蔽や見え方の減衰を表す。

　　ざっくりとした説明では、表面の細かい凹凸を考えたときに、一部が光を遮る・一部が視線から見えにくくなるといった効果を近似する項である。

　　今回は GeometrySchlickGGX と GeometrySmith を使った。

　　GeometrySchlickGGX：

　　　　geometry = NdotX / {NdotX(1-k) + k}

　　　　k = (roughness + 1)^2 / 8

　　GeometrySmith：

　　　　G = SchlickGGX(NdotV, roughness) * SchlickGGX(NdotL, roughness)

　　この Geometry 項を入れることで、Specular が不自然に強くなりすぎるのを抑え、見た目がより自然になる。

- GGX 分布

　　GGX 分布関数は、表面のマイクロファセットがどの方向を向いているかの分布を表す。

　　DistributionGGX：

　　　　a = roughness^2

　　　　D = a^2 / PI * (NdotH^2(a^2 - 1) + 1)^2

　　これにより、roughness が低いと鋭いハイライト、roughness が高いと広いハイライトになる。

　　以前の pow(NdotH, specPower) ベースの簡易Specularよりも、ハイライトの出方がかなり自然になる。

- Cook-Torrance Specular

　　最終的な Specular は次の式で求めている。

　　Specular = (D * F * G)(4 * NdotV * NdotL)

　　D：GGX分布

　　F：Fresnel

　　G：Geometry項

　　max(..., 0.0001f) を入れているのは、0除算や極端な値の暴れを防ぐためである。（DistributionGGXやGeometrySchlickGGXも該当）

- Metallic

　　Metallic は、その材質が「非金属」か「金属」かを表す値である。

　　metallic = 0 → 非金属 → Diffuse がある。

　　metallic = 1 → 金属 → Diffuse がほぼ無くなり、Specular 主体になる。

　　PBRではエネルギー配分の考え方として、

　　kS = F

　　kD = 1−kS

　　さらに金属では拡散反射を減らすために、

　　kD = (1−kS)(1−metallic)

　　最終的な Diffuse は、

　　Diffuse = kD * albedo / π

　　これにより、Metallic を上げると Diffuse が減り、金属らしい反射主体の見た目に変わるようになる。

- 最終的な色の合成

　　最終的な色は、Ambient と、Diffuse + Specular にライトを掛けて合成している。

　　finalRGB = ambient + (diffuse + specular) * LightColor * NdotL

　　Ambient は単純化して、ambient = albedo * Ambient としている。

- PI

　　PBRの式では PI が何度も出てくる。

　　PI=3.14159265、これは円周率 π のことである。

　　今回の実装では主に以下で使った。

　　Diffuse：

　　　　diffuse = kD * albedo / π

　　　　Lambert 拡散反射の正規化。半球全体に光を散らしても反射量が増えすぎないようにしている。


　　GGX Distribution：

　　　　D = a^2 / PI * (NdotH^2(a^2 - 1) + 1)^2

　　　　GGX 分布の正規化。マイクロファセットの向きの分布として（分布の総量）物理的に正しい量にしている。

	​最初は「なぜここで円周率が出るのか」が直感的に分かりにくかったが、PBRの式の正規化に必要な定数として使われていると理解した。

- 各キーで見えたこと

　　調整用にキー入力で値を切り替えられるようにした。

　　N : NormalMap ON / OFF

　　　　OFF：表面が滑らかで、法線はモデル本来のものだけになる。ハイライトも大きく均一に見える。

　　　　ON：表面の細かい凹凸が見える。ハイライトの出方が細かく変化し、ディテール感が増す。

　　　　NormalMap はライティングの見た目にかなり効いており、PBRとの相性も良かった。

　　M : Metallic 切り替え

　　　　Metallic = 0.0：非金属らしい見た目。Diffuse がしっかり残る。

　　　　Metallic = 0.5：中間的な材質。Diffuse と Specular のバランスが変わる。

　　　　Metallic = 1.0：金属らしい見た目。Diffuse が大きく減り、反射主体になる。

　　　　BaseColor の色が Specular 側に影響するため、特に金属時は色の違いが分かりやすかった。

　　R : Roughness 切り替え

　　　　Roughness = 0.1：ツルツル。ハイライトが鋭い。

　　　　Roughness = 0.4：中間的。

　　　　Roughness = 0.8：ザラザラ。ハイライトが広がる

　　　　Roughness は「材質感」にかなり直結する値であり、見た目変化が分かりやすかった。

　　C : BaseColor 切り替え

　　　　白系：素直な見た目確認に向く。

　　　　赤系：金属化した時に反射色が赤くなりやすく、変化が分かりやすい。

　　　　黄系：金属っぽい色味の確認に向いていた。

　　　　BaseColor 単体でも見た目は変わるが、Metallic と組み合わせた時の変化が特に分かりやすかった。

- まとめ

　　Fresnel は視線角で反射率を変える重要な要素。

　　Roughness はハイライトの広がりを決める重要なパラメータ。

　　Geometry 項は見た目を自然にするために必要。

　　Metallic は Diffuse / Specular の配分を変える役割を持つ。

　　NormalMap と PBRを組み合わせると、表面ディテールがかなり良く見える。

　　worldPos や cameraPosition が必要になるため、シェーダーだけでなくCPU側のデータ設計も重要になる。

