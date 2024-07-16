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
	Window::Resize(1920, 1080);
	FontAsset::Register(U"Cell", 20);
	FontAsset::Register(U"Button", 30, Typeface::Bold);
	Scene::SetBackground(ColorF{ 0.8, 1.0, 0.9 });

	// JSONファイルからゲームを初期化
	auto [board, patterns] = initializeFromJSON(U"input.json");

	int32 currentPattern = 0;
	Point patternPos(0, 0);
	int32 direction = 0;
	int32 patternHeight = patterns[currentPattern].grid.height();
	int32 patternWidth = patterns[currentPattern].grid.width();

	Array<Algorithm::Type> algorithms = { Algorithm::Type::Greedy, Algorithm::Type::BeamSearch, Algorithm::Type::DynamicProgramming, Algorithm::Type::RowByGreedy,  Algorithm::Type::RowByRowAdvancedGreedy,
	Algorithm::Type::OneByOne, Algorithm::Type::DiagonalSearch, Algorithm::Type::SimulatedAnnealing };
	GameMode currentMode = GameMode::Manual;
	int32 currentAlgorithm = 0;
	Rect manualButton(1000, 650, 200, 50);
	Rect algorithmButton(1350, 650, 200, 50);
	Rect resetButton(1600, 650, 200, 50);
	// リセット誤爆防止 10回押したらリセットされる
	int32 resetFaleProof = 0;
	Array<String> algorithmNames = { U"Greedy", U"BeamSearch", U"DP", U"RowGreedy", U"RowGreedy改", U"OneByOne", U"Diagonal", U"Annealing"};

	double progress = 100.0 * (1.0 - double(board.calculateDifference(board.grid)) / double((board.grid.height() * board.grid.width())));

	while (System::Update()) {

		// モード切り替えボタンの描画と処理
		manualButton.draw(currentMode == GameMode::Manual ? Palette::Red : Palette::White);
		algorithmButton.draw(currentMode == GameMode::Auto ? Palette::Blue : Palette::White);
		resetButton.draw(Palette::Yellow);
		FontAsset(U"Button")(U"Manual").drawAt(manualButton.center(), Palette::Black);
		FontAsset(U"Button")(U"Auto").drawAt(algorithmButton.center(), Palette::Black);
		FontAsset(U"Button")(U"!!Reset!!").drawAt(resetButton.center(), Palette::Black);


		if (manualButton.leftClicked()) {
			currentMode = GameMode::Manual;
		}
		if (algorithmButton.leftClicked()) {
			currentMode = GameMode::Auto;
			currentAlgorithm = (currentAlgorithm + 1) % algorithms.size();
		}
		if (resetButton.leftClicked()) {
			resetFaleProof++;
			if (resetFaleProof == 10) {
				resetFaleProof = 0;
				board = initializeFromJSON(U"input.json").first;
			}
		}

		if (currentMode == GameMode::Manual) {
			// 入力処理
			if (KeyLeft.down()) patternPos.x = Max(-patternWidth + 1, patternPos.x - 1);
			if (KeyRight.down()) patternPos.x = Min(board.width - 1, patternPos.x + 1);
			if (KeyUp.down()) patternPos.y = Max(-patternHeight + 1, patternPos.y - 1);
			if (KeyDown.down()) patternPos.y = Min(board.height - 1, patternPos.y + 1);
			if (KeyTab.down()) {
				currentPattern = (currentPattern + 1) % patterns.size();
				Console << U"Selected pattern: " << patterns[currentPattern].name;
				patternHeight = patterns[currentPattern].grid.height();
				patternWidth = patterns[currentPattern].grid.width();
				patternPos = Point(0, 0);
			}
			if (KeyR.down()) direction = (direction + 1) % 4;
			if (KeySpace.down()) board.apply_pattern(patterns[currentPattern], patternPos, direction);
		}
		else {
			if (KeySpace.down()) {
				
				auto solution = Algorithm::solve(algorithms[currentAlgorithm], board, patterns);
				Console << U"step size : " << solution.steps.size();
				for (const auto& [solvePattern, solvePos, solveDir] : solution.steps) {
					board.apply_pattern(solvePattern, solvePos, solveDir);
					Console << solvePos;
				}
				progress = 100.0 * (1.0 - double(board.calculateDifference(board.grid)) / double((board.grid.height() * board.grid.width())));
			}

		}
		// 描画
		board.draw();

		// 現在のパターンを描画
		for (int32 y = 0; y < patterns[currentPattern].grid.height(); ++y) {
			for (int32 x = 0; x < patterns[currentPattern].grid.width(); ++x) {
				if (patterns[currentPattern].grid[y][x] == 1) {
					Rect((patternPos.x + x) * 30, (patternPos.y + y) * 30, 30)
						.draw(ColorF(1.0, 0.0, 0.0, 0.5));
				}
			}
		}

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
