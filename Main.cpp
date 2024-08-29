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
		// const FilePath path = FileSystem::FullPath(U"input.json");
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
		Console << U"Failed.";
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

// 学習用のデータ生成
void generateData(int32 width, int32 height, int32 moveCount) {

	Board board(width, height);

	int32 totalCells = width * height;
	int32 minCount = Min(1, totalCells / 10);
	Array<int32> counts(4, minCount);
	int32 remain = totalCells - minCount * 4;
	for (auto i : step(remain)) {
		counts[Random(0, 3)]++;
	}
	Array<int32> numbers;
	for (auto i : step(4)) {
		for (auto j : step(counts[i])) {
			numbers.push_back(i);
		}
	}
	std::mt19937 get_rand_mt;
	std::shuffle(numbers.begin(), numbers.end(), get_rand_mt);


}


void Main() {



	// Window::Resize(1920, 1080);
	const auto monitor = System::EnumerateMonitors()[0];

	Window::Resize(monitor.fullscreenResolution);
	FontAsset::Register(U"Cell", 20);
	FontAsset::Register(U"Button", 30, Typeface::Bold);
	Scene::SetBackground(ColorF{ 0.8, 1.0, 0.9 });

	// PCどうしでやるときはIPアドレスとportを書き換える
	const URL url = U"192.168.154.167:3000";
	const URL getUrl = U"{}/problem"_fmt(url);
	Console << getUrl;
	const URL postUrl = U"{}/answer"_fmt(url);

	// tokenはもらったやつを使う
	const String token = U"token1";
	auto [board, patterns] = initializeFromGet(getUrl, token, U"_input.json");

	// JSONファイルからゲームを初期化
	// auto [board, patterns] = initializeFromJSON(U"input.json");
	int32 cellSize = Min(1024 / board.grid.width(), 1024 / board.grid.height());
	int32 currentPattern = 0;
	Point patternPos(0, 0);
	int32 direction = 0;
	int32 patternHeight = patterns[currentPattern].grid.height();
	int32 patternWidth = patterns[currentPattern].grid.width();

	Array<Algorithm::Type> algorithms = { Algorithm::Type::Greedy, Algorithm::Type::BeamSearch, Algorithm::Type::DynamicProgramming, Algorithm::Type::RowByGreedy,  Algorithm::Type::RowByRowAdvancedGreedy,
	Algorithm::Type::OneByOne, Algorithm::Type::DiagonalSearch, Algorithm::Type::SimulatedAnnealing, Algorithm::Type::Dijkstra, Algorithm::Type::HorizontalSwapSort,
		Algorithm::Type::ChokudaiSearch };
	GameMode currentMode = GameMode::Manual;
	int32 currentAlgorithm = 0;
	Rect manualButton(1000, 650, 200, 50);
	Rect algorithmButton(1350, 650, 200, 50);
	Rect autoButton(1350, 900, 200, 50);
	Rect resetButton(1600, 650, 200, 50);
	// リセット誤爆防止 10回押したらリセットされる
	int32 resetFailProof = 0;
	Array<String> algorithmNames = { U"Greedy", U"BeamSearch", U"DP", U"RowGreedy", U"RowGreedy改", U"OneByOne", U"Diagonal", U"Annealing", U"dijkstra" , U"水平スワップソート", U"chokudai" };

	double progress = 100.0 * (1.0 - double(board.calculateDifference(board.grid)) / double((board.grid.height() * board.grid.width())));

	Algorithm::Solution answer;

	while (System::Update()) {




		if (manualButton.leftClicked()) {
			currentMode = GameMode::Manual;
		}
		if (algorithmButton.leftClicked()) {
			currentMode = GameMode::Auto;
			currentAlgorithm = (currentAlgorithm + 1) % algorithms.size();
		}
		if (resetButton.leftClicked()) {
			resetFailProof++;
			if (resetFailProof == 10) {
				resetFailProof = 0;
				board = initializeFromJSON(U"input.json").first;
				// board = initializeFromGet(getUrl, token, U"_input.json").first;
				progress = 100.0 * (1.0 - double(board.calculateDifference(board.grid)) / double((board.grid.height() * board.grid.width())));
				cellSize = Min(1024 / board.grid.width(), 1024 / board.grid.height());
			}
		}
		if (KeyO.down()) {
			if (board.is_goal()) {
				JSON output;
				output[U"n"] = static_cast<int32>(answer.steps.size());
				Array<JSON> ops;

				for (const auto& [solvePattern, solvePos, solveDir] : answer.steps)
				{
					JSON stepJson;
					stepJson[U"p"] = solvePattern.p;
					stepJson[U"x"] = solvePos.x;
					stepJson[U"y"] = solvePos.y;
					stepJson[U"s"] = solveDir;
					ops << stepJson;
				}

				output[U"ops"] = ops;
				Console << U"Writing ...";
				output.save(U"output.json");
				Console << U"Wrote ";
				JSON submission;
				try {
					submission = JSON::Load(U"output.json");
					const FilePath path = FileSystem::FullPath(U"output.json");
					Console << U"Full path: " << path;
					JSON submission = JSON::Load(path);
					if (not submission) {
						throw Error(U"Failed to load JSON file");
					}
					Console << U"Loaded JSON: " << submission;
				}
				catch (const Error& error) {
					Console << U"Error loading JSON: " << error.what();
				}

				// POST

			}
		}
		if (autoButton.leftClicked() || KeyEnter.down()) {
			if (board.is_goal()) {
				board = initializeFromJSON(U"input.json").first;
				std::unordered_map<Board, int32> seen;

				int cnt = 0;
				int sum = 0;
				for (const auto& [solvePattern, solvePos, solveDir] : answer.steps) {

					board.apply_pattern(solvePattern, solvePos, solveDir);


					if (seen.count(board)) {
						seen[board]++;
						Console << U"seen " << cnt;
						Console << board.grid << U" " << seen[board];
						sum++;
						Console << U"pos:{}, dir:{}"_fmt(solvePos, solveDir);
					}
					else seen[board]++;
					board.draw();
					System::Update();
					cnt++;
				}
				Console << sum;
			}
			int32 diff = board.calculateDifference(board.grid);
			int32 height = board.grid.height(), width = board.grid.width();
			auto startTime = std::chrono::high_resolution_clock::now();
			while (diff) {
				Algorithm::Solution solution;
				if (KeySpace.down()) { break; }
				for (int32 i : step(2)) {
					Algorithm::Type algo = i % 2 ? Algorithm::Type::OneByOne : Algorithm::Type::Dijkstra;
					solution = Algorithm::solve(algo, board, patterns);
					for (int32 h : step(height)) {
						for (int32 w : step(width)) {
							board.grid[h][w] = solution.grid[h][w];
						}
					}
					for (auto x : solution.steps) {
						answer.steps.push_back(x);
					}
				}
				// 変化がない時
				if (diff == board.calculateDifference(board.grid)) {
					int32 lastDiff = -1;
					while (lastDiff != diff) {
						solution = Algorithm::solve(Algorithm::Type::HorizontalSwapSort, board, patterns);
						for (int32 h : step(height)) {
							for (int32 w : step(width)) {
								board.grid[h][w] = solution.grid[h][w];
							}
						}
						for (auto x : solution.steps) {
							answer.steps.push_back(x);
						}
						lastDiff = diff;
						diff = board.calculateDifference(board.grid);
						Console << U"one by one";
					}
				}
				diff = board.calculateDifference(board.grid);
				progress = 100.0 * (1.0 - double(diff) / double((board.grid.height() * board.grid.width())));
				board.draw();
				System::Update();


			}
			auto currentTime = std::chrono::high_resolution_clock::now();
			double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();
			Console << elapsedTime << U"sec";
		}
		if (currentMode == GameMode::Manual) {
			if (Cursor::Delta().x != 0 || Cursor::Delta().y != 0) patternPos = Point(Cursor::Pos().x / cellSize, Cursor::Pos().y / cellSize);
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
			if (KeySpace.down() || MouseL.down()) board.apply_pattern(patterns[currentPattern], patternPos, direction);
		}
		else {
			if (KeyTab.down()) {
				currentAlgorithm = (currentAlgorithm + 1) % algorithms.size();
			}
			if (KeySpace.down()) {

				auto solution = Algorithm::solve(algorithms[currentAlgorithm], board, patterns);
				Console << U"step size:" << solution.steps.size();
				int32 height = board.grid.height(), width = board.grid.width();
				if (solution.grid.height() == height && solution.grid.width() == width && 1 == 0) {
					for (int32 h : step(height)) {
						for (int32 w : step(width)) {
							board.grid[h][w] = solution.grid[h][w];
						}
					}
				}
				else {
					for (const auto& [solvePattern, solvePos, solveDir] : solution.steps) {
						board.apply_pattern(solvePattern, solvePos, solveDir);
						board.draw();
						System::Update();
					}
				}
				solution.outuputToJson();
				postAnswer(postUrl, token, U"output.json");
				Console << postUrl;
				progress = 100.0 * (1.0 - double(board.calculateDifference(board.grid)) / double((board.grid.height() * board.grid.width())));
			}
			if (KeyD.down()) {
				answer = Algorithm::Solution();
				int32 diff = board.calculateDifference(board.grid);
				int32 height = board.grid.height(), width = board.grid.width();
				auto startTime = std::chrono::high_resolution_clock::now();
				while (diff) {
					Algorithm::Solution solution;
					if (KeySpace.down()) { break; }

					Algorithm::Type algo = Algorithm::Type::DynamicProgramming;
					solution = Algorithm::solve(algo, board, patterns);
					Console << U"size :" << solution.steps.size();

					for (int32 h : step(height)) {
						for (int32 w : step(width)) {
							board.grid[h][w] = solution.grid[h][w];
						}

					}

					for (const auto& x : solution.steps) {
						/*const auto& [pattern, point, direction] = x;
						board.apply_pattern(pattern, point, direction);*/
						answer.steps.push_back(x);
					}

					diff = board.calculateDifference(board.grid);
					progress = 100.0 * (1.0 - double(diff) / double((board.grid.height() * board.grid.width())));
					// board.draw();
					// System::Update();


				}
				auto currentTime = std::chrono::high_resolution_clock::now();
				double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();
				Console << elapsedTime << U"sec";
				Console << answer.steps.size() << U"terns";
			}

		}
		// 描画
		board.draw();

		// 現在のパターンを描画
		if (cellSize > 10) {
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
		manualButton.draw(currentMode == GameMode::Manual ? Palette::Red : Palette::White);
		algorithmButton.draw(currentMode == GameMode::Auto ? Palette::Blue : Palette::White);
		autoButton.draw(Palette::Green);
		resetButton.draw(Palette::Yellow);
		FontAsset(U"Button")(U"Manual").drawAt(manualButton.center(), Palette::Black);
		FontAsset(U"Button")(U"Algo").drawAt(algorithmButton.center(), Palette::Black);
		FontAsset(U"Button")(U"Auto").drawAt(autoButton.center(), Palette::Black);
		FontAsset(U"Button")(U"!!Reset!!").drawAt(resetButton.center(), Palette::Black);
		// 情報表示
		FontAsset(U"Cell")(U"Pattern: {}\nPosition: ({},{})\nDirection: {}\nMode: {}\nProgress: {}%"_fmt(
			patterns[currentPattern].p, patternPos.x, patternPos.y, U"↑↓←→"[direction],
			currentMode == GameMode::Manual ? U"Manual" : algorithmNames[currentAlgorithm],
			progress))
			.draw(1300, 300, Palette::Black);

		// ゴール状態のチェック
		if (board.is_goal()) {
			FontAsset(U"Cell")(U"Goal Reached!").drawAt(Scene::Center(), Palette::Red);
		}


	}
}
