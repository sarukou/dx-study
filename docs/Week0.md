# Week0 log


## Done

- DX11: clear + present 表示確認

- DX12: HelloTriangle 表示確認


## Environment

- OS: Windows 11

- VS: Visual Studio 2022

- SDK: Windows SDK 10.0.22621.0


## Issues & Fixes

- リンカーの追加の依存ファイル設定

　　d3d11.lib dxgi.lib d3dcompiler.lib

　　これらを追加して解決

- リンカーのシステムのサブシステム設定

　　コンソールから Windows に変更して解決（エントリーポイントを wWinMain にするため）


## Learning

- RALL(Resource Acquisition Is Initialization)

　　DirectX はリソースが多いためスコープを出ると自動で Release される std::vector、ComPtr の利用が必須（ComPtr は参照カウント管理）

　　例外が起こった場合でも安全。

　　その他（std::unique_ptr, std::shared_ptr, std::string など）

- ComPtr の操作

　　.Get() は生ポインタを借りる処理。所有権は渡さない

　　.Reset() は持っている参照を外す処理。明示的に開放する際や依存関係を切るときに利用。

　　IID_PPV_ARGS(&ComPtr) は生成APIに渡す際の正しい書き方。出力先（2重ポインタ）を渡すため

　　.GetAddressOf(), .ReleaseAndGetAddressOb() は2重ポインタを渡す正しい書き方

- 参照渡し（const T&）

　　DirectX は頻繁に大きいデータを渡すため値渡しをすると毎回コピーが起こり処理が遅くなったり意図しないバグが起きたりする

　　その解決のために const T& で書き込みなし読み取りだけにする

　　その他渡し方（書き込み可 T&、所有権を渡す T&& / std::move）

- std::array<T, n>

　　連続メモリ（配列と同じ並び）

　　T （変数名）[n]は関数の引数にすると T* に退化し、サイズ情報が消えるが、std::array は引数にしても情報が消えない。

　　.data(), .size() が使える。.data() はT* が取れ、.size() は要素数が取れる。など

　　今回の constT& で設計がしやすい。

- std::vector<T>

　　連続メモリであり、DX が求める情報を渡しやすい。必要に応じて 自動で確保/解放（RAII）

　　.data() で先頭ポインタ、.size() で要素数、sizeof(T) で1要素の大きさ、sizeof(T) * .size() でバイト数が取得できる。

　　今回の例だと Vertex は float が7個（x, y, z, r, g, b, a）で float は4バイトなので1要素の値は7 * 4 で28になり要素数は3のためバイト数は84となる。

　　std::array とよく似ているが、使い分けが重要となる。

　　array はサイズが決まっているため固定のもので利用する（行列（4x4）、色RGBA、固定ボーン数、固定ライト数など）

　　対して vector はサイズを実行時に変更可能（.push_back など）

　　また、保存領域が違う（array はスタック領域、vector はヒープ領域）

　　よって小さくて固定、余計なヒープを確保したくない（頻繁に生成破棄）場合は array、大きく要素数が増減する場合はvector を使用すると良い。

- パディング/アラインメント

　　今回の例では float がきれいに並んだため7 * 4 で sizeof(Vertex) は28となったが、bool やchar 、float3 + float2 のような混ざり方をする

　　とCPU側で「境界を揃えるための隙間（パディング）」が入り sizeof(T) が増えることがある。

- World & View & Projection

　　モデルの頂点の座標はモデル自身の基準（Local）、モデルを世界に置いた座標（World）、カメラから見た座標（View）、遠近法で画面に映る形（Projection）の順で変換される。

　　変換に World 行列、View 行列、Projection 行列を掛け合わせる。まとめて WVP 行列。

　　GPU（Vertex Shader）で Local で入ってきた座標を WVP をかけて画面に映る座標に変換される。

- VS→PS→Present（描画までの流れ）

　　① Input Assembler（IA）　CPU が用意した頂点配列（VB） と インデックス（IB） を読み取る。

　　② Vertex Shader（VS）　　頂点ごとに走る。座標を変換（Local→World→View→Proj）するなど。

　　③ Rasterizer（RS）	　　　三角形を ピクセルの集合に変換する（どのピクセルが塗られるか決める）裏面カリング、ビューポート、深度などもここに関係している。

　　④ Pixel Shader（PS)      ピクセルごとに走る。ピクセルの色を決める（テクスチャ貼ったり、ライティングしたり）

　　⑤ Output Merger（OM）　　PS が出した色を RenderTarget に書き込む（深度/ブレンディング）

　　⑥ Present 		　　　バックバッファを画面に表示する。SwapChain が「表に出すバッファ」を入れ替える。

　　「CPU が頂点バッファやパイプラインをセットして描画を指示。IA で頂点を取り出し、VS で座標変換して三角形を作れる形にする。ラスタライザが三角形を画面のピクセルに分解し、PS が各ピクセルの色を決める。最後に OM がバックバッファ（レンダーターゲット）に書き込み、Present で画面に表示する。」

- リソースとビュー

　　リソース（実体）　データが入っているもの（テクスチャやバッファ）

　　ビュー　　　　　　その実体をどう使うかを表す見方・入口（読み方、書き方、形式、範囲）

　例：Render Target と Render Target View

　　Render Target　　 　描画結果を書き込む先そのもの（実体はだいたい Texture2D）
　　
    Render Target View　その実体を出力先として書き込むためのハンドル（GPU が参照する書き込み口）

