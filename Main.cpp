#include <Siv3D.hpp>

#include "Pattern.h"
#include "StandardPatterns.h"
#include "Board.h"
#include "Algorithm.h"
#include "GameMode.h"

// 自身のパソコンのサイズに合わせて設定
#define BUTTON_X 1100

// 固定値
#define BOARD_AREA_SIZE 1024
#define BUTTON_HEIGHT 50
#define BUTTON_WIDTH 200
#define BUTTON_ROUND 5
#define BUTTON_Y(n) (BOARD_AREA_SIZE - 50 - 100 * (5 - (n)))

// JSONからゲームの初期化を行う関数
std::pair<Board, Array<Pattern>> initializeFromJSON(const FilePath& path) {
	JSON json;
	Board board(1, 1);  // デフォルトの空のボードを作成
	static Array<Pattern> patterns = StandardPatterns::getAllStandardPatterns_Grid();
	FilePath tempPath = path;
	try {
		json = JSON::Load(path);
		const FilePath path = FileSystem::FullPath(tempPath);
		Console << U"Full path: " << path;
		JSON json = JSON::Load(path);
		if (not json) {
			throw Error(U"Failed to load JSON file");
		}
		Console << U"Loaded JSON: " << json;
		Console << U"starts at " << json[U"startsAt"];
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

std::pair<Board, Array<Pattern>> initializeFromGet(const URL& url, const String token, const FilePath& path, String& httpResponse) {
	JSON json;
	Board board(1, 1);
	static Array<Pattern> patterns = StandardPatterns::getAllStandardPatterns_Grid();
	const HashTable<String, String> header = { { U"Procon-Token",token} };
	static const ColorF headlineColor(U"#33272a");
	constexpr int MAX_RETRIES = 20;
	constexpr int INITIAL_WAIT_MS = 100;
	int currentWait = INITIAL_WAIT_MS;

	for (int attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		if (attempt > 0) {
			System::Sleep(currentWait);
			currentWait *= 2;
			Console << U"Retry attempt " << attempt << U"...";
			httpResponse = U"GET:Retry attempt:{}"_fmt(attempt);
		}

		if (const auto response = SimpleHTTP::Get(url, header, path)) {
			const auto statusCode = response.getStatusCode();

			switch (statusCode) {
			case s3d::HTTPStatusCode::OK:
				// Console << U"Success!";
				httpResponse = U"GET:Sucess!";
				return initializeFromJSON(path);


			case s3d::HTTPStatusCode::InternalServerError:
			case s3d::HTTPStatusCode::BadGateway:
			case s3d::HTTPStatusCode::ServiceUnavailable:
			case s3d::HTTPStatusCode::GatewayTimeout:
				Console << U"Server error. Retrying...";
				Console << U"Server error: {}"_fmt(FromEnum(statusCode));
				httpResponse = U"GET:Server error Retrying...";
				continue;

			default:
				if (FromEnum(statusCode) >= 400 && FromEnum(statusCode) < 500) {
					Console << U"Client error: {}"_fmt(FromEnum(statusCode));
					httpResponse = U"GET:ClientError";
					// throw Error(U"Client error occurred");
				}
			}
		}
		else {
			// Console << U"Connection failed. Retrying...";
			httpResponse = U"Connection failed. Retrying...";
		}
	}
	httpResponse = U"Failed to initialize after " + Format(MAX_RETRIES) + U" attempts";
	// throw Error(U"Failed to initialize after " + Format(MAX_RETRIES) + U" attempts");
}

void postAnswer(const URL& url, const String token, const FilePath& path, String& httpResponse) {

	const HashTable<String, String> headers = { { U"Procon-Token", token }, {U"Content-Type", U"application/json"} };
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
			httpResponse = U"POST:Success!";
			Console << TextReader{ saveFilePath }.readAll();
		}
	}
	else
	{
		httpResponse = U"post failed";
		Console << U"Failed.";
	}
}

bool isMouseOnBoard() {
	return 0 <= Cursor::Pos().x && Cursor::Pos().x < BOARD_AREA_SIZE && 0 <= Cursor::Pos().y && Cursor::Pos().y < BOARD_AREA_SIZE;
}

void drawColorSample() {
	static const ColorF colors[] = { ColorF(U"#5f6c7b") , ColorF(U"#ef4565") ,ColorF(U"#094067") ,ColorF(U"#33272a") };
	static const Font font{ 32 };
	const int offsetX = BUTTON_WIDTH / 4;
	for (int i : step(4)) {
		int x = BUTTON_X + offsetX * i, y = 0;
		Rect(x, y, BUTTON_HEIGHT, BUTTON_HEIGHT).draw(colors[i]);
		font(i).drawAt(x + BUTTON_HEIGHT / 2, y + BUTTON_HEIGHT / 2);
	}
}

void patternDraw(const Array<Pattern>& patterns, int currentPattern, const int cellSize, const Point& patternPos, const GameMode& currentMode) {
	if (currentMode != GameMode::Manual) return;
	for (int32 y = 0; y < patterns[currentPattern].grid.height(); ++y) {
		for (int32 x = 0; x < patterns[currentPattern].grid.width(); ++x) {
			if (patterns[currentPattern].grid[y][x] == 1) {
				Rect((patternPos.x + x) * cellSize, (patternPos.y + y) * cellSize, cellSize)
					.draw(ColorF(1.0, 1.0, 0.0, 0.5));
			}
		}
	}
}

void trainAndDebug(const Array<Pattern>& patterns) {
	int trainStepSize = 1;
	Array<int> failedStep;
	for (int count : step(trainStepSize)) {
		Board board(128, 128);
		for (int i : step(128)) {
			for (int j : step(128)) {
				int value = Random<int>(4) % 4;
				board.grid[i][j] = value;
				board.goal[i][j] = value;
			}
		}
		Array<int> values;
		for (const auto& value : board.grid)
		{
			values << value;
		}

		values.shuffle();

		auto it = values.begin();
		for (auto& cell : board.grid)
		{
			cell = *it++;
		}

		auto solution = Algorithm::solve(Algorithm::Type::BeamSearch, board, patterns);
		Console << U"step size:" << solution.steps.size();
		// 移動方向割合の確認
		Array<int> directionCount(4, 0);
		for (const auto& action : solution.steps) {
			const auto& [solvePattern, solvePos, solveDir] = action;
			// answer.steps.emplace_back(action);
			board.apply_pattern(solvePattern, solvePos, solveDir);
			directionCount[solveDir]++;
			/*board.draw();
			System::Update();*/
		}
		Console << directionCount;

		if (board.is_goal()) {
			Console << U"step{}:clear"_fmt(count);
		}
		else {
			failedStep.emplace_back(count);
		}
	}
	Console << U"total {} failed {}%"_fmt(failedStep.size(), 100.0 * failedStep.size() / trainStepSize);
	for (const auto& stepNum : failedStep) {
		Console << U"step:{} failed"_fmt(stepNum);
	}
}

void Main() {

	//　シミュレータサイズ
	// const auto monitor = *System::EnumerateMonitors().rbegin();
	const auto monitor = System::EnumerateMonitors()[0];
	Window::Resize(monitor.fullscreenResolution);

	// フォント設定
	FontAsset::Register(U"Cell", 20);
	FontAsset::Register(U"Button", 30, Typeface::Bold);

	// 背景色
	const ColorF backgroundColor(U"#fffffe");
	Scene::SetBackground(backgroundColor);

	// ボタンの色
	const ColorF buttonColor(U"#094067");
	const ColorF white(U"#d8eefe");
	const ColorF autoColor(U"#5f6c7b");
	const ColorF resetColor(U"#ef4565");
	const ColorF buttonTextColor(U"#d8eefe");
	const ColorF headlineColor(U"#33272a");

	// PCどうしでやるときはIPアドレスとportを書き換える
	// 試合用
	const URL url = U"172.29.1.2:80";
	// const URL url = U"172.28.144.1:8080";
	// const URL url = U"172.28.144.1:8080";
	// const URL url = U"169.254.121.245:8080";
	const URL getUrl = U"{}/problem"_fmt(url);
	Console << getUrl;
	const URL postUrl = U"{}/answer"_fmt(url);
	int gameStartTime = -1;

	// tokenはもらったやつを使う
	const String token = U"kumamotoaccde9a1a8f15e4993dabb0d6c09aac7d3095c6172f68f34dbf3e950";
	// auto [board, patterns] = initializeFromGet(getUrl, token, U"input.json");

	// JSONファイルからゲームを初期化
	// auto [board, patterns] = initializeFromJSON(U"practice.json");
	auto [board, patterns] = initializeFromJSON(U"input.json");

	// 盤面１マス当たりのサイズ
	int32 cellSize = Min(BOARD_AREA_SIZE / board.width, BOARD_AREA_SIZE / board.height);

	// 抜き型設定
	int32 currentPattern = 0;
	Point patternPos(0, 0);
	int32 direction = 0;
	int32 patternHeight = patterns[currentPattern].grid.height();
	int32 patternWidth = patterns[currentPattern].grid.width();

	// 使用可能アルゴリズム
	const Array<Algorithm::Type> algorithms = {
		Algorithm::Type::Greedy,
		Algorithm::Type::BeamSearch,
		Algorithm::Type::ImprovedGreedy,
	};
	const Array<String> algorithmNames = { U"Greedy", U"BeamSearch", U"Greedy2" };

	// モード設定
	GameMode currentMode = GameMode::Manual;
	int32 currentAlgorithm = 0;

	// ボタン設定
	RoundRect manualButton(BUTTON_X, BUTTON_Y(1), BUTTON_WIDTH, BUTTON_HEIGHT, BUTTON_ROUND);
	RoundRect algorithmButton(BUTTON_X, BUTTON_Y(2), BUTTON_WIDTH, BUTTON_HEIGHT, BUTTON_ROUND);
	RoundRect autoButton(BUTTON_X, BUTTON_Y(3), BUTTON_WIDTH, BUTTON_HEIGHT, BUTTON_ROUND);
	RoundRect resetButton(BUTTON_X, BUTTON_Y(4), BUTTON_WIDTH, BUTTON_HEIGHT, BUTTON_ROUND);
	RoundRect submitButton(BUTTON_X, BUTTON_Y(5), BUTTON_WIDTH, BUTTON_HEIGHT, BUTTON_ROUND);

	// リセット機能
	// 10回押したらリセットされる
	int32 resetConfirmationCount = 0;
	int32 submitConfirmationCount = 0;

	// マウス入力（座標のみ）を無視するかどうか
	// m キーで切り替え
	bool readMouseInput = 1; // 1 -> 入力

	// 進度
	double progress = 100.0 * (1.0 - double(board.calculateDifference(board.grid)) / double((board.height * board.width)));
	double nextProgress = progress;

	//回答用
	Algorithm::Solution answer;
	// HTTPリクエスト結果保管用
	String httpResponse = U"";

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
			resetConfirmationCount++;
			// resetボタン付近の座標が型抜きに適用されてしまうのを防ぐためにモードを変更
			currentMode = GameMode::Auto;

			// 回答を初期化
			answer = Algorithm::Solution();

			if (resetConfirmationCount == 10) {
				resetConfirmationCount = 0;
				// ファイルから読む
				//board = initializeFromJSON(U"input.json").first;

				// getする
				board = initializeFromGet(getUrl, token, U"input.json", httpResponse).first;

				// 空の回答を送信
				/*Algorithm::Solution emptySolution;
				emptySolution.outuputToJson();
				postAnswer(postUrl, token, U"output.json");*/

				// 進度初期化
				progress = 100.0 * (1.0 - double(board.calculateDifference(board.grid)) / double((board.height * board.width)));

				// 盤面サイズ初期化
				cellSize = Min(BOARD_AREA_SIZE / board.width, BOARD_AREA_SIZE / board.height);


				// 範囲外適用を避けるため抜き型座標を初期化
				patternPos = Point(0, 0);
			}
		}

		if (submitButton.leftClicked()) {
			submitConfirmationCount++;
			// submitボタン付近の座標が型抜きに適用されてしまうのを防ぐためにモードを変更
			currentMode = GameMode::Auto;

			if (submitConfirmationCount == 10) {
				submitConfirmationCount = 0;
				// 出力と回答
				if (board.is_goal()) {
					answer.outuputToJson();
					postAnswer(postUrl, token, U"output.json", httpResponse);
					Console << postUrl;
				}
			}
		}

		// 自動 (貪欲)
		if (autoButton.leftClicked()) {
			// 既に揃っていたらリプレイ
			if (board.is_goal()) {
				// get時に"input.json"に保存済み
				// リプレイ用の一時的な盤面
				Board replayBoard = initializeFromJSON(U"input.json").first;;
				size_t currentStep = 0;

				while (currentStep < answer.steps.size()) {
					// 現在の状態を描画
					replayBoard.draw();

					// 次のステップのプレビューを表示
					if (currentStep < answer.steps.size()) {
						const auto& [pattern, point, direction] = answer.steps[currentStep];
						patternDraw(patterns, pattern.p, cellSize, point, currentMode);
					}

					// 「次へ」ボタンの描画
					if (SimpleGUI::Button(U"Next", Vec2(BUTTON_X, 20))) {
						if (currentStep < answer.steps.size()) {
							const auto& [pattern, point, direction] = answer.steps[currentStep];
							replayBoard.apply_pattern(pattern, point, direction);
							currentStep++;
						}
					}

					// 「最後まで」ボタンの描画
					if (SimpleGUI::Button(U"Skip to End", Vec2(BUTTON_X, 60))) {
						bool replaySteps = 1;
						while (currentStep < answer.steps.size() && replaySteps) {
							const auto& [pattern, point, direction] = answer.steps[currentStep];
							replayBoard.apply_pattern(pattern, point, direction);
							currentStep++;
							replayBoard.draw();
							System::Update();
							if (KeySpace.down()) {
								replaySteps = 0;
								currentStep = answer.steps.size();
								break;
							}
						}
					}

					// ステップ数の表示
					FontAsset(U"Regular")(U"{}/{}"_fmt(currentStep, answer.steps.size())).draw(700, 100);

					System::Update();
				}

				// リプレイ終了後、実際のboardを更新
				// board = replayBoard;

			}
			else {
				// 現在の盤面からスタート
				const auto& solution = Algorithm::solve(Algorithm::Type::BeamSearch, board, patterns);

				// 提出
				solution.outuputToJson();
				postAnswer(postUrl, token, U"output.json", httpResponse);
				Console << postUrl;

				// 適用&回答に保存
				for (const auto& action : solution.steps) {
					const auto& [pattern, point, direction] = action;
					board.apply_pattern(pattern, point, direction);
					answer.steps.emplace_back(action);
				}
			}

		}

		// 人力の入力処理
		if (currentMode == GameMode::Manual) {
			// マウス入力のオンオフを切り替え
			if (KeyM.down()) {
				readMouseInput ^= 1;
			}

			// マウス入力オン & マウスが盤面の上にある & マウスが動いたら
			// マウス座標に抜き型を置く
			if (readMouseInput && isMouseOnBoard()) {
				if (Cursor::Delta().x != 0 || Cursor::Delta().y != 0)
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
					progress = 100.0 * (1.0 - double(board.calculateDifference(board.grid)) / double((board.height * board.width)));
					nextProgress = 100.0 * board.calculateNextProgress(patterns[currentPattern], patternPos, direction) / double(board.height * board.width);
				}
			}

		}
		else {

			// アルゴリズムを変更
			if (KeyTab.down()) {
				currentAlgorithm = (currentAlgorithm + 1) % algorithms.size();
			}

			// アルゴリズムを実行
			if (KeySpace.down()) {
				auto startTime = std::chrono::high_resolution_clock::now();
				auto solution = Algorithm::solve(algorithms[currentAlgorithm], board, patterns);
				auto endTime = std::chrono::high_resolution_clock::now();
				double elapsedTime = std::chrono::duration<double>(endTime - startTime).count();
				Console << U"Time taken: " << elapsedTime << U" seconds";
				Console << U"step size:" << solution.steps.size();
				// 移動方向割合の確認
				Array<int> directionCount(4, 0);
				for (const auto& action : solution.steps) {
					const auto& [solvePattern, solvePos, solveDir] = action;
					answer.steps.emplace_back(action);
					board.apply_pattern(solvePattern, solvePos, solveDir);
					directionCount[solveDir]++;
					/*board.draw();
					System::Update();*/
				}
				Console << directionCount;

				progress = 100.0 * (1.0 - double(board.calculateDifference(board.grid)) / double((board.height * board.width)));
			}
		}
		// 描画
		board.draw();

		// モード切り替えボタンの描画と処理
		manualButton.draw(currentMode == GameMode::Manual ? buttonColor : white);
		algorithmButton.draw(currentMode == GameMode::Auto ? buttonColor : white);
		autoButton.draw(autoColor);
		resetButton.draw(resetColor);
		submitButton.draw(buttonColor);
		FontAsset(U"Button")(U"Manual").drawAt(manualButton.center(), buttonTextColor);
		FontAsset(U"Button")(U"Algo").drawAt(algorithmButton.center(), buttonTextColor);
		FontAsset(U"Button")(U"Auto").drawAt(autoButton.center(), buttonTextColor);
		// ９回ボタンを押すと点滅
		// -> あと一回でリセットの意
		const double p2 = Periodic::Sine0_1(1s);
		FontAsset(U"Button")(U"!!Reset!!").drawAt(resetButton.center(), (resetConfirmationCount == 9) ? buttonTextColor * p2 : buttonTextColor);
		FontAsset(U"Button")(U"!!Submit!!").drawAt(submitButton.center(), (submitConfirmationCount == 9) ? buttonTextColor * p2 : buttonTextColor);

		// 情報表示
		// 実際の値を計算
		const int actualValue =
			(patternPos.y >= 0 && patternPos.y < board.height && patternPos.x >= 0 && patternPos.x < board.width)
			? board.goal[patternPos.y][patternPos.x] : -1;

		// モード文字列の取得
		const String modeString = currentMode == GameMode::Manual
			? U"Manual"
			: algorithmNames[currentAlgorithm];

		const String formatString =
			U"Pattern: {}\n"		// 現在の抜き型
			U"Position: ({},{})\n"  // 抜き型の位置
			U"Direction: {}\n"		// 適用の方向
			U"Mode: {}\n"			// モード
			U"Progress: {}%\n"		// 進度
			U"Prediction: {}%\n"	// マニュアルモードで現在の抜き型を適用した時の進度を先読み
			U"Actual: {}\n"		    // カーソルがある位置の正しい要素
			U"Step Size: {}\n"
			U"{}"_fmt(
			patterns[currentPattern].p,
			patternPos.x, patternPos.y,
			U"↑↓←→"[direction],
			modeString,
			progress,
			nextProgress,
			actualValue,
			answer.steps.size(),
			httpResponse
			);

		// フォーマット適用
		FontAsset(U"Cell")(formatString).draw(BUTTON_X, 300, headlineColor);

		// 色見本表示
		drawColorSample();
		patternDraw(patterns, currentPattern, cellSize, patternPos, currentMode);

		// 不正なサイズ
		// 問題が読み取れていない場合はサイズが１になる
		if (board.height < 32 || board.width < 32) {
			if (1) {
				// 試合開始前の場合
				FontAsset(U"Cell")(U"Game Not Started").drawAt(Scene::Center(), Palette::Red);
			}
			else FontAsset(U"Cell")(U"Invalid Board").drawAt(Scene::Center(), Palette::Red);

		}
		// ゴール状態のチェック
		else if (board.is_goal()) {
			FontAsset(U"Cell")(U"Goal Reached!").drawAt(Scene::Center(), Palette::Red);
		}

	}
}
