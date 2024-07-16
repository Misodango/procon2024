// Algorithm.cpp
#include "Algorithm.h"

namespace Algorithm {

	Solution timeLimitedGreedy(const Board& initialBoard, const Array<Pattern>& patterns, int32 time) {
		Board board = initialBoard;
		Solution solution;
		solution.score = board.calculateDifference(board.grid);
		Console << U"h*w : {}, {}"_fmt(board.grid.height(), board.grid.width());
		bool improved = true;
		int32 bestDiff = solution.score;
		Pattern bestPattern = patterns[0];
		Point bestPos(0, 0);
		int32 bestDirection = 0;

		
		while (improved && !board.is_goal()) {
			improved = false;

			for (const auto& pattern : patterns) {
				std::map<size_t, bool> seen;
				seen[board.hash()] = 1;
				if (pattern.grid.height() >= Max(board.grid.height(), board.grid.width())) continue;
				for (int32 y = -(int32)pattern.grid.height() + 1; y < board.height; ++y) {
					for (int32 x = -(int32)pattern.grid.width() + 1; x < board.width; ++x) {
						Console << U"y, x : {}, {}"_fmt(y, x);
						for (int32 dir = 0; dir < 4; ++dir) {
							Board newBoard = board.applyPatternCopy(pattern, Point(x, y), dir);
							int32 newDiff = newBoard.calculateDifference(board.grid);
							if (seen.count(newBoard.hash())) {
								Console << U"seen already";
								continue;
							}
							seen[newBoard.hash()] = 1;
							Console << U"newBoard:" << newBoard.grid;
							if (newDiff < bestDiff) {
								Console << U"found";
								bestDiff = newDiff;
								bestPattern = pattern;
								bestPos = Point(x, y);
								bestDirection = dir;
								improved = true;
							}
						}
					}
				}
				if (improved) {
					board.apply_pattern(bestPattern, bestPos, bestDirection);
					solution.steps.push_back(std::make_tuple(bestPattern, bestPos, bestDirection));
					solution.score = bestDiff;
				}
				if (time != -1 && solution.steps.size() > time) {
					Console << U"found size: " << solution.steps.size();
					return solution;
				}
			}
		}
		if (solution.steps.size() == 0 && time == 1) {
			Console << U"not found";
			board.apply_pattern(bestPattern, bestPos, bestDirection);
			solution.steps.push_back(std::make_tuple(bestPattern, bestPos, bestDirection));
			solution.score = bestDiff;
		}
		return solution;
	}
	Solution greedy(const Board& initialBoard, const Array<Pattern>& patterns) {
		return timeLimitedGreedy(initialBoard, patterns, -1); // -1 -> no limit
	}

	Solution beamSearch(const Board& initialBoard, const Array<Pattern>& patterns, int32 beamWidth, int32 maxSteps) {
		struct State {
			Board board;
			Solution solution;
			int32 score;

			State(const Board& b, const Solution& s, int32 sc) : board(b), solution(s), score(sc) {}
		};

		auto compareStates = [](const State& a, const State& b) {
			return a.score > b.score; // スコアが低い方が優先
		};

		std::priority_queue<State, std::vector<State>, decltype(compareStates)> beam(compareStates);
		beam.emplace(initialBoard, Solution(), initialBoard.calculateAdvancedDifference(initialBoard.grid));

		Solution bestSolution;
		int32 bestScore = std::numeric_limits<int32>::max();
		int steps = 0;

		while (!beam.empty() && steps < maxSteps) {
			std::vector<State> nextBeam;

			for (int i = 0; i < beamWidth && !beam.empty(); ++i) {
				State current = beam.top();
				beam.pop();

				if (current.board.is_goal()) {
					return current.solution;
				}

				if (current.score < bestScore) {
					bestScore = current.score;
					bestSolution = current.solution;
				}

				for (const auto& pattern : patterns) {
					if (pattern.grid.height() >= Max(current.board.grid.height(), current.board.grid.width())) continue;
					for (int32 y = -int32(pattern.grid.height()) + 1; y < current.board.height; ++y) {
						for (int32 x = -int32(pattern.grid.width()) + 1; x < current.board.width; ++x) {
							for (int32 dir = 0; dir < 4; ++dir) {
								Board newBoard = current.board.applyPatternCopy(pattern, Point(x, y), dir);
								Solution newSolution = current.solution;
								newSolution.steps.push_back(std::make_tuple(pattern, Point(x, y), dir));

								int32 newScore = newBoard.calculateAdvancedDifference(newBoard.grid);
								newSolution.score = newScore;
								nextBeam.emplace_back(newBoard, newSolution, newScore);
							}
						}
					}
				}
			}

			std::sort(nextBeam.begin(), nextBeam.end(), compareStates);
			beam = std::priority_queue<State, std::vector<State>, decltype(compareStates)>(compareStates);
			for (int i = 0; i < std::min(beamWidth, (int)nextBeam.size()); ++i) {
				beam.emplace(std::move(nextBeam[i]));
			}

			steps++;

			// 早期終了条件: 一定回数改善が見られない場合
			if (steps % 100 == 0 && bestScore == std::numeric_limits<int32>::max()) {
				break;
			}
		}

		return bestSolution; // 最良の解を返す（ゴールに到達していなくても）
	}
	/*
	Solution dynamicProgramming(const Board& initialBoard, const Array<Pattern>& patterns, double timeLimit) {
		struct State {
			int32 score;
			std::vector<std::tuple<Pattern, Point, int32>> steps;

			State(int32 s = std::numeric_limits<int32>::max()) : score(s) {}
		};

		const int32 tileCount = initialBoard.grid.height() * initialBoard.grid.width();
		std::unordered_map<Board, State> dp;
		dp[initialBoard] = State(tileCount - initialBoard.calculateAdvancedDifference());

		std::queue<Board> q;
		q.push(initialBoard);

		Board goalBoard = Board(initialBoard.grid.height(), initialBoard.grid.width());
		int32 bestScore = std::numeric_limits<int32>::max();

		auto startTime = std::chrono::high_resolution_clock::now();

		Solution tmpSolution;

		while (!q.empty()) {

			auto currentTime = std::chrono::high_resolution_clock::now();
			double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();

			if (elapsedTime > timeLimit) {
				Console << U"DP: Time limit reached. Returning best solution found so far.";
				return tmpSolution;
				// break;
			}

			Board currentBoard = q.front();
			q.pop();

			if (currentBoard.is_goal()) {
				if (dp[currentBoard].score < bestScore) {
					goalBoard = currentBoard;
					bestScore = dp[currentBoard].score;
				}
				continue;
			}

			for (const auto& pattern : patterns) {
				if (pattern.grid.height() >= Max(initialBoard.grid.height(), initialBoard.grid.width())) continue;
				for (int32 y = -int32(pattern.grid.height()) + 1; y < currentBoard.height; ++y) {
					for (int32 x = -int32(pattern.grid.width()) + 1; x < currentBoard.width; ++x) {
						for (int32 dir = 0; dir < 4; ++dir) {
							Board newBoard = currentBoard.applyPatternCopy(pattern, Point(x, y), dir);
							// int32 newScore = dp[currentBoard].score + pattern.p;
							int32 newScore = dp[currentBoard].score + tileCount - newBoard.calculateDifference(initialBoard.goal);
							if (dp.find(newBoard) == dp.end() || newScore < dp[newBoard].score) {
								dp[newBoard] = State(newScore);
								dp[newBoard].steps = dp[currentBoard].steps;
								dp[newBoard].steps.push_back(std::make_tuple(pattern, Point(x, y), dir));
								q.push(newBoard);
								tmpSolution.steps = dp[newBoard].steps;
								tmpSolution.score = newScore;
								Console << newScore;
							}
						}
					}
				}
			}
		}

		if (bestScore == std::numeric_limits<int32>::max()) {
			return Solution(); // No solution found
		}

		Solution solution;
		solution.score = bestScore;
		solution.steps = dp[goalBoard].steps;
		return solution;
	}
	*/

	Solution dynamicProgramming(const Board& initialBoard, const Array<Pattern>& patterns, double timeLimit) {
		struct State {
			int32 score;
			std::vector<std::tuple<Pattern, Point, int32>> steps;

			State(int32 s = std::numeric_limits<int32>::max()) : score(s) {}
		};
		const int32 tileCount = initialBoard.grid.height() * initialBoard.grid.width();
		std::unordered_map<Board, State> dp;
		dp[initialBoard] = State(tileCount - initialBoard.calculateAdvancedDifference(initialBoard.grid));

		std::queue<Board> q;
		q.push(initialBoard);

		Board goalBoard = initialBoard;
		int32 bestScore = std::numeric_limits<int32>::max();

		auto startTime = std::chrono::high_resolution_clock::now();

		while (!q.empty()) {
			auto currentTime = std::chrono::high_resolution_clock::now();
			double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();

			if (elapsedTime > timeLimit) {
				break;
			}

			Board currentBoard = q.front();
			q.pop();

			if (currentBoard.is_goal()) {
				if (dp[currentBoard].score < bestScore) {
					goalBoard = currentBoard;
					bestScore = dp[currentBoard].score;
				}
				continue;
			}

			for (const auto& pattern : patterns) {
				if (pattern.grid.height() >= Max(currentBoard.grid.height(), currentBoard.grid.width())) continue;
				for (int32 y = -int32(pattern.grid.height()) + 1; y < currentBoard.height; ++y) {
					for (int32 x = -int32(pattern.grid.width()) + 1; x < currentBoard.width; ++x) {
						for (int32 dir = 0; dir < 4; ++dir) {
							Board newBoard = currentBoard.applyPatternCopy(pattern, Point(x, y), dir);
							int32 newScore = tileCount - newBoard.calculateAdvancedDifference(newBoard.grid);
							if (dp.find(newBoard) == dp.end() || newScore < dp[newBoard].score) {
								dp[newBoard] = State(newScore);
								dp[newBoard].steps = dp[currentBoard].steps;
								dp[newBoard].steps.push_back(std::make_tuple(pattern, Point(x, y), dir));
								q.push(newBoard);
							}
						}
					}
				}
			}
		}

		if (bestScore == std::numeric_limits<int32>::max()) {
			return Solution(); // No solution found
		}

		Solution solution;
		solution.score = bestScore;
		solution.steps = dp[goalBoard].steps;
		return solution;
	}

	/*
	Solution rowByRowGreedy(const Board& initialBoard, const Array<Pattern>& patterns) {
		Board currentBoard = initialBoard;
		Solution solution;
		solution.score = 0;

		for (int32 targetRow = 0; targetRow < currentBoard.height; ++targetRow) {
			while (!currentBoard.isRowMatched(targetRow)) {
				bool improved = false;
				int32 bestDiff = currentBoard.calculateDifferenceByRow(targetRow);
				Pattern bestPattern = patterns[0];
				Point bestPos(0, 0);
				int32 bestDirection = 0;

				for (const auto& pattern : patterns) {
					if (pattern.grid.height() >= Max(initialBoard.grid.height(), initialBoard.grid.width())) continue;
					int32 minY = (targetRow == 0) ? -int32(pattern.grid.height()) + 1 : targetRow;
					int32 maxY = targetRow;

					for (int32 y = minY; y <= maxY; ++y) {
						for (int32 x = -int32(pattern.grid.width()) + 1; x < currentBoard.width; ++x) {
							for (int32 dir = 0; dir < 4; ++dir) {
								Board newBoard = currentBoard.applyPatternCopy(pattern, Point(x, y), dir);
								int32 newDiff = newBoard.calculateDifferenceByRow(targetRow);
								if (newDiff < bestDiff) {
									bestDiff = newDiff;
									bestPattern = pattern;
									bestPos = Point(x, y);
									bestDirection = dir;
									improved = true;
								}
							}
						}
					}
				}

				if (improved) {
					currentBoard.apply_pattern(bestPattern, bestPos, bestDirection);
					solution.steps.push_back(std::make_tuple(bestPattern, bestPos, bestDirection));
					solution.score += bestPattern.p;
					//solution.score +=
				}
				else {
					// 改善できない場合、次の行に移動
					break;
				}
			}
		}

		return solution;
	}*/

	Solution rowByRowGreedy(const Board& initialBoard, const Array<Pattern>& patterns) {
		Board currentBoard = initialBoard;
		Solution solution;
		solution.score = 0;

		for (int32 targetRow = 0; targetRow < currentBoard.height; ++targetRow) {
			while (!currentBoard.isRowMatched(targetRow, currentBoard.grid)) {
				bool improved = false;
				int32 bestDiff = currentBoard.calculateDifferenceByRow(targetRow, currentBoard.grid);
				Pattern bestPattern = patterns[0];
				Point bestPos(0, 0);
				int32 bestDirection = 0;

				for (const auto& pattern : patterns) {
					if (pattern.grid.height() >= Max(initialBoard.grid.height(), initialBoard.grid.width())) continue;
					int32 minY = (targetRow == 0) ? -int32(pattern.grid.height()) + 1 : targetRow;
					int32 maxY = targetRow;

					for (int32 y = minY; y <= maxY; ++y) {
						for (int32 x = -int32(pattern.grid.width()) + 1; x < currentBoard.width; ++x) {
							for (int32 dir = 0; dir < 4; ++dir) {
								Board newBoard = currentBoard.applyPatternCopy(pattern, Point(x, y), dir);
								int32 newDiff = newBoard.calculateDifferenceByRow(targetRow, newBoard.grid);
								if (newDiff < bestDiff) {
									bestDiff = newDiff;
									bestPattern = pattern;
									bestPos = Point(x, y);
									bestDirection = dir;
									improved = true;
									if (newDiff == 0) {
										currentBoard.apply_pattern(bestPattern, bestPos, bestDirection);
										solution.steps.push_back(std::make_tuple(bestPattern, bestPos, bestDirection));
										solution.score += currentBoard.calculateDifference(currentBoard.goal);
										return solution;
									}
								}
							}
						}
					}
				}

				if (improved) {
					currentBoard.apply_pattern(bestPattern, bestPos, bestDirection);
					solution.steps.push_back(std::make_tuple(bestPattern, bestPos, bestDirection));
					solution.score += currentBoard.calculateDifference(currentBoard.goal);
				}
				else {
					break;
				}
			}
		}

		return solution;
	}

	/*
	Solution rowByRowAdvancedGreedy(const Board& initialBoard, const Array<Pattern>& patterns) {
		Board currentBoard = initialBoard;
		Solution solution;
		solution.score = 0;

		for (int32 targetRow = 0; targetRow < currentBoard.height; ++targetRow) {
			while (!currentBoard.isRowMatched(targetRow)) {
				bool improved = false;
				int32 bestDiff = currentBoard.calculateAdvancedDifferenceByRow(targetRow);
				Pattern bestPattern = patterns[0];
				Point bestPos(0, 0);
				int32 bestDirection = 0;

				for (const auto& pattern : patterns) {
					if (pattern.grid.height() >= Max(initialBoard.grid.height(), initialBoard.grid.width())) continue;
					int32 minY = (targetRow == 0) ? -int32(pattern.grid.height()) + 1 : targetRow;
					int32 maxY = targetRow;

					for (int32 y = minY; y <= maxY; ++y) {
						for (int32 x = -int32(pattern.grid.width()) + 1; x < currentBoard.width; ++x) {
							for (int32 dir = 0; dir < 4; ++dir) {
								Board newBoard = currentBoard.applyPatternCopy(pattern, Point(x, y), dir);
								int32 newDiff = newBoard.calculateAdvancedDifferenceByRow(targetRow);
								if (newDiff < bestDiff) {
									bestDiff = newDiff;
									bestPattern = pattern;
									bestPos = Point(x, y);
									bestDirection = dir;
									improved = true;
								}
							}
						}
					}
				}

				if (improved) {
					currentBoard.apply_pattern(bestPattern, bestPos, bestDirection);
					solution.steps.push_back(std::make_tuple(bestPattern, bestPos, bestDirection));
					solution.score += bestPattern.p;
				}
				else {
					break;
				}
			}
		}

		return solution;
	}*/


	Solution rowByRowAdvancedGreedy(const Board& initialBoard, const Array<Pattern>& patterns) {
		Board currentBoard = initialBoard;
		Solution solution;
		solution.score = 0;

		for (int32 targetRow = 0; targetRow < currentBoard.height; ++targetRow) {
			while (!currentBoard.isRowMatched(targetRow, currentBoard.grid)) {
				bool improved = false;
				int32 bestDiff = currentBoard.calculateAdvancedDifferenceByRow(targetRow, currentBoard.grid);
				Pattern bestPattern = patterns[0];
				Point bestPos(0, 0);
				int32 bestDirection = 0;

				for (const auto& pattern : patterns) {
					if (pattern.grid.height() >= Max(initialBoard.grid.height(), initialBoard.grid.width())) continue;
					int32 minY = (targetRow == 0) ? -int32(pattern.grid.height()) + 1 : targetRow;
					int32 maxY = targetRow;

					for (int32 y = minY; y <= maxY; ++y) {
						for (int32 x = -int32(pattern.grid.width()) + 1; x < currentBoard.width; ++x) {
							for (int32 dir = 0; dir < 4; ++dir) {
								Board newBoard = currentBoard.applyPatternCopy(pattern, Point(x, y), dir);
								int32 newDiff = newBoard.calculateAdvancedDifferenceByRow(targetRow, currentBoard.grid);
								if (newDiff < bestDiff) {
									bestDiff = newDiff;
									bestPattern = pattern;
									bestPos = Point(x, y);
									bestDirection = dir;
									improved = true;
								}
							}
						}
					}
				}

				if (improved) {
					currentBoard.apply_pattern(bestPattern, bestPos, bestDirection);
					solution.steps.push_back(std::make_tuple(bestPattern, bestPos, bestDirection));
					solution.score += currentBoard.calculateAdvancedDifference(currentBoard.grid);
				}
				else {
					break;
				}
			}
		}

		return solution;
	}





	Solution oneByOne(const Board& initialBoard, const Array<Pattern>& patterns) {
		Board board = initialBoard;
		Solution solution;
		for (int32 x : step(initialBoard.grid.width())) {
			for (int32 y : step(initialBoard.grid.height())) {
				Console << U"y,x: {},{}"_fmt(y, x);
				if (board.grid[y][x] == initialBoard.goal[y][x]) {
					continue;
				}

				for (int32 dx = 1; x + dx < board.grid.width(); dx++) {
					if (board.grid[y][x + dx] == initialBoard.goal[y][x]) {
						Console << U"x+dx: {}"_fmt(x + dx);
						while (dx--) {
							solution.steps.push_back(std::make_tuple(patterns[0], Point(x, y), 2));
							// board = board.applyPatternCopy(patterns[0], Point(i, j), 2);
							board.apply_pattern(patterns[0], Point(x, y), 2);
							Console << board.grid;
						}
						break;
					}
				}
			}
		}

		for (int32 y : step(initialBoard.grid.height())) {
			for (int32 x : step(initialBoard.grid.width())) {
				Console << U"y,x: {},{}"_fmt(y, x);
				if (board.grid[y][x] == initialBoard.goal[y][x]) {
					continue;
				}
				for (int32 dy = 1; y + dy < board.grid.height(); dy++) {
					Console << U"y+dy: {}"_fmt(y + dy);
					if (board.grid[y + dy][x] == initialBoard.goal[y][x]) {
						while (dy--) {
							solution.steps.push_back(std::make_tuple(patterns[0], Point(x, y), 0));
							board.apply_pattern(patterns[0], Point(x, y), 0);
						}
						break;
					}
				}
			}
		}

		return solution;
	}

	// 対角成分探索
	Solution diagonalSearch(const Board& initialBoard, const Array<Pattern>& patterns) {
		Board board = initialBoard;
		Solution solution;
		solution.score = 0;

		for (int32 i : step(Min(initialBoard.grid.height(), initialBoard.grid.width()))) {
			for (int32 x = i; x < initialBoard.grid.width(); x++) {
				if (board.grid[i][x] == initialBoard.goal[i][x]) {
					continue;
				}
				for (int32 dx = 1; x + dx < board.grid.width(); dx++) {
					Console << U"i, x, i+dx: {}, {}, {}"_fmt(i, x, i + dx);
					if (board.grid[i][x + dx] == initialBoard.goal[i][x]) {
						while (dx--) {
							solution.steps.push_back(std::make_tuple(patterns[0], Point(x, i), 2));
							// board = board.applyPatternCopy(patterns[0], Point(i, j), 2);
							board.apply_pattern(patterns[0], Point(x, i), 2);

							Console << board.grid;
						}
						break;
					}
				}
			}
			for (int32 y = i; y < initialBoard.grid.height(); y++) {
				if (board.grid[y][i] == initialBoard.goal[y][i]) {
					continue;
				}
				for (int32 dy = 1; y + dy < board.grid.height(); dy++) {
					Console << U"y, i , i+dy: {}, {}, {} = "_fmt(y, i, i + dy) << board.grid[y + dy][i];
					if (board.grid[y + dy][i] == initialBoard.goal[y][i]) {
						while (dy--) {
							solution.steps.push_back(std::make_tuple(patterns[0], Point(i, y), 0));
							board.apply_pattern(patterns[0], Point(i, y), 0);
							Console << board.grid;
						}
						break;
					}
				}
			}
		}
		return solution;
	}

	/*Solution partialBFS(const Board& initialBoard, const Array<Pattern>& patterns, const int32& sy, const int32& sx) {
		Solution solution;
		int32 height = initialBoard.grid.height() - sy;
		int32 width = initialBoard.grid.width() - sx;
		Grid<int32> partialGrid = initialBoard.partialGrid(sy, sx);
		Grid<int32> partialGoal = initialBoard.partialGoal(sy, sx);
		Board partialBoard = {height, width};
		partialBoard.grid = partialGrid;
		partialBoard.goal = partialGoal;

		std::queue<Board> que;
		que.push(partialBoard);
		std::map<Grid<int32>, bool>  seen;
		seen[partialGrid] = 1;
		// pattern[0]で固定
		Pattern pattern = patterns[0];
		while (!que.empty()) {
			Board board = que.front();
			que.pop();

			for (int32 i = 0; i < height; i++) {
				for (int32 j = 0; j < width; j++) {
					for (int32 dir = 0; dir < 4; dir++) {
						Board nextBoard = board.applyPatternCopy(pattern, Point(i, j), dir);
						if (seen[nextBoard.grid]) continue;
						que.push(nextBoard);
					}
				}
			}

		}
		for (int32 y = sy; y < initialBoard.grid.height(); y++) {
			for (int32 x = sx; x < initialBoard.grid.width(); x++) {

			}
		}
	}
	*/

	Solution simulatedAnnealing(const Board& initialBoard, const Array<Pattern>& patterns, int32 sy, int32 sx,
	 int32 number, double startTemp, double endTemp) {
		Solution solution;
		Console << U"sx, sy = {}"_fmt(Point(sx, sy));
		int32 height = initialBoard.grid.height() - sy;
		int32 width = initialBoard.grid.width() - sx;
		Grid<int32> partialGrid = initialBoard.partialGrid(sy, sx);
		Console << U"partial grid :\n" << partialGrid;
		Grid<int32> partialGoal = initialBoard.partialGoal(sy, sx);
		Console << U"partial goal :\n" << partialGoal;
		Board partialBoard = { width, height };
		partialBoard.grid = partialGrid;
		partialBoard.goal = partialGoal;
		Console << height << U", " << width;

		int32 bestScore = partialBoard.calculateDifference(partialGrid);
		int32 currentScore = bestScore;
		auto bestBoard = partialBoard;

		// 行動選択の乱数
		std::mt19937 mtAction(0); // 初期化
		const int32 INF = 1 << 31;



		for (int32 i = 0; i < number; i++) {
			Console << U"time : {} board:\n{} "_fmt(i, partialBoard.grid);
			Solution newSolution = timeLimitedGreedy(partialBoard, patterns, 1);
			Console << U"size: " << newSolution.steps.size();
			const auto& [solvePattern, solvePos, solveDir] = newSolution.steps[0];
			auto nextBoard = partialBoard.applyPatternCopy(solvePattern, solvePos, solveDir);
			int32 nextScore = nextBoard.calculateDifference(nextBoard.grid);

			double temp = startTemp + (endTemp - startTemp) * (i / number);
			double probablity = exp((nextScore - currentScore) / temp);
			bool isForceNext = probablity > (mtAction() % INF) / double(INF);
			if (nextScore > currentScore || isForceNext) {
				currentScore = nextScore;
				partialBoard = nextBoard;
				Point actualPos = Point(solvePos.x + sx, solvePos.y + sy);
				solution.steps.push_back(std::make_tuple(solvePattern, actualPos, solveDir));
			}

			if (nextScore > bestScore) {
				bestScore = nextScore;
				bestBoard = partialBoard;
			}

		}

		return solution;
	}


	Solution solve(Type algorithmType, const Board& initialBoard, const Array<Pattern>& patterns) {
		Point p = Cursor::Pos();
		int32 sy = p.y / 30, sx = p.x / 30;
		switch (algorithmType) {
		case Type::Greedy:
			return greedy(initialBoard, patterns);

		case Type::BeamSearch:
			return beamSearch(initialBoard, patterns, 50, 1000);

		case Type::DynamicProgramming:
			return dynamicProgramming(initialBoard, patterns, 5);

		case Type::RowByGreedy:
			return rowByRowGreedy(initialBoard, patterns);
		case Type::RowByRowAdvancedGreedy:
			return rowByRowAdvancedGreedy(initialBoard, patterns);
		case Type::OneByOne:
			return oneByOne(initialBoard, patterns);
		case Type::DiagonalSearch:
			return diagonalSearch(initialBoard, patterns);
		case Type::SimulatedAnnealing:
			if (sy < 0 || sy >= initialBoard.grid.height() || sx < 0 || sx >= initialBoard.grid.width()) return Solution();
			return simulatedAnnealing(initialBoard, patterns, p.y / 30, p.x / 30);
		default:
			throw Error(U"Unknown algorithm type");
		}
	}
}
