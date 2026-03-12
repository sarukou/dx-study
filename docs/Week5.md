# Week5 log


## Done

- DX11: モデル描画ができるようになる（OBJファイル）


## Environment

- OS: Windows 11

- VS: Visual Studio 2022

- SDK: Windows SDK 10.0.22621.0


## Issues & Fixes


## Learning

- constexpr修飾子

　　コンパイル時に確定できる値や関数のこと。

　　const は変更しない値（定数）だが、constexpr は変更しない（定数）だけでなく、できればコンパイル時に確定する値。

　　constexpr は関数にもつけることができ、その関数はコンパイル時にも計算できる形で書かれているという関数の意味。

　　必ずしもコンパイル時にしか使えないという意味ではなく普通の関数として使うこともできる。

　　安全に定位数として使うことができる、無駄な実行時計算を減らすことができるなどのメリットがある。

　　変数、関数、コンストラクタ、メンバ関数などいろんなところで使える。

　　似たものとして consteval があるが、これは必ずコンパイル時に計算しなければならないという意味。

- セマンティクス

　　セマンティクスは、HLSL 側で使う「意味ラベル」

　　（例）POSITION、NORMAL、TEXCOORD

　　変数名そのものではなく、「この値は位置です」「この値は法線です」という意味を表すタグ

　　HLSL 側は、POSITION セマンティクスが付いているから位置と判断している。

　　セマンティクス名が C++ と HLSL 側で一致しないと対応できず、InputLayout 作成に失敗したり、正しく頂点が渡らない。

　　SemanticIndex：

　　　　POSITION や TEXCOORD の後ろには番号をつけることができる。

　　　　C++ 側：

　　　　　　{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },

　　　　　　{ "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },

　　　　HLSL 側：

　　　　　　struct VSInput { float2 uv0 : TEXCOORD0; float2 uv1 : TEXCOORD1; };

　　　　これは TEXCOORD0 → 1枚目のUV、TEXCOORD1 → 2枚目のUVという意味。

　　よく使う代表的なセマンティクス：

　　　　POSITION （頂点位置）

　　　　NORMAL （法線）

　　　　TEXCOORD （UV座標や、その他の補助データにもよく使う。）

　　　　COLOR （頂点カラー）

　　　　TANGENT / BINORMAL （法線マップ用で使うことがある。）

　　SV_ で始まるものとの違い：

　　　　float4 position : SV_POSITION;

　　　　の SV_POSITION は、普通の入力セマンティクスとは少し違う。

　　　　これは System Value Semantic と呼ばれており、GPU パイプライン上で特別な意味を持つセマンティクス。

　　　　（例）SV_POSITION、SV_TARGET、SV_VERTEXID、SV_INSTANCEID

　　　　頂点シェーダーの出力で SV_POSITION と書くが、その意味は「この値は最終的なクリップ空間座標です」という意味。

　　　　POSITION, NORMAL, TEXCOORD → 頂点データの意味を表す通常セマンティクス

　　　　SV_POSITION, SV_TARGET → GPU が特別扱いするシステム値セマンティクス

　　セマンティクスは、頂点データの各部品に貼るラベル。

　　InputLayout は、そのラベルが頂点バッファのどの位置にあるかを説明する表。

　　HLSL はそのラベルを見て、どの値をどの変数に受け取るかを決める。

- OBJについて

　　OBJは、3Dモデルの形状をテキスト形式で記述するファイル形式のひとつ。
　　頂点座標、UV、法線、面情報などを人間が読める形で保存できる。

　　OBJの要素：

　　　　v 頂点座標（position）

　　　　vt UV座標

　　　　vn 法線ベクトル（normal）

　　　　f 面情報

　　　　OBJには他にもマテリアル参照（mtllib/usemtl）やグループ、オブジェクト情報（g/o）などがある。

　　OBJとDirectXで使う頂点データの違い：

　　　　OBJファイルは、position / uv / normal を別々の配列として持っている。

　　　　（例）f 1/1/1 2/2/2 3/3/3

　　　　これは 1頂点ぶんの情報を直接持っているのではなく、

　　　　position の何番目を使うか、uv の何番目を使うか、normal の何番目を使うかを組み合わせて参照している。

　　　　一方、DirectX側で描画に使う Vertex 構造体は、position/normal/uv を1セットにまとめた形で並んでいる必要がある。

　　　　そのため、OBJをそのままGPUに渡すことはできず、OBJの v / vt / vn / f を読んで、vertices と indices に変換する必要がある。

