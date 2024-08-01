# include <Siv3D.hpp>

#include "Pattern.h"
#include "StandardPatterns.h"
#include "Board.h"
#include "Algorithm.h"
#include "GameMode.h"


// JSONからゲームの初期化を行う関数
std::pair<Board, Array<Pattern>> initializeFromJSON(const FilePath& path) {
	JSON json;
	Board board(1, 1);  // デフォルトの空のボードを作成
	Array<Pattern> patterns = StandardPatterns::getAllStandardPatterns_Grid();

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

void Main() {
	// Window::Resize(1920, 1080);
	const auto monitor = System::EnumerateMonitors()[0];
	
	Window::Resize(monitor.fullscreenResolution); 
	FontAsset::Register(U"Cell", 20);
	FontAsset::Register(U"Button", 30, Typeface::Bold);
	Scene::SetBackground(ColorF{ 0.8, 1.0, 0.9 });



	// JSONファイルからゲームを初期化
	auto [board, patterns] = initializeFromJSON(U"input.json");
	int32 cellSize = Min(1920 / board.grid.width(), 1080 / board.grid.height());
	int32 currentPattern = 0;
	Point patternPos(0, 0);
	int32 direction = 0;
	int32 patternHeight = patterns[currentPattern].grid.height();
	int32 patternWidth = patterns[currentPattern].grid.width();

	Array<Algorithm::Type> algorithms = { Algorithm::Type::Greedy, Algorithm::Type::BeamSearch, Algorithm::Type::DynamicProgramming, Algorithm::Type::RowByGreedy,  Algorithm::Type::RowByRowAdvancedGreedy,
	Algorithm::Type::OneByOne, Algorithm::Type::DiagonalSearch, Algorithm::Type::SimulatedAnnealing, Algorithm::Type::Dijkstra, Algorithm::Type::HorizontalSwapSort };
	GameMode currentMode = GameMode::Manual;
	int32 currentAlgorithm = 0;
	Rect manualButton(1000, 650, 200, 50);
	Rect algorithmButton(1350, 650, 200, 50);
	Rect autoButton(1350, 900, 200, 50);
	Rect resetButton(1600, 650, 200, 50);
	// リセット誤爆防止 10回押したらリセットされる
	int32 resetFailProof = 0;
	Array<String> algorithmNames = { U"Greedy", U"BeamSearch", U"DP", U"RowGreedy", U"RowGreedy改", U"OneByOne", U"Diagonal", U"Annealing", U"dijkstra" , U"水平スワップソート0" };

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
				progress = 100.0 * (1.0 - double(board.calculateDifference(board.grid)) / double((board.grid.height() * board.grid.width())));
			}
		}
		if (autoButton.leftClicked() || KeyEnter.down()) {
			if (board.is_goal()) {
				board = initializeFromJSON(U"input.json").first;
				for (const auto& [solvePattern, solvePos, solveDir] : answer.steps) {
					
					board.apply_pattern(solvePattern, solvePos, solveDir);
					board.draw();
					System::Update();
				}
			}
			int32 diff = board.calculateDifference(board.grid);
			int32 height = board.grid.height(), width = board.grid.width();
			auto startTime = std::chrono::high_resolution_clock::now();
			while (diff) {
				Algorithm::Solution solution;
				if (KeySpace.down()) { break; }
				for (int32 i : step(2)) {
					Algorithm::Type algo = i % 2 ? Algorithm::Type::HorizontalSwapSort : Algorithm::Type::Dijkstra;
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
				if (diff = board.calculateDifference(board.grid)) {
					int32 lastDiff = -1;
					while (lastDiff != diff) {
						solution = Algorithm::solve(Algorithm::Type::OneByOne, board, patterns);
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
			patternPos = Point(Cursor::Pos().x / cellSize, Cursor::Pos().y / cellSize);
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
				if (solution.grid.height() == height && solution.grid.width() == width) {
					for (int32 h : step(height)) {
						for (int32 w : step(width)) {
							board.grid[h][w] = solution.grid[h][w];
						}
					}
				}
				else {
					for (const auto& [solvePattern, solvePos, solveDir] : solution.steps) {
						board.apply_pattern(solvePattern, solvePos, solveDir);

					}
				}

				progress = 100.0 * (1.0 - double(board.calculateDifference(board.grid)) / double((board.grid.height() * board.grid.width())));
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
