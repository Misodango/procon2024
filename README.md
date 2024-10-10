# procon35 登録番号 30013 ゴリゴリズム2.0.0

# 概要
- これは、第34回全国高等専門学校プログラミングコンテスト 競技部門のために開発されたソフトウェアです。
- ボードゲームAIであるAlpha Zeroを参考にしており、モンテカルロ木探索とニューラルネットワークを駆使して勝利を目指します。
- 大きく分けて、「盤面管理統合ソフト」と「盤面価値評価ニューラルネットワーク」によって構成されています。
- 言語としてC++とpythonを用いており、Visual Studio上で完結します。

# installation

visual studio上からこのレポジトリをクローンし、siv3dのパスを通してください。

`プロジェクト` -> `プロパティ` -> `構成プロパティ` -> `VC++ディレクトリ` -> `インクルードディレクトリ`と`ライブラリディレクトリ`のsiv3dのパスを、

お使いのPCにインストールされているsiv3dのパス（環境変数）と揃えてください。

もしくは、お使いのPCのsiv3dを6.15(2024/08/29 時点で最新)にしてください。

siv3dを最新版にしたら、[ここ](https://zenn.dev/link/comments/8aaf45fc5ae077)を参考に、設定をしてください。

App下のディレクトリの置換とリビルドを忘れずに！



# 盤面管理統合ソフト
競技サーバーとのHTTP通信、データの管理、データの表示、職人の行動決定をすることが出来るソフトウェアです。
- [Player](Procon2023/Player/Main.cpp)
  - 試合全体の処理の流れが書いてあります。
  - main関数のような立ち位置のプロジェクトです。
- [Player(enemy)](Procon2023/Player(enemy)/Main.cpp)
  - 1台のPCで対戦をする際に、Playerと同時に実行するプロジェクトです。
  - 本番では使用しません。
- [Player(training)](Procon2023/Player(training)/Main.cpp), [Player(enemy)(training)](Procon2023/Player(enemy)(training)/Main.cpp)
  - 試合を複数回、連続で実行するときに用いるプロジェクトです。
  - ニューラルネットワーク用に大量の対戦データを生成するときに使います。
  - 本番では使用しません。
- [server](Procon2023/server/server.py)
  - プロコン公式サイトで配布された、簡易競技サーバーを簡単に動かすためのプロジェクトです。
  - 1台のPCで対戦をする際に、Playerと同時に実行するプロジェクトです。
  - 本番では使用しません。
- [no_time_server](Procon2023/no_time_server/no_time_server.py)
  - 対戦の高速化のために、1から自作した競技サーバーです。
  - POSTリクエストを受け取り次第盤面を遷移します。
  - 本番では使用しません。
- [keras_server](Procon2023/keras_server/keras_server.py)
  - ニューラルネットワークによって盤面価値を計算するプロジェクトです。
  - ローカルホストを用いたHTTP通信により、C++とPython間でのデータ共有を行います。
- [Game.hpp](Procon2023/include/Game.hpp)
  - 試合全般を管理するクラスです。
  - 行動の決定やHTTP通信、画面表示をするために必要な全てのメソッドの宣言が書かれています。
- [Algorithm.cpp](Procon2023/include/Algorithm.cpp)
  - 行動決定のためのアルゴリズムが複数書かれたcppファイルです。
  - 本番では、ここに書かれたメソッドの中から一つを選んで使用します。


# 盤面価値評価ニューラルネットワーク
盤面価値評価ニューラルネットワークを訓練するプログラムをvalue_network_trainingに示します。
value_network_trainingは3つのプログラム、1つのフォルダ、1つの学習済みデータによって構成されています。本番では学習済みデータのみを使用します。
- [A11_sig](value_network_training/A11_sig)
  - 大量の対戦データが格納されたフォルダです。各ターンごとの行動をjsonで保存しています。
- [my_regression_model.h5](value_network_training/my_regression_model.h5)
  - lenet_2.pyを用いて学習されたデータです。
- [json_read.py](value_network_training/json_read.py)
  - A11_sigに格納された大量の対戦データをjsonから読み取り、学習を行うことの出来るデータフォーマットに変換するプログラムです。
- [lenet_2.py](value_network_training/lenet_2.py)
  - 大量の対戦データから盤面価値を計算するニューラルネットワークを学習するプログラムです。畳み込みニューラルネットワークの一つであるlenetを改良したものとなっています。
- [try_model.py](value_network_training/try_model.py)
  - lenet_2.pyによって学習されたデータmy_regression_model.h5を読み込み推論の確認を行うプログラムです。
