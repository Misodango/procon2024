// Algorithm.cpp
#include "Algorithm.h"
#include <omp.h>

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


	/**
	* @brief 二次元セグメント木
	* @docs docs/data-structure-2d/2d-segment-tree.md
	*/
	/*
	class SegmentTree2D
	{
	private:
		int32 id(int32 h, int32 w) const { return h * 2 * W + w; }

	public:
		int32 H, W;
		Array<int32> seg;
		std::function<int32(int32, int32)> f;
		int32 I;

		SegmentTree2D(int32 h, int32 w, std::function<int32(int32, int32)> _f, const int32& i)
			: f(_f), I(i)
		{
			init(h, w);
		}

		void init(int32 h, int32 w)
		{
			H = W = 1;
			while (H < h) H <<= 1;
			while (W < w) W <<= 1;
			seg.resize(4 * H * W, I);
		}

		void set(int32 h, int32 w, const int32& x) { seg[id(h + H, w + W)] = x; }

		void build()
		{
			for (int32 w = W; w < 2 * W; w++)
			{
				for (int32 h = H - 1; h; h--)
				{
					seg[id(h, w)] = f(seg[id(2 * h + 0, w)], seg[id(2 * h + 1, w)]);
				}
			}
			for (int32 h = 0; h < 2 * H; h++)
			{
				for (int32 w = W - 1; w; w--)
				{
					seg[id(h, w)] = f(seg[id(h, 2 * w + 0)], seg[id(h, 2 * w + 1)]);
				}
			}
		}

		int32 get(int32 h, int32 w) const { return seg[id(h + H, w + W)]; }
		int32 operator()(int32 h, int32 w) const { return seg[id(h + H, w + W)]; }

		void update(int32 h, int32 w, const int32& x)
		{
			h += H, w += W;
			seg[id(h, w)] = x;
			for (int32 i = h >> 1; i; i >>= 1)
			{
				seg[id(i, w)] = f(seg[id(2 * i + 0, w)], seg[id(2 * i + 1, w)]);
			}
			for (; h; h >>= 1)
			{
				for (int32 j = w >> 1; j; j >>= 1)
				{
					seg[id(h, j)] = f(seg[id(h, 2 * j + 0)], seg[id(h, 2 * j + 1)]);
				}
			}
		}

		int32 _inner_query(int h, int w1, int w2)
		{
			int32 res = I;
			for (; w1 < w2; w1 >>= 1, w2 >>= 1)
			{
				if (w1 & 1) res = f(res, seg[id(h, w1)]), w1++;
				if (w2 & 1) --w2, res = f(res, seg[id(h, w2)]);
			}
			return res;
		}

		int32 query(int h1, int w1, int h2, int w2)
		{
			if (h1 >= h2 || w1 >= w2) return I;
			int32 res = I;
			h1 += H, h2 += H, w1 += W, w2 += W;
			for (; h1 < h2; h1 >>= 1, h2 >>= 1)
			{
				if (h1 & 1) res = f(res, _inner_query(h1, w1, w2)), h1++;
				if (h2 & 1) --h2, res = f(res, _inner_query(h2, w1, w2));
			}
			return res;
		}
	};
	class CountSegmentTree2D
	{
	private:
		Array<SegmentTree2D> trees;

	public:
		CountSegmentTree2D(int32 h, int32 w)
			: trees(4)
		{
			auto f = [](int32 a, int32 b) { return a + b; };
			const int32 identity = 0;

			for (auto& tree : trees)
			{
				tree = SegmentTree2D(h, w, f, identity);
			}
		}

		void set(int32 h, int32 w, int32 value)
		{
			assert(0 <= value && value < 4);
			for (int32 i = 0; i < 4; ++i)
			{
				trees[i].set(h, w, (i == value) ? 1 : 0);
			}
		}

		void build()
		{
			for (auto& tree : trees)
			{
				tree.build();
			}
		}

		Array<int32> query(int32 h1, int32 w1, int32 h2, int32 w2)
		{
			Array<int32> result(4);
			for (int32 i = 0; i < 4; ++i)
			{
				result[i] = trees[i].query(h1, w1, h2, w2);
			}
			return result;
		}
	};
	*/


	class SegmentTree {
	private:
		std::vector<std::pair<int, std::pair<int32, int32>>> tree;
		int32 n;
		int32 INF = 1e9;
	public:
		SegmentTree(int32 size) {
			n = 1;
			while (n < size) n *= 2;
			tree.resize(2 * n, { INF, {-1, -1} });
		}

		void update(int32 idx, int32 val, int32 x, int32 y) {
			idx += n;
			tree[idx] = { val, {x, y} };
			while (idx > 1) {
				idx /= 2;
				tree[idx] = min(tree[2 * idx], tree[2 * idx + 1]);
			}
		}

		std::pair<int32, std::pair<int32, int32>> query(int32 l, int32 r) {
			l += n; r += n;
			std::pair<int32, std::pair<int32, int32>> res = { INF, {-1, -1} };
			while (l < r) {
				if (l & 1) res = min(res, tree[l++]);
				if (r & 1) res = min(res, tree[--r]);
				l /= 2; r /= 2;
			}
			return res;
		}
	};

	Solution dynamicProgramming(const Board& initialBoard, const Array<Pattern>& patterns, double timeLimit) {
		// dp max step: HWlogHlogW
		// dpの遷移の方法で焼きなましができる
		// 累積和にはセグ木を使う
		Solution solution;
		Board board = initialBoard;
		int32 h = board.grid.height(), w = board.grid.width();

		while (!board.is_goal()) {
			for (int32 x : step(w)) {
				int32 beforeSize = solution.steps.size();
				for (int32 y : step(h)) {
					if (board.grid[y][x] == board.goal[y][x]) continue;
					Point nearestPoint = board.BFSbyPopcount(Point(x, y), board.goal[y][x]);
					int32 dy = nearestPoint.y - y, dx = nearestPoint.x - x;
					Console << U"sx, sy: " << Point(x, y);
					Console << U"dx dy:" << Point(dx, dy);
					if (dy == 0 && dx == 0) continue;
					// dy を愚直に処理するとき
					for (int32 bit = 0; bit <= 8; bit++) {
						if ((abs(dy) >> bit) & 1) {
							auto pattern = patterns[0];
							if (bit == 0) {
								pattern = patterns[0];
							}
							else {
								// idx: 3*(bit-1)+1
								pattern = patterns[3 * (bit - 1) + 1];
							}

							int32 dir = dy < 0; // dy > 0 => 上移動
							board.apply_pattern(pattern, Point(x + dx, y), dir);
							solution.steps.push_back(std::make_tuple(pattern, Point(x + dx, y), dir));
						}
					}

					// マイナス座標も許容することで1手で処理できる y == 0二のみ有効
					// 未完成
					/*
					if (dy > 0) {
						auto pattern = patterns[24];
						board.apply_pattern(pattern, Point(x + dx, y - dy - 256), 0);
						solution.steps.push_back(std::make_tuple(pattern, Point(x + dx, y - dy - 256), 0));
					}
					else if (dy < 0) {
						auto pattern = patterns[24];
						board.apply_pattern(pattern, Point(x + dx, y + dy + 1), 1);
						solution.steps.push_back(std::make_tuple(pattern, Point(x + dx, y + dy + 1), 1));
					}
					*/
					for (int32 bit = 0; bit <= 8; bit++) {
						if ((dx >> bit) & 1) {
							auto pattern = patterns[0];
							if (bit == 0) {
								pattern = patterns[0];
							}
							else {
								pattern = patterns[3 * (bit - 1) + 1];

							}
							board.apply_pattern(pattern, Point(x, y), 2);
							solution.steps.push_back(std::make_tuple(pattern, Point(x, y), 2));
						}
					}
					solution.grid = board.grid;
					return solution;
					// break;
				}
				// int32 afterSize = solution.steps.size();
				// if (beforeSize < afterSize) continue;
			}
		}
		return solution;

	}


	Solution rowByRowGreedy(const Board& initialBoard, const Array<Pattern>& patterns) {
		static const int32 width = initialBoard.grid.width(), height = initialBoard.grid.height();
		Board board = initialBoard;
		Solution solution;
		static Grid<int32> distances(width * height, width * height);
		static Grid<Array<std::pair<int32, Point>>> sortedDistances(width, height); // (y, x) に近いものを順に入れたい

		static bool initialized;

		if (initialized) {
			Console << U"calculated";

		}
		else {
			for (int32 y1 : step(height)) {
				for (int32 x1 : step(width)) {
					for (int32 y2 : step(height)) {
						for (int32 x2 : step(width)) {
							int32 dx = x1 - x2;
							int32 dy = y1 - y2;
							int32 dist = std::popcount(static_cast<uint32>(Abs(dx))) + std::popcount(static_cast<uint32>(Abs(dy)));
							if (dist == 0) continue;

							// Point(y1, x1) -> int32
							distances[y1 * width + x1][y2 * width + x2] = dist;
							sortedDistances[y1][x1].push_back({ dist, Point(x2, y2) });
						}
					}

					// static Grid<Array<std::pair<int32, Point>>> sortedDistances(width, height);
					std::sort(sortedDistances[y1][x1].begin(), sortedDistances[y1][x1].end(), [&](const auto& a, const auto& b) {
						return a.first < b.first;
						});
				}
			}


			initialized = true;
		}

		Array<std::pair<int32, Point>> cord;

		for (int32 x : step(width)) {
			for (int32 y : step(height)) {
				if (board.grid[y][x] == board.goal[y][x]) continue;
				for (const auto& candidate : sortedDistances[y][x]) {
					int32 targetY = candidate.second.y, targetX = candidate.second.x;
					if (board.grid[targetY][targetX] == board.goal[y][x] && board.grid[targetY][targetX] != board.goal[targetY][targetX]) {
						// return p.second;
						cord.push_back(candidate);
					}

				}
				if (board.grid[y][x] != board.goal[y][x]) {
					int32 maxStep = cord.size();
					Console << U"max step:" << maxStep;
					int32 bestDiff = 1e9;
					Solution bestSolution;
					for (int32 i : step(Min(5, maxStep))) {
						Board newBoard = board;
						const auto& points = cord[i].second;
						int32 dy = points.y - y, dx = points.x - x;
						Console << U"dx dy:" << Point(dx, dy);
						Solution currentSolution;
						if (dy == 0 && dx == 0) continue;
						for (int32 bit = 0; bit <= 8; bit++) {
							if ((abs(dy) >> bit) & 1) {
								auto pattern = patterns[0];
								if (bit == 0) {
									pattern = patterns[0];
								}
								else {
									// idx: 3*(bit-1)+1
									pattern = patterns[3 * (bit - 1) + 1];
								}

								int32 dir = dy < 0; // dy > 0 => 上移動
								newBoard.apply_pattern(pattern, Point(x + dx, y), dir);
								currentSolution.steps.push_back(std::make_tuple(pattern, Point(x + dx, y), dir));
							}
						}
						for (int32 bit = 0; bit <= 8; bit++) {
							if ((dx >> bit) & 1) {
								auto pattern = patterns[0];
								if (bit == 0) {
									pattern = patterns[0];
								}
								else {
									pattern = patterns[3 * (bit - 1) + 1];
								}
								newBoard.apply_pattern(pattern, Point(x, y), 2);
								currentSolution.steps.push_back(std::make_tuple(pattern, Point(x, y), 2));
							}
						}
						int32 sumDiff = 0;

						for (int32 ny = y; ny < height; ny++) {
							if (newBoard.grid[ny][x] != newBoard.goal[ny][x]) sumDiff++;
						}
						Console << U"sumDiff : " << sumDiff;
						if (sumDiff < bestDiff) {
							bestSolution.steps = currentSolution.steps;
							bestDiff = sumDiff;
							Console << U"bestDiff:" << bestDiff;
						}
					}
					const auto& [pat, point, d] = *bestSolution.steps.begin();
					board.apply_pattern(pat, point, d);
					solution.steps.push_back(*bestSolution.steps.begin());
				}
				return solution;
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
						// bitが立っている部分
						// 1->bit目 size:1 :: 1種類
						// otherwise :: 3種類
						// size : max 2^8
						// max 距離 255

						for (int32 bit = 0; bit <= 8; bit++) {
							if ((dx >> bit) & 1) {
								auto pattern = patterns[0];
								if (bit == 0) {
									pattern = patterns[0];
								}
								else {
									// idx: 3*(bit-1)+1
									pattern = patterns[3 * (bit - 1) + 1];
								}
								board.apply_pattern(pattern, Point(x, y), 2);
								solution.steps.push_back(std::make_tuple(pattern, Point(x, y), 2));
							}
						}

						/*
						while (dx--) {

							// board = board.applyPatternCopy(patterns[0], Point(i, j), 2);
							board.apply_pattern(patterns[0], Point(x, y), 2);

						}
						*/
						found = 1;
						break;
					}
				}

				if (!found) {
					//continue;

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

	// 他のSolution 関数 とかと違い，1のみ先読みするsolutionの集合として考える
	// solution_i と solution_{i+1}が連続でないみたいな
	// Array<Solution> にしないと動かないよ～～～～～//
	// 鬼計算量になるけど評価関数にDP入れたら精度高くなる //
	Array<Solution> nextState(const Board& initialBoad, const Array<Pattern>& patterns) {
		Array<Solution> solutions;

		// 2手以内で到達可能な状態を探す
		// 見つからなかったらdpのほうに投げる
		static const int32 bitDy[] = { 1,2,4,8,16,32,64,128 };
		// static const int32 bitDy[] = { -128,-64,-32,-16,-8,-4,-2,-1, 0,1,2,4,8,16,32,64,128 };

		static const int32 bitDx[] = { 1,2,4,8,16,32,64,128 };
		// static const int32 bitDx[] = { 0,1,2,4,8,16,32,64,128 };

		for (int32 x : step(initialBoad.grid.width())) {
			for (int32 y : step(initialBoad.grid.height())) {
				if (initialBoad.grid[y][x] == initialBoad.goal[y][x]) continue;
				for (const auto& dy : bitDy) {
					Solution solution;
					int32 ny = y + dy;
					if (ny >= initialBoad.grid.height()) continue;
					if (initialBoad.grid[ny][x] != initialBoad.goal[y][x]) continue;
					int32 bit = log2(dy);
					auto pattern = patterns[0];
					if (bit) {
						pattern = patterns[3 * (bit - 1) + 1];
					}
					int32 dir = dy < 0; // dy > 0 => 上移動
					solution.steps.emplace_back(std::make_tuple(pattern, Point(x, y), dir));
					if (solution.steps.size()) solutions.emplace_back(solution);
				}
				for (const auto& dx : bitDx) {
					Solution solution;
					int32 nx = x + dx;
					if (nx >= initialBoad.grid.width())continue;
					if (initialBoad.grid[y][nx] != initialBoad.goal[y][x]) continue;
					int32 bit = log2(dx);
					auto pattern = patterns[0];
					if (bit) {
						pattern = patterns[3 * (bit - 1) + 1];
					}
					solution.steps.emplace_back(std::make_tuple(pattern, Point(x, y), 2));
					if (solution.steps.size()) solutions.emplace_back(solution);
				}
				if (solutions.size()) return solutions;
				solutions.emplace_back(dynamicProgramming(initialBoad, patterns, 0));
				return solutions;
				//for (const auto& dy : bitDy) {
				//	for (const auto& dx : bitDx) {
				//		if (dy <= 0 && dx == 0) continue;
				//		Solution solution;
				//		int32 ny = y + dy, nx = x + dx;
				//		if (ny < 0 || ny >= initialBoad.grid.height() || nx < 0 || nx >= initialBoad.grid.width()) continue;
				//		if (initialBoad.grid[ny][nx] == initialBoad.goal[y][x]) {
				//			Console << U"x, y, nx, ny" << Point(x, y) << U" " << Point(nx, ny);
				//			for (int32 bit = 0; bit <= 8; bit++) {
				//				if ((abs(dy) >> bit) & 1) {
				//					auto pattern = patterns[0];
				//					if (bit == 0) {
				//						pattern = patterns[0];
				//					}
				//					else {
				//						// idx: 3*(bit-1)+1
				//						pattern = patterns[3 * (bit - 1) + 1];
				//					}

				//					int32 dir = dy < 0; // dy > 0 => 上移動
				//					solution.steps.push_back(std::make_tuple(pattern, Point(x + dx, y), dir));
				//				}
				//			}
				//			for (int32 bit = 0; bit <= 8; bit++) {
				//				if ((dx >> bit) & 1) {
				//					auto pattern = patterns[0];
				//					if (bit == 0) {
				//						pattern = patterns[0];
				//					}
				//					else {
				//						pattern = patterns[3 * (bit - 1) + 1];

				//					}
				//					solution.steps.push_back(std::make_tuple(pattern, Point(x, y), 2));
				//				}
				//			}
				//		}
				//		if (solution.steps.size()) solutions.push_back(solution);
				//	}
				//}
				//if (solutions.size()) return solutions;
				//solutions.push_back(dynamicProgramming(initialBoad, patterns, 0));
				//return solutions;

			}
		}
		// 見つからなかったとき
		// DP の方で返す
		Console << U"not found";
		solutions.push_back(dynamicProgramming(initialBoad, patterns, 0));
		return solutions;
	}



	/*
	Solution simulatedAnnealing(const Board& initialBoard, const Array<Pattern>& patterns, int32 sy, int32 sx,
	 int32 number, double startTemp, double endTemp) {
	 */
	Solution  simulatedAnnealing(const Board& initialBoard, const Array<Pattern>& patterns) {
		Solution solution;
		Board board = initialBoard;
		double startTemp = 50, endTemp = 10;
		auto startTime = std::chrono::high_resolution_clock::now();
		const double timeLimit = 120;
		// 行動選択の乱数
		std::mt19937 mtAction(0); // 初期化
		const int32 INF = 1 << 31;
		while (true) {
			auto currentTime = std::chrono::high_resolution_clock::now();
			double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();
			if (elapsedTime > timeLimit) {
				break;
			}
			Board newBoard = board;
			//Solution newSolutions = nextState(newBoard, patterns);
			//Console << U"new Sol size: " << newSolutions.steps.size();
			//if (newSolutions.steps.size() == 0) break;
			//int32 randomIndex = mtAction() % newSolutions.steps.size();
			//const auto& newSolution = newSolutions.steps[randomIndex];
			//const auto& [pattern, point, direction] = newSolution;
			////newBoard.grid =
			//newBoard.apply_pattern(pattern, point, direction);
			//int32 newDiff = newBoard.calculateDifference(newBoard.grid);
			//int32 preDiff = board.calculateDifference(board.grid);
			//Console << U"newDiff, preDiff" << Point(newDiff, preDiff);
			//double temp = startTemp + (endTemp - startTemp) * (elapsedTime) / timeLimit;
			//double prob = exp((preDiff - newDiff) / temp);

			//if (prob > (mtAction() % INF) / (double)(INF) || 1 == 1) {
			//	board = newBoard;
			//	solution.steps.push_back(newSolution);

			//}


		}
		/*
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
		*/
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

	Solution chokudaiSearch(const Board& board, const Array<Pattern>& patterns) {
		static const int16 height = board.grid.height();
		static const int16 width = board.grid.width();
		static const int16 beamWidth = 1000;
		static const int16 beamDepth = height * width;
		static const int16 beamNumber = 1;
		struct State {

			Board board;
			Solution solution;
			int32 score;
			State(const Board& b, const Solution& s, int32 sc) :board(b), solution(s), score(sc) {}
		};

		auto progress = [](const Board& a) {
			int32 res = 0;
			for (int32 x : step(a.width)) {
				for (int32 y : step(a.height)) {
					if (a.grid[y][x] != a.goal[y][x]) return res;
					res++;
				}
			}
			return res;
			};
		auto compareStates = [](const State& a, const State& b) {
			return a.score < b.score; // スコアが高い方が優先
			// return a.score > b.score; // スコアが低い方が優先
			};



		Solution solution;
		State state(board, solution, progress(board));

		std::vector<std::priority_queue<State, std::vector<State>, decltype(compareStates)>> beam(beamDepth + 1);
		for (int32 depth : step(beamDepth + 1)) {
			beam[depth] = std::priority_queue<State, std::vector<State>, decltype(compareStates)>(compareStates);
		}
		// beam[0].push(state);
		beam[0].emplace(board, Solution(), board.calculateDifference(board.goal));
		for (int32 cnt : step(beamNumber)) {
			for (int32 depth : step(beamDepth)) {
				auto& currentBeam = beam[depth];
				auto& nextBeam = beam[depth + 1];
				for (int32 wid : step(beamWidth)) {
					if (currentBeam.empty()) {
						break;
					}

					State currentState = currentBeam.top();
					if (currentState.board.is_goal()) {
						break;
					}

					currentBeam.pop();

					const auto& legalActions = nextState(currentState.board, patterns);
					for (const auto& solutions : legalActions) {
						Board nextBoard = currentState.board;
						int32 stepSize = solutions.steps.size();
						Solution nextSolution;
						for (const auto& action : solutions.steps) {
							const auto& [pattern, point, direction] = action;
							// Console << U"point{}, dir{}"_fmt(point, direction);
							nextBoard.apply_pattern(pattern, point, direction);
							nextSolution.steps.push_back(action);
							currentState.solution.steps.push_back(action);
						}
						double delta = progress(nextBoard) - progress(currentState.board);
						// double acc = 1 - double(nextBoard.calculateDifference(nextBoard.grid) / (height * width));
						double newScore = delta / stepSize;// *acc;

						Console << U"score:" << newScore;
						Console << U"new Prog, before ,Prog:" << Point(progress(nextBoard), progress(currentState.board));
						State newState = State(nextBoard, currentState.solution, newScore);

						if (depth == 0) {
							newState.solution = nextSolution;
						}
						for (int32 _ : step(stepSize)) currentState.solution.steps.pop_back();
						nextBeam.emplace(newState);


						// }
					}
				}
			}
		}

		for (int32 depth : step(beamDepth, 0, -1)) {
			const auto& currentBeam = beam[depth];

			if (!currentBeam.empty()) {
				return currentBeam.top().solution;
			}
		}

		return dynamicProgramming(board, patterns, 0);
	}

	Solution beamSearch(const Board& initialBoard, const Array<Pattern>& patterns, int32 beamWidth, int32 maxSteps) {
		static const int32 height = initialBoard.grid.height();
		static const int32 width = initialBoard.grid.width();
		struct State {
			Board board;
			Solution solution;
			double score;
			int32 progress;
			State(const Board& b, const Solution& s, double sc, int32 prog) : board(b), solution(s), score(sc), progress(prog) {}
		};
		// 評価関数
		// 始点から連続するマスがどれだけそろっているかをベース
		// その次に全体を見る
		auto progress = [](const Board& a) {

			int32 res = 0;
			for (int32 x : step(a.width)) {
				for (int32 y : step(a.height)) {
					if (a.grid[y][x] != a.goal[y][x]) {
						return res;
					}
					res++;
				}
			}
			};


		auto compareStates = [](const State& a, const State& b) {
			// そろっている部分が多いときに使うと手数が削減される場合がある
			/*if (a.score == b.score) {
				return a.board.calculateDifference(a.board.grid) > b.board.calculateDifference(b.board.grid);
			}*/
			return a.score < b.score; // スコアが高い方が優先
			// return a.score > b.score; // スコアが低い方が優先
			};

		std::priority_queue<State, std::vector<State>, decltype(compareStates)> beam(compareStates);
		// beam.emplace(initialBoard, Solution(), initialBoard.calculateDifference(initialBoard.goal));
		beam.emplace(initialBoard, Solution(), progress(initialBoard) * 1 - double(initialBoard.calculateDifference(initialBoard.grid) / (height * width)),
			progress(initialBoard));

		State bestState = beam.top();

		auto startTime = std::chrono::high_resolution_clock::now();


		for (int32 t : step(height* width)) {
			std::priority_queue<State, std::vector<State>, decltype(compareStates)> nextBeam;
			for (int32 w = 0; w < beamWidth; w++) {
				if (beam.empty())break;
				State currentState = beam.top();
				beam.pop();
				const auto& legalActions = nextState(currentState.board, patterns);
				for (const auto& solutions : legalActions) {
					Board nextBoard = currentState.board;
					int32 stepSize = solutions.steps.size();
					Solution nextSolution;
					for (const auto& action : solutions.steps) {
						const auto& [pattern, point, direction] = action;
						// Console << U"point{}, dir{}"_fmt(point, direction);
						nextBoard.apply_pattern(pattern, point, direction);
						nextSolution.steps.push_back(action);
						currentState.solution.steps.push_back(action);
					}
					int32 prog = progress(nextBoard);
					double delta = prog - currentState.progress;
					// double acc = 1 - double(nextBoard.calculateDifference(nextBoard.grid) / (height * width));
					double newScore = delta / stepSize;
					// double newScore = delta / stepSize * acc;
					// Console << U"score:" << newScore;
					State newState = State(nextBoard, currentState.solution, newScore, prog);
					
					if (t == 0) {
						newState.solution = nextSolution;
					}

					for (int32 _ : step(stepSize)) currentState.solution.steps.pop_back();
					nextBeam.emplace(newState);



				}
			}

			beam = nextBeam;
			bestState = beam.top();

			if (bestState.board.is_goal()) {
				auto currentTime = std::chrono::high_resolution_clock::now();
				double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();
				Console << elapsedTime << U"sec";
				break;
			}
		}
		return bestState.solution;
	}

	Solution solve(Type algorithmType, const Board& initialBoard, const Array<Pattern>& patterns) {
		Point p = Cursor::Pos();
		int32 sy = p.y / 30, sx = p.x / 30;
		switch (algorithmType) {
		case Type::Greedy:
			return greedy(initialBoard, patterns);

		case Type::BeamSearch:
			return beamSearch(initialBoard, patterns, 5, 100);

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
			return simulatedAnnealing(initialBoard, patterns);
		case Type::Dijkstra:
			return dijkstra(initialBoard, patterns);
		case Type::HorizontalSwapSort:
			return horizontalSwapSort(initialBoard, patterns);
		case Type::ChokudaiSearch:
			return chokudaiSearch(initialBoard, patterns);
		default:
			throw Error(U"Unknown algorithm type");
		}
	}
}
