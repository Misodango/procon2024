# procon35 登録番号 30013 ゴリゴリズム2.0.0

# 概要
- これは、第35回全国高等専門学校プログラミングコンテスト 競技部門のために開発されたソフトウェアです。
- 言語としてC++を用いており、Visual Studio上で完結します。

## ファイル構成

### [Main.cpp](./Main.cpp)
試合進行の中心となるファイルです。以下の機能を提供します：
- 試合の操作制御
- アルゴリズムの実行
- 人力/自動モードの切り替え
- リセット機能
- 競技サーバーとのHTTP通信
- リプレイ検証

![image](https://github.com/user-attachments/assets/e458f444-dd42-4251-9636-b175f1a667d3)

![Manual](https://img.shields.io/badge/Manual-blue?style=for-the-badge&logo=auto&logoColor=white)では、選択している抜き型に対応する領域が黄色くハイライトされます。
![image](https://github.com/user-attachments/assets/3e215dfa-dc68-4479-bfe4-428eb6e88d0d)


### Board
- [Board.h](./Board.h)
- [Board.cpp](./Board.cpp)

問題盤面のクラスを定義しています。ゲームの状態管理を担当します。

### Algorithm
- [Algorithm.h](./Algorithm.h)
- [Algorithm.cpp](./Algorithm.cpp)

試合で使用するアルゴリズムを実装しています：
- 貪欲法
- ビームサーチ
- 最適化された盤面（OptimizedBoard）
  - 元の盤面を16倍圧縮し、高速化を実現

### [GameMode.h](./GameMode.h)
ゲームモードの選択を管理します：
- アルゴリズムモード
- 人力操作モード
- 自動実行モード

### [Pattern.h](./Pattern.h)
パターンの処理を担当：
- JSONからのパターン取得

### [StandardPatterns.h](./StandardPatterns.h)
一般抜き型パターンの生成を行います。

## 開発環境
- Siv3D

## 使用方法
1. プロジェクトをクローン
2. Siv3Dの環境をセットアップ
3. プロジェクトをビルド
4. Main.cppを実行して試合を開始

## 注意事項
- 競技サーバーとの通信にはインターネット接続が必要です
- リプレイ機能を使用する際は、保存されたデータが必要になります

## リプレイ機能
![image](https://github.com/user-attachments/assets/da1f56b7-a670-4a46-b80e-45e47714f3dd)
盤面が揃っている状態で![Auto](https://img.shields.io/badge/Auto-gray?style=for-the-badge&logo=auto&logoColor=gray)を押すと、1手ずつ検証することができます。
![Skip to End](https://img.shields.io/badge/Skip_to_End-blue?style=for-the-badge&logo=auto&logoColor=white)は、連続するアニメーションとなります。
途中でスペースキーをおすと、リプレイが止まります。


# installation(メンバー向け)

visual studio上からこのレポジトリをクローンし、siv3dのパスを通してください。

`プロジェクト` -> `プロパティ` -> `構成プロパティ` -> `VC++ディレクトリ` -> `インクルードディレクトリ`と`ライブラリディレクトリ`のsiv3dのパスを、

お使いのPCにインストールされているsiv3dのパス（環境変数）と揃えてください。

もしくは、お使いのPCのsiv3dを6.15(2024/08/29 時点で最新)にしてください。

siv3dを最新版にしたら、[ここ](https://zenn.dev/link/comments/8aaf45fc5ae077)を参考に、設定をしてください。

App下のディレクトリの置換とリビルドを忘れずに！
