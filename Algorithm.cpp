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
							int32 newDiff = newBoard.calculateDifference(newBoard.grid);

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
		std::map<size_t, bool> seen;
		while (!beam.empty() && steps < maxSteps) {
			std::vector<State> nextBeam;

			for (int i = 0; i < beamWidth && !beam.empty(); ++i) {
				State current = beam.top();
				beam.pop();
				if (seen.count(current.board.hash())) continue;
				seen[current.board.hash()] = 1;
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
				if (board.grid[y][x] == initialBoard.goal[y][x]) {
					continue;
				}
				bool found = false;
				Console << y << U", " << x;
				for (int32 dx = 1; x + dx < board.grid.width(); dx++) {
					if (board.grid[y][x + dx] == initialBoard.goal[y][x]) {
						found = true;

						while (dx--) {
							solution.steps.push_back(std::make_tuple(patterns[0], Point(x, y), 2));
							// board = board.applyPatternCopy(patterns[0], Point(i, j), 2);
							board.apply_pattern(patterns[0], Point(x, y), 2);

						}
						found = 1;
						break;
					}
				}

				if (!found) {
					//continue;
					break;
					for (int32 dy = 1; y + dy < board.grid.height(); dy++) {
						bool foundAlternative = false;
						for (int32 dx = 1; x + dx < board.grid.width(); dx++) {
							if (board.grid[y + dy][x + dx] == initialBoard.grid[y][x]) {
								foundAlternative = true;
								for (int32 i : step(dy)) {
									solution.steps.push_back(std::make_tuple(patterns[0], Point(x + dx, y), 0));
									board.apply_pattern(patterns[0], Point(x, y + dy), 0);
								}
								for (int32 j : step(dx)) {
									solution.steps.push_back(std::make_tuple(patterns[0], Point(x, y), 2));
									board.apply_pattern(patterns[0], Point(x, y), 0);
								}
								break;
							}
						}
						if (foundAlternative) break;
					}
				}
			}
		}

		for (int32 y : step(initialBoard.grid.height())) {
			for (int32 x : step(initialBoard.grid.width())) {

				if (board.grid[y][x] == initialBoard.goal[y][x]) {
					continue;
				}
				for (int32 dy = 1; y + dy < board.grid.height(); dy++) {

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
		solution.grid = board.grid;
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


		auto startTime = std::chrono::high_resolution_clock::now();
		const double timeLimit = 5;
		for (int32 i = 0; i < number; i++) {
			auto currentTime = std::chrono::high_resolution_clock::now();
			double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();

			if (elapsedTime > timeLimit) {
				break;
			}

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

	Solution dijkstra(const Board& initialBoard, const Array<Pattern>& patterns) {
		Solution solution;
		Board board = initialBoard;
		// 孤立しているマスを探す
		std::queue<Point> pos;
		for (int32 y : step(initialBoard.grid.height())) {
			if (pos.size() > 0) break;
			for (int32 x : step(initialBoard.goal.width())) {
				if (initialBoard.grid[y][x] != initialBoard.goal[y][x]) {
					pos.push(Point(x, y));
					break;
				}
			
			}
		}

		while (!pos.empty()) {
			Point nextPos = pos.front();
			pos.pop();

			Point actualPos = board.BFS(nextPos, board.goal[nextPos.y][nextPos.x]);
			Console << U"next: {}, actual:{}"_fmt(nextPos, actualPos);
			int32 dy = actualPos.y - nextPos.y, dx = actualPos.x - nextPos.x;
			Point applyPos = Point(nextPos.x + dx, nextPos.y);
			Console << U"dy, dx:{},{}"_fmt(dy, dx);
			for (int32 i : step(abs(dy))) {
				solution.steps.push_back(std::make_tuple(patterns[0], applyPos, 0));
				board.apply_pattern(patterns[0], applyPos, 0);
			}
			// return solution;
			applyPos = nextPos;
			int32 dirX = dx < 0 ? 3 : 2;
			for (int32 i : step(abs(dx))) {
				solution.steps.push_back(std::make_tuple(patterns[0], applyPos, dirX));
				board.apply_pattern(patterns[0], applyPos, dirX);
			}
			// 一旦1度で終わっとく(デバッグ)
			break;
		}
		solution.grid = board.grid;
		return solution;
	}
	// y行 swap(i,j)
	Solution swap(const Board& initialBoard, const Array<Pattern>& patterns, int32 i, int32 j, int32 y) {
		Solution solution;

		// swap
		for (int32 left : step(i)) {
			solution.steps.push_back(std::make_tuple(patterns[0], Point(0, y), 2));
		}
		for (int32 left : step(j - i - 1)) {
			solution.steps.push_back(std::make_tuple(patterns[0], Point(1, y), 2));
		}
		for (int32 right : step(1)) {
			solution.steps.push_back(std::make_tuple(patterns[0], Point(1, y), 3));
		}
		for (int32 left : step(initialBoard.grid.width() - j)) {
			solution.steps.push_back(std::make_tuple(patterns[0], Point(1, y), 2));
		}
		for (int32 right : step(i)) {
			solution.steps.push_back(std::make_tuple(patterns[0], Point(i, y), 3));
		}
		return solution;
	}

	// そのうちswapに関しては全て書き出してhashとかで保持しておけばよさそう

	Solution horizontalSwapSort(const Board& initialBoard, const Array<Pattern>& patterns) {
		Board board = initialBoard;
		Solution solution;
		// swap対象
		for (int32 y : step(board.grid.height())) {
			// 正しくない要素を探索
			std::vector<std::pair<int32, int32>> targets = board.sortToMatchPartially(y);
			for (std::pair<int32, int32> target : targets) {
				const auto [i, j] = target;
				if (i == -1 || j == -1) continue;
				// Console << U"i, j{}"_fmt(Point(i, j));
				// Console << U"before:" << board.grid;

				int32 tmp = board.grid[y][i];
				board.grid[y][i] = board.grid[y][j];
				board.grid[y][j] = tmp;
				// swap
				
				
				for (int32 left : step(i)) {
					solution.steps.push_back(std::make_tuple(patterns[0], Point(0, y), 2));
				}
				for (int32 left : step(j - i - 1)) {
					solution.steps.push_back(std::make_tuple(patterns[0], Point(1, y), 2));
				}
				for (int32 right : step(1)) {
					solution.steps.push_back(std::make_tuple(patterns[0], Point(1, y), 3));
				}
				for (int32 left : step(board.grid.width() - j)) {
					solution.steps.push_back(std::make_tuple(patterns[0], Point(1, y), 2));
				}
				for (int32 right : step(i)) {
					solution.steps.push_back(std::make_tuple(patterns[0], Point(i, y), 3));
				}
				
				// Console << U"after:" << board.grid;
				// return solution; // debug
			}
		}
		solution.grid = board.grid;
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
			return simulatedAnnealing(initialBoard, patterns, p.y / 7, p.x / 7); // セルのサイズで割る
		case Type::Dijkstra:
			return dijkstra(initialBoard, patterns);
		case Type::HorizontalSwapSort:
			return horizontalSwapSort(initialBoard, patterns);
		default:
			throw Error(U"Unknown algorithm type");
		}
	}
}
