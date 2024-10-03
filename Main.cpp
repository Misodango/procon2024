#include <Siv3D.hpp>

#include "Pattern.h"
#include "StandardPatterns.h"
#include "Board.h"
#include "Algorithm.h"
#include "GameMode.h"


// JSONからゲームの初期化を行う関数
std::pair<Board, Array<Pattern>> initializeFromJSON(const FilePath& path) {
	JSON json;
	Board board(1, 1);  // デフォルトの空のボードを作成
	static Array<Pattern> patterns = StandardPatterns::getAllStandardPatterns_Grid();

	try {
		json = JSON::Load(path);
		const FilePath path = FileSystem::FullPath(U"input.json");
		Console << U"Full path: " << path;
		JSON json = JSON::Load(path);
		if (not json) {
			throw Error(U"Failed to load JSON file");
		}
		Console << U"Loaded JSON: " << json;
		// ボードの初期化
		board = Board::fromJSON(json[U"board"]);

		// パターンの初期化
		for (const auto& patternJson : json[U"general"][U"patterns"].arrayView()) {
			patterns << Pattern::fromJSON(patternJson);
		}
	}
	catch (const Error& error) {
		Console << U"Error loading JSON: " << error.what();
	}
	return { board, patterns };
}

std::pair<Board, Array<Pattern>> initializeFromGet(const URL& url, const String token, const FilePath& path) {
	JSON json;
	Board board(1, 1);  // デフォルトの空のボードを作成
	static Array<Pattern> patterns = StandardPatterns::getAllStandardPatterns_Grid();
	const HashTable<String, String> header = { { U"Procon-Token",token} };
	if (const auto response = SimpleHTTP::Get(url, header, path))
	{
		Console << U"------";
		Console << response.getStatusLine().rtrimmed();
		Console << U"status code: " << FromEnum(response.getStatusCode());
		Console << U"------";
		Console << response.getHeader().rtrimmed();
		Console << U"------";

		if (response.isOK())
		{
			Console << TextReader{ path }.readAll();
		}
	}
	else
	{
		Console << U"Get Failed.";
	}

	return initializeFromJSON(path);
	// return { board, patterns };
}

void postAnswer(const URL& url, const String token, const FilePath& path) {

	const HashTable<String, String> headers = { { U"Procon-Token", token }, {U"Content-Type", U"application/json"} };
	/*const std::string data = JSON
	{
		{ U"body", U"Hello, Siv3D!" },
		{ U"date", DateTime::Now().format() },
	}.formatUTF8();*/
	const std::string data = JSON::Load(path).formatUTF8();
	const FilePath saveFilePath = U"post_result.json";

	if (auto response = SimpleHTTP::Post(url, headers, data.data(), data.size(), saveFilePath))
	{
		Console << U"------";
		Console << response.getStatusLine().rtrimmed();
		Console << U"status code: " << FromEnum(response.getStatusCode());
		Console << U"------";
		Console << response.getHeader().rtrimmed();
		Console << U"------";

		if (response.isOK())
		{
			Console << TextReader{ saveFilePath }.readAll();
		}
	}
	else
	{
		Console << U"Failed.";
	}
}


void Main() {
	// Window::Resize(1920, 1080);
	// const auto monitor = *System::EnumerateMonitors().rbegin();
	const auto monitor = System::EnumerateMonitors()[0];
	const ColorF backgroundColor(U"#fffffe");
	Window::Resize(monitor.fullscreenResolution);
	FontAsset::Register(U"Cell", 20);
	FontAsset::Register(U"Button", 30, Typeface::Bold);
	Scene::SetBackground(backgroundColor);

	// 新しい色（ボタン用）
	const ColorF buttonColor(U"#094067");
	const ColorF white(U"#d8eefe");
	const ColorF autoColor(U"#5f6c7b");
	const ColorF resetColor(U"#ef4565");
	const ColorF buttonTextColor(U"#d8eefe");
	const ColorF headlineColor(U"#33272a");

	// PCどうしでやるときはIPアドレスとportを書き換える
	// const URL url = U"192.168.154.167:3000";
	const URL url = U"127.0.0.1:8080";
	const URL getUrl = U"{}/problem"_fmt(url);
	Console << getUrl;
	const URL postUrl = U"{}/answer"_fmt(url);

	// tokenはもらったやつを使う
	const String token = U"token1";
	auto [board, patterns] = initializeFromGet(getUrl, token, U"input.json");

	// JSONファイルからゲームを初期化
	// auto [board, patterns] = initializeFromJSON(U"input.json");
	int32 cellSize = Min(1024 / board.grid.width(), 1024 / board.grid.height());
	int32 currentPattern = 0;
	Point patternPos(0, 0);
	int32 direction = 0;
	int32 patternHeight = patterns[currentPattern].grid.height();
	int32 patternWidth = patterns[currentPattern].grid.width();

	const Array<Algorithm::Type> algorithms = {
		Algorithm::Type::Greedy,
		Algorithm::Type::BeamSearch };

	GameMode currentMode = GameMode::Manual;
	int32 currentAlgorithm = 0;
	RoundRect manualButton(1100, 674, 200, 50, 5);
	RoundRect algorithmButton(1100, 774, 200, 50, 5);
	RoundRect autoButton(1100, 874, 200, 50, 5);
	RoundRect resetButton(1100, 974, 200, 50, 5); // 1024 - 50
	// リセット誤爆防止 10回押したらリセットされる
	int32 resetFailProof = 0;
	const Array<String> algorithmNames = { U"Greedy", U"BeamSearch" };

	// マウス入力（座標のみ）を無視するかどうか
	// m キーで切り替え
	bool readMouseInput = 1; // 1 -> 入力

	// 進度
	double progress = 100.0 * (1.0 - double(board.calculateDifference(board.grid)) / double((board.grid.height() * board.grid.width())));
	double nextProgress = progress;


	//回答用
	Algorithm::Solution answer;

	while (System::Update()) {

		// 人力モード
		if (manualButton.leftClicked()) {
			currentMode = GameMode::Manual;
		}

		// アルゴリズムモード
		if (algorithmButton.leftClicked()) {
			currentMode = GameMode::Auto;
			currentAlgorithm = (currentAlgorithm + 1) % algorithms.size();
		}

		// リセット
		// 10回押したら発動
		if (resetButton.leftClicked()) {
			resetFailProof++;
			// resetボタン付近の座標が型抜きに適用されてしまうのを防ぐためにモードを変更
			currentMode = GameMode::Auto;

			answer = Algorithm::Solution();

			if (resetFailProof == 10) {
				resetFailProof = 0;
				// ファイルから読む
				// board = initializeFromJSON(U"input.json").first;
				// getする
				board = initializeFromGet(getUrl, token, U"input.json").first;
				progress = 100.0 * (1.0 - double(board.calculateDifference(board.grid)) / double((board.grid.height() * board.grid.width())));
				cellSize = Min(1024 / board.grid.width(), 1024 / board.grid.height());
				patternPos = Point(0, 0);
			}
		}

		// 自動 (貪欲)
		if (autoButton.leftClicked()) {
			// 既に揃っていたらリプレイ
			if (board.is_goal()) {
				board = initializeFromJSON(U"input.json").first;

				for (const auto& [pattern, point, direction] : answer.steps) {
					board.apply_pattern(pattern, point, direction);
					board.draw();
					System::Update();
				}

			}
			else {
				// 現在の盤面からスタート
				const auto& solution = Algorithm::solve(Algorithm::Type::Greedy, board, patterns);

				for (const auto& action : solution.steps) {
					const auto& [pattern, point, direction] = action;
					board.apply_pattern(pattern, point, direction);
					answer.steps.emplace_back(action);
				}
			}

		}

		// 人力の入力処理
		if (currentMode == GameMode::Manual) {
			if (KeyM.down()) {
				readMouseInput ^= 1;
			}
			if (readMouseInput && Cursor::Pos().x >= 0 && Cursor::Pos().x < 1024 && Cursor::Pos().y >= 0 && Cursor::Pos().y < 1024) if (Cursor::Delta().x != 0 || Cursor::Delta().y != 0)
				patternPos = Point(Cursor::Pos().x / cellSize, Cursor::Pos().y / cellSize);

			else {
				// 入力処理
				if (KeyLeft.down()) patternPos.x = Max(-patternWidth + 1, patternPos.x - 1);
				if (KeyRight.down()) patternPos.x = Min(board.width - 1, patternPos.x + 1);
				if (KeyUp.down()) patternPos.y = Max(-patternHeight + 1, patternPos.y - 1);
				if (KeyDown.down()) patternPos.y = Min(board.height - 1, patternPos.y + 1);
				if (KeyTab.down()) {
					currentPattern = (currentPattern + 1) % patterns.size();
					patternHeight = patterns[currentPattern].grid.height();
					patternWidth = patterns[currentPattern].grid.width();
					patternPos = Point(0, 0);
				}
				if (KeyR.down()) direction = (direction + 1) % 4;
				if (KeySpace.down() || MouseL.down()) {
					board.apply_pattern(patterns[currentPattern], patternPos, direction);
					answer.steps.emplace_back(patterns[currentPattern], patternPos, direction);
				}
				progress = 100.0 * (1.0 - double(board.calculateDifference(board.grid)) / double((board.grid.height() * board.grid.width())));
				nextProgress = 100.0 * board.calculateNextProgress(patterns[currentPattern], patternPos, direction) / double(board.grid.height() * board.grid.width());
			}
		}
		else {

			// アルゴリズムを変更
			if (KeyTab.down()) {
				currentAlgorithm = (currentAlgorithm + 1) % algorithms.size();
			}

			// アルゴリズムを実行
			if (KeySpace.down()) {
				auto solution = Algorithm::solve(algorithms[currentAlgorithm], board, patterns);
				Console << U"step size:" << solution.steps.size();
				// 移動方向割合の確認
				Array<int> directionCount(4, 0);
				for (const auto& action : solution.steps) {
					const auto& [solvePattern, solvePos, solveDir] = action;
					answer.steps.emplace_back(action);
					board.apply_pattern(solvePattern, solvePos, solveDir);
					directionCount[solveDir]++;
					board.draw();
					System::Update();
				}
				Console << directionCount;

				// 出力と回答
				if (board.is_goal()) {
					answer.outuputToJson();
					postAnswer(postUrl, token, U"output.json");
					Console << postUrl;
				}
				progress = 100.0 * (1.0 - double(board.calculateDifference(board.grid)) / double((board.grid.height() * board.grid.width())));
			}
		}
		// 描画
		board.draw();

		// 現在のパターンを描画
		// 盤面が128より大きいと人力はに合わない
		if (board.height < 128 && board.width < 128) {
			for (int32 y = 0; y < patterns[currentPattern].grid.height(); ++y) {
				for (int32 x = 0; x < patterns[currentPattern].grid.width(); ++x) {
					if (patterns[currentPattern].grid[y][x] == 1) {
						Rect((patternPos.x + x) * cellSize, (patternPos.y + y) * cellSize, cellSize)
							.draw(ColorF(1.0, 0.0, 0.0, 0.5));
					}
				}
			}
		}


		// モード切り替えボタンの描画と処理
		manualButton.draw(currentMode == GameMode::Manual ? buttonColor : white);
		algorithmButton.draw(currentMode == GameMode::Auto ? buttonColor : white);
		autoButton.draw(autoColor);
		resetButton.draw(resetColor);
		FontAsset(U"Button")(U"Manual").drawAt(manualButton.center(), buttonTextColor);
		FontAsset(U"Button")(U"Algo").drawAt(algorithmButton.center(), buttonTextColor);
		FontAsset(U"Button")(U"Auto").drawAt(autoButton.center(), buttonTextColor);
		FontAsset(U"Button")(U"!!Reset!!").drawAt(resetButton.center(), buttonTextColor);

		// 情報表示
		FontAsset(U"Cell")(U"Pattern: {}\nPosition: ({},{})\nDirection: {}\nMode: {}\nProgress: {}%\nPrediction: {}%\nActual:{}"_fmt(
			patterns[currentPattern].p, patternPos.x, patternPos.y, U"↑↓←→"[direction],
			currentMode == GameMode::Manual ? U"Manual" : algorithmNames[currentAlgorithm],
			progress, nextProgress,
			patternPos.y >= 0 && patternPos.y < board.height &&
			patternPos.x >= 0 && patternPos.x < board.width
			? board.goal[patternPos.y][patternPos.x] : -1))
			.draw(1100, 300, headlineColor);

		// ゴール状態のチェック
		if (board.is_goal()) {
			FontAsset(U"Cell")(U"Goal Reached!").drawAt(Scene::Center(), Palette::Red);
		}

	}
}
