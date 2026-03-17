# Week5 log


## Done

- DX11: モデル描画、テクスチャ実装ができている状態にする（OBJファイル）


## Environment

- OS: Windows 11

- VS: Visual Studio 2022

- SDK: Windows SDK 10.0.22621.0


## Issues & Fixes

特になし

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

- WICを使った画像読み込み・テクスチャ作成

　　今回は WICTextureLoader のような補助ライブラリは使わず、WICを自分で利用して画像ファイルを読み込んだ。

　　WICはWindows標準の画像デコード機能で、pngやjpgなどの画像ファイルを読み込み、ピクセルデータへ変換する役割を持っている。

　　まずWICのファクトリを作成、ファクトリから画像デコーダを作成、画像の先頭フレームを取得、IWICFormatConverter を使ってピクセルフォーマットを 32bppRGBA に変換、CopyPixels() を使って画像データをCPUメモリ上の配列にコピーという流れで読み込みを行った。

　　フォーマットを揃えることで、Direct3D側で DXGI_FORMAT_R8G8B8A8_UNORM のテクスチャとして扱いやすくなる。

　　この時点ではまだ「画像ファイルを読んだだけ」であり、GPUのテクスチャになってはおらず、画像ファイルの読み込みと、GPUリソースの作成は別工程である。

　　IWICImagingFactory：

　　　　WIC の各種オブジェクトを作るための入り口（画像処理用オブジェクトを作る工場）

　　　　WIC ではデコーダ、フォーマットコンバータ、ビットマップなどのいろいろなオブジェクトを使うが、それらを作る機転が IWICImagingFactory

　　　　CoCreateInstance関数（COMオブジェクトを生成する関数）で作成する。

　　IWICBitmapDecoder：

　　　　画像ファイルを読み解くオブジェクト（ファイル形式を理解して中身を取り出す担当）

　　　　png や jpg のファイルはそのままでは単なるバイト列だが、それを幅、高さ、ピクセルフォーマット、ピクセル内容として読み取れる形にするのがデコーダ。

　　　　CreateDecoderFromFilename関数で読み込み、パスや読み取り専用か、キャッシュについて、受け取り先を設定する。

　　　　フレーム取得（GetFrame）：

　　　　　　png や jpg は 1フレームだけだが、GIF や TIFFの一部、アニメーション画像などは複数フレームを持つことがある。

　　　　　　デコーダから何番目のフレームを使うかを選ぶ。今回は先頭フレームを使用。（jpg のため）

　　IWICBitmapFrameDecode：

　　　　一枚分の画像データを表すオブジェクト。

　　　　ここからサイズ取得、ピクセルフォーマット確認、ピクセル変換ができる。

　　　　フレーム取得の出力先として使われる。

　　IWICFormatConverter：

　　　　画像のピクセル形式を変換するオブジェクト。

　　　　Initialize関数で変換元の画像データ、変換先のフォーマットやパレット設定などを指定する。

　　　　ピクセルフォーマット変換：

　　　　　　画像ファイルのピクセルフォーマットはいろいろある。

　　　　　　（例）RGB24,BGR24,BGRA32,Gray8,Indexed color など

　　　　　　Direct3D側で扱いやすいフォーマットにしたい、揃えたいため、WIC側は32bppRGBA、D3D側はDXGI_FORMAT_R8G8B8A8_UNORM としている。

　　　　　　これに揃えた場合はどんな画像が来ても最終的に RGBA8 の 4チャンネル画像に統一できる。

　　　　サイズ取得：

　　　　　　元のフレーム（FrameDecode）からでも取得することができるが、最終的に使う変換後画像のサイズとして取得している。

　　　　ピクセルバッファに読み込み（CopyPixels）：

　　　　　　変換後の画像を、CPUメモリ上の生のバイト配列にコピー

　　　　　　pixelsの中には「R,G,B,A,R,G,B,A,...」のように並んだピクセルデータが入る。（GPUテクスチャではなく CPU配列）

　　　　　　（今回の実装の例）

　　　　　　　　bytesPerPixel = 4：

　　　　　　　　　　RGBA8 なので R8bit, G8bit, B8bit, A8bit のため合計 4バイト

　　　　　　　　rowPitch = width * bytesPerPixel：

　　　　　　　　　　1行当たりのバイト数（行ピッチ）

　　　　　　　　　　幅256 なら 256 * 4 = 1024byte

　　　　　　　　imageSize = rowPitch * height：

　　　　　　　　　　画像全体の総バイト数（行 * 高さ）

　　　　　　　　std::vector<std::uint8_t> pixels(imageSize)：

　　　　　　　　　　画像全体を入れるバッファの確保

　　　　　　　　　　1要素＝1バイトのため uint8_t

　　Texture2D作成：

　　　　CreateTexture2D関数で GPU上に本物のテクスチャリソースを作成する。

　　　　引数では、Desc、初期ピクセルデータ、出力先を指定する。


- Texture2DとShaderResourceView

　　ID3D11Texture2D：

　　　　GPU上に作られるテクスチャリソースそのもの

　　　　画像のピクセルデータを保持する実体が Texture2D

　　ID3D11ShaderResourceView：

　　　　テクスチャをシェーダーから参照するための窓口

　　　　Pixel Shaderに直接 Texture2D を渡すのではなく、SRVを通してアクセスを行う

　　テクスチャ表示を行うには、画像から Texture2D を作るだけでなく、それに対する ShaderResourceView も作成する必要がある。

- SamplerState

　　テクスチャをどのように読むかを表す設定オブジェクト

　　テクスチャ自体は画像データそのものを持っているが、その画像をPixel Shaderで参照するときに、どのようなルールで色を取り出すかを決めるためのもの。

　　SamplerState が別オブジェクトの理由：

　　　　SamplerState が Texture と別れていることで同じテクスチャに対して複数の読み方を使い分けることができる。

　　　　なめらかに補間、ドット絵のように補間なし、UV範囲外で繰り返す、UV範囲外で端の色を維持、という違いを Texture 本体を作り直さずに切り替えられる。

　　SamplerState が関わる場面：

　　　　HLSL の PixelShader で Texture2D.Sample() を呼んだ時に使われる。

　　　　Sample() はその座標の色を返すわけではなく、SamplerState に従って色を返している。

- Pixel Shaderでのテクスチャサンプリング

　　Pixel Shaderでは、頂点シェーダーから渡されたUV座標を使ってテクスチャを参照する。

　　Texture2D.Sample() を使い、input.uv に対応する画像上の位置から色を取得することができる。

　　UVは単なる値ではなく、「モデルの表面上の位置」と「画像上の位置」を結びつける座標である。

- テクスチャ色とライティングの関係

　　テクスチャ：モデル表面の模様や材質のベース色

　　ライティング：その表面に光がどのくらい当たっているか

　　最終的な色：テクスチャ色 × ライティング結果

　（今回の例）：

　　　　textureColor でテクスチャ色を取得

　　　　dot(N, L) でLambertの明るさを計算

　　　　ambient + diffuse でライティング係数を作成

　　　　texColor.rgb * lighting で最終色を計算

　　この実装で単に画像を貼るだけだった状態から、光の方向によって明るさが変化する、より立体感のある見た目になる。


