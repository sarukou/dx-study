\# Week0 log



\## Done

\- DX11: clear + present 表示確認

\- DX12: HelloTriangle 表示確認



\## Environment

\- OS: Windows 11

\- VS: Visual Studio 2022

\- SDK: Windows SDK 10.0.22621.0



\## Issues \& Fixes

\- リンカーの追加の依存ファイル設定

　　d3d11.lib dxgi.lib d3dcompiler.lib

　　これらを追加して解決

\- リンカーのシステムのサブシステム設定

　　コンソールからWindowsに変更して解決（エントリーポイントをwWinMainにするため）



\## Learning

\- RALL(Resource Acquisition Is Initialization)

　　DirectXはリソースが多いためスコープを出ると自動でReleaseされるstd::vector、ComPtrの利用が必須（ComPtrは参照カウント管理）

　　例外が起こった場合でも安全。

　　その他Rall型（std::unique\_ptr, std::shared\_ptr, std::stringなど）

\- ComPtrの操作

　　.Get() は生ポインタを借りる処理。所有権は渡さない

　　.Reset() は持っている参照を外す処理。明示的に開放する際や依存関係を切るときに利用。

　　IID\_PPV\_ARGS(\&comPtr) は生成APIに渡す際の正しい書き方。出力先（2重ポインタ）を渡すため

　　.GetAddressOf(), .ReleaseAndGetAddressOb() は2重ポインタを渡す正しい書き方

\- 参照渡し（const\&）

　　DirectXは頻繁に大きいデータを渡すため値渡しをすると毎回コピーが起こり処理が遅くなったり意図しないバグが起きたりする

　　その解決のためにconst\& で書き込みなし読み取りだけにする

　　その他渡し方（書き込み可T\&、所有権を渡すT\&\& / std::move）

