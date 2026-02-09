# Week0 log


## Done

- DX11: clear + present 表示確認

- DX12: HelloTriangle 表示確認


## Environment

- OS: Windows 11

- VS: Visual Studio 2022

- SDK: Windows SDK 10.0.22621.0


## Issues \& Fixes

- リンカーの追加の依存ファイル設定

　　d3d11.lib dxgi.lib d3dcompiler.lib

　　これらを追加して解決

- リンカーのシステムのサブシステム設定

　　コンソールからWindowsに変更して解決（エントリーポイントをwWinMainにするため）


## Learning

- RALL(Resource Acquisition Is Initialization)

　　DirectXはリソースが多いためスコープを出ると自動でReleaseされるstd::vector、ComPtrの利用が必須（ComPtrは参照カウント管理）

　　例外が起こった場合でも安全。

　　その他Rall型（std::unique\_ptr, std::shared\_ptr, std::stringなど）

- ComPtrの操作

　　.Get() は生ポインタを借りる処理。所有権は渡さない

　　.Reset() は持っている参照を外す処理。明示的に開放する際や依存関係を切るときに利用。

　　IID\_PPV\_ARGS(\&comPtr) は生成APIに渡す際の正しい書き方。出力先（2重ポインタ）を渡すため

　　.GetAddressOf(), .ReleaseAndGetAddressOb() は2重ポインタを渡す正しい書き方

- 参照渡し（const\&）

　　DirectXは頻繁に大きいデータを渡すため値渡しをすると毎回コピーが起こり処理が遅くなったり意図しないバグが起きたりする

　　その解決のためにconst\& で書き込みなし読み取りだけにする

　　その他渡し方（書き込み可T\&、所有権を渡すT\&\& / std::move）

- std::array<T, n>

　　連続メモリ（配列と同じ並び）

　　T （変数名）\[n]は関数の引数にするとT\*に退化し、サイズ情報が消えるが、std::arrayは引数にしても情報が消えない。

　　.data(), .size() が使える。.data() はT\* が取れ、.size() は要素数が取れる。など

　　今回のconst\& で設計がしやすい。

- std::vector<T>

　　連続メモリであり、DXが求める情報を渡しやすい。必要に応じて 自動で確保/解放（RAII）

　　.data() で先頭ポインタ、.size() で要素数、sizeof(T) で1要素の大きさ、sizeof(T) \* .size() でバイト数が取得できる。

　　今回の例だとVertexはfloatが7個（x, y, z, r, g, b, a）でfloatは4バイトなので1要素の値は7 \* 4 で28になり要素数は3 のためバイト数は84となる。

　　std::arrayとよく似ているが、使い分けが重要となる。

　　arrayはサイズが決まっているため固定のもので利用する（行列（4x4）、色RGBA、固定ボーン数、固定ライト数など）

　　対してvectorはサイズを実行時に変更可能（.push\_back など）

　　また、保存領域が違う（arrayはスタック領域、vectorはヒープ領域）

　　よって小さくて固定、余計なヒープを確保したくない（頻繁に生成破棄）場合はarray、大きく要素数が増減する場合はvectorを使用すると良い。

- パディング/アラインメント

　　今回の例ではfloatがきれいに並んだため7 \* 4 でsizeof(Vertex) は28となったが、bool やchar 、float3 + float2 のような混ざり方をする

　　とCPU側で「境界を揃えるための隙間（パディング）」が入りsizeof(T) が増えることがある。

- World & View & Projection

　　モデルの頂点の座標はモデル自身の基準（Local）、モデルを世界に置いた座標（World）、カメラから見た座標（View）、遠近法で画面に映る形（Projection）の順で変換される。

　　変換にWorld行列、View行列、Projection行列を掛け合わせる。まとめてWVP行列。

　　GPU（Vertex Shader）でLocalで入ってきた座標をWVPをかけて画面に映る座標に変換される。
