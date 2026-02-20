# Week3 log


## Done

- DX11: カメラ実装により視点移動が行える


## Environment

- OS: Windows 11

- VS: Visual Studio 2022

- SDK: Windows SDK 10.0.22621.0


## Issues & Fixes

- FPSカメラ（yaw/pitch + マウス）を実装したが、視点がガクガクして操作不能

　　原因：View行列の生成にXMMatrixLookAtLH関数を使っていたが、求められるターゲット座標ではなく前方向ベクトルを渡していた。

　　　　　XMMatrixLookAtLH(eye, at, up) の at は「注視点（ワールド座標）」であり座標。

　　　　　FPSカメラで yaw/pitch から作る forward は方向ベクトル。座標ではなく向きであり、原点基準の位置ではない。

　　　　　つまりLookAtLHに方向ベクトルを注視点として誤って渡していたためカメラがワールド原点付近の点を見ようとしたり、

　　　　　eye が動くたびに at が意図しない位置と解釈され視点が不安定になった。

　　解決方法：FPSカメラでは方向ベクトルを直接受け取る XMMatrixLootToLH関数を使うように修正した。

　　　　　　　（LookAtLH関数を使い続ける場合は at = eye + forward とする方法もある。）


## Learning

- DirectXMath

　　DirectXMathには大きく2種類の型がある。

　　（1）XMFLOAT3 / XMFLOAT4 / XMFLOAT4X4 などのメモリに保存するための型（構造体・配列に入れやすい）

　　（2）XMVECTOR / XMMATRIX のような計算するための型（CPU の SIMDレジスタに乗ることを前提としている）

　　そのため、XMFLOAT3（保存）→　XMVECTOR（計算）→　XMFLOAT3（保存）の流れが多い。

　　XMLoad系：

　　　　XMLoadFloat3（例）はXMFLOAT3（メモリ上の3つのfloat）を読み込んで、計算用の XMVECTOR に変換する。

　　　　XMVECTOR は基本4要素（x, y, z, w）を持つが、XMLoadFloat3 は w を 0 にするのが一般的。

　　　　変換しないままでも計算はできるが、自分で加算/減算/正規化/外積など全部書くことが必要、SIMD最適化も効かないためXMLoadを利用するのが無難。

　　　　引数はポインタであり、値渡しではない。

　　XMVector3Normalize：

　　　　3Dベクトル（x, y, z）の長さを 1 に揃える（正規化する）

　　　　今回の forward のような向きは必要で長さは必要ない、長さがあると不便な場合に正規化するときに利用する。

　　XMVector3Cross：

　　　　2つのベクトル（a, b）から、 両方に直行するベクトル（垂直な方向）を作る。（外積）

　　　　外積は掛ける順番で符号（向き）が反転する。

　　　　DXはLH（左手座標系）のため右方向の向きを合わせるために up * forward の順番で掛ける。

　　　　up と　forward が平行に近いと外積がほぼ0になり、正規化で破綻する。（pitch を90度付近に行かせない（クランプ）の一つ）

　　XMStore系：

　　　　XMVECTOR の中身を保存用の XMFLOAT に書き戻す。

　　　　XMVECTOR には w 要素があるが XMFLOAT3 は 3要素しかないため w は捨てられる。

- ビット演算と進数

　　ビット/上位ビット：

　　　　数値は内部的に 2進数のビット列として扱われる。

　　　　上位ビット（最上位ビット/MSB）は一番左のビットのこと。

　　　　16bitの値なら、bit15（0始まりで数える）が最上位ビット。

　　AND演算（&）：

　　　　& はビットごとの AND。

　　　　各ビットで両方が1なら 1 、それ以外は0。

　　　　例：1010 & 110 = 1000

　　進数リテラルの接頭辞（C/C++）：

　　　　10進数：接頭辞なし

　　　　16進数：0x

　　　　2進数：0b（C++14以降）

　　　　8進数：0（先頭0。紛らわしいため注意が必要）

　　　　例：10進数で54の場合

　　　　　　16進数：0x36

　　　　　　2進数：0b110110

　　　　　　8進数：066

　　GetAsyncKeyState() と & 0x8000：

　　　　GetAsyncKeyState(key) は 16bit(SHORT)の値を返す。

　　　　そのうちの最上位ビット(bit15) が「今そのキーが押されているか」の情報を持つ。

　　　　そのため & 0x8000 を行い、押されているかだけを取り出すことができる。

　　　　押されているときの返り値が必ず 0x8000 だけとは限らない。

　　　　他の下位ビットも状況によって立つことがあるので、0x8000でマスクしてbit15だけ見るのが重要


　　進数変換の基本パターン：

　　　　2進→10進：各ビット × 2^n を足す（位取り）

　　　　10進→2進：2で割って余りを下から読む

　　　　10進→16進：16で割って余りを下から読む

　　　　16進↔2進：1桁=4ビット変換が最速

- XMMatrixLookAtLH と XMMatrixToLH

　　XMMatrixLookAtLH(eye, at, up)：

　　　　左手系のView行列を作る。

　　　　eye はカメラ位置（ワールド座標）at は注視点（ワールド座標）up は上方向の基準ベクトル

　　　　内部では1. forward = normalize(at - eye)

　　　　　　　　2. right = normalize(cross(up, forward)) （LH想定）

　　　　　　　　3. trueUp = cross(forwatd, right)

　　　　　　　　4. これらの基底と -dot(基底, eye) でView行列を組んでいる。

　　XMMatrixLookToLH(eye, direction, up)：

　　　　左手系のView行列を作る。

　　　　eye はカメラ位置（ワールド座標）direction は見る方向ベクトル（通常は単位ベクトル）　up は上方向の基準ベクトル

　　　　内部では1. forward = normalize(direction)

　　　　　　　　2. right = normalize(cross(up, forward))

　　　　　　　　3. trueUp = cross(forward, right)

　　　　　　　　4. 以降はLookAtLH と同様にView行列を組んでいる。

　　この二つの関数の違いはforward を（at - eye）で作るか、direction を直接使うかだけ。

- デルタタイム計算

　　フレームレート（FPS）が変わっても同じ速度で動かすためにはデルタタイムが必要。

　　デルタタイムは前フレームからの経過秒数。

　　QueryPerformanceCounter関数は高精度の「単調増加カウンタ」で、フレーム間の経過時間を測れる。

　　QueryPerformanceFrequency関数は「1秒あたりのカウント数」を返し、(now - prev) / freq で秒に変換できる。

　　デルタタイムを使って 移動量 = 速度（m / s）* deltaTime（s）の形にすると、フレームレート非依存の挙動になる。

　　デバッグ停止や負荷で deltaTime が跳ねることがあるため、ワープ防止に上限クランプを入れると安全。

- static, const

　　static float ClampPitch(float pitch) のstatic：

　　　　クラスの静的メンバ関数（static member function）を意味するstatic。

　　　　このstaticをつけることでオブジェクト（this）に依存しない関数になる

　　　　つまり、その関数の中ではメンバ変数（position/yaw/pitch など）に直接アクセスできない。

　　　　呼び出し方も「インスタンス経由」ではなく「クラス名経由」が基本。

　　　　ClampPitch は「引数で渡された pitch を範囲に収めて返す」だけで、position や yaw など カメラ固有の状態を読まないため合っている。

　　XMVECTOR GetForward() const の末尾 const：

　　　　これはconstメンバ関数（const member function）という意味。

　　　　この関数の実行中、メンバ変数を変更しない（変更しようとするとコンパイルエラー）

　　　　つまり、GetForward() はカメラの状態（yaw/pitch）を読むだけで、forwardベクトルを 計算して返すという「読み取り専用の関数」だと宣言している。

　　　　（状態を変更しない関数にconstをつけて読みより専用を保証することができる。）

　　　　つける利点：

　　　　　　間違って yaw += ... みたいな変更を書いたら コンパイルで止められたり、

　　　　　　const Camera& からでも呼べる（設計が綺麗になる）

　　static の全般：

　　　　C++ の static は寿命 / 所属 / 見える範囲を変えるキーワードで、場所によって意味が変わる。

　　　　1. ローカル変数に付くstatic

　　　　　　寿命が「プログラム終了まで」になる。

　　　　　　用途：前回値の保持、初回フラグ、キャッシュなど。

　　　　2. グローバル変数/関数に付く static（古い意味：内部リンケージ）

　　　　　　その.cpp内だけで使える（他の.cppから見えない）

　　　　　　今は anonymous namespace {} を使うことも多い

　　　　　　用途：ファイル内限定の隠蔽。

　　　　3. クラスの static メンバ変数

　　　　　　全インスタンスで共有される変数（A全体で1つ）

　　　　　　個体ごとではなくクラス全体の状態

　　　　　　用途：生成数カウント、共通設定、共有キャッシュ。

　　　　4. クラスの static メンバ関数（今回）

　　const の全般：

　　　　const は変更不可を表すが、どこに付くかで対象が変わる。

　　　　1. 変数のconst

　　　　　　その変数は変更できない。

　　　　2. ポインタとconst

　　　　　　const int* p;　「指す先」がconst（値を変えない）

　　　　　　int* const q;　「ポインタ自体」がconst（別の場所を指せない）

　　　　　　const int* const r; // 両方const

　　　　3. 参照のconst（参照渡しの読み取り版）

　　　　　　void g(const std::vector<int>& v);　vを変更しない、コピーもしない

　　　　4. メンバ関数末尾のconst（今回）

　　　　5. 戻り値に付く const（基本は不要）

　　　　　　const int Foo(); // ほぼ意味がないことが多い

　　　　　　※戻り値にconstは有用性が薄いケースが多い（参照を返す場合は別）。




