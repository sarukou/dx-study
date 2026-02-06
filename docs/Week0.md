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

