// Algorithm.cpp
#include "Algorithm.h"
#include <omp.h>

namespace Algorithm {

	class OptimizedBoard {
	private:
		std::vector<uint64_t> grid;
		std::vector<uint64_t> goal;
		static constexpr int CELLS_PER_UINT64 = 32;
		static constexpr uint64_t MASK = 0x3; // 11 in binary

		int calculateIndex(int x, int y) const {
			return y * width + x;
		}

		// Helper function to calculate popcount
		int popcount(int n) const {
			// return std::popcount(static_cast<unsigned>(n));
			return std::popcount(static_cast<uint32_t>(n));
			// return __builtin_popcount(n);
		}


		int getYFromIndex(int index) const {
			return index / width;
		}


	public:
		int width, height;
		OptimizedBoard(int w, int h) : width(w), height(h) {
			int cellCount = width * height;
			int uint64Count = (cellCount + CELLS_PER_UINT64 - 1) / CELLS_PER_UINT64;
			grid.resize(uint64Count, 0);
		}

		OptimizedBoard(int w, int h, const Grid<int>& gr, const Grid<int>& go) :width(w), height(h) {
			int cellCount = width * height;
			int uint64Count = (cellCount + CELLS_PER_UINT64 - 1) / CELLS_PER_UINT64;
			grid.resize(uint64Count, 0);
			goal.resize(uint64Count, 0);
			setGrid(gr);
			setGoal(go);
		}

		bool operator==(const OptimizedBoard& other) const { return grid == other.grid; };

		void set(int x, int y, int value) {
			int index = y * width + x;
			int arrayIndex = index / CELLS_PER_UINT64;
			int bitIndex = (index % CELLS_PER_UINT64) * 2;

			uint64_t clearMask = ~(MASK << bitIndex);
			grid[arrayIndex] = (grid[arrayIndex] & clearMask) |
				(static_cast<uint64_t>(value) << bitIndex);
		}

		void _set(int x, int y, int value) {
			int index = y * width + x;
			int arrayIndex = index / CELLS_PER_UINT64;
			int bitIndex = (index % CELLS_PER_UINT64) * 2;

			uint64_t clearMask = ~(MASK << bitIndex);
			goal[arrayIndex] = (goal[arrayIndex] & clearMask) |
				(static_cast<uint64_t>(value) << bitIndex);
		}

		int getGrid(int x, int y) const {
			if (x >= width || y >= height)return -1;
			int index = y * width + x;
			int arrayIndex = index / CELLS_PER_UINT64;
			int bitIndex = (index % CELLS_PER_UINT64) * 2;

			return (grid[arrayIndex] >> bitIndex) & MASK;
		}

		int getGoal(int x, int y) const {
			if (x >= width || y >= height)return -1;
			int index = y * width + x;
			int arrayIndex = index / CELLS_PER_UINT64;
			int bitIndex = (index % CELLS_PER_UINT64) * 2;

			return (goal[arrayIndex] >> bitIndex) & MASK;
		}

		void setGrid(const Grid<int>& grid) {
			for (int i : step(grid.height())) {
				for (int j : step(grid.width())) {
					set(j, i, grid[i][j]);
				}
			}
		}

		void setGoal(const Grid<int>& goal) {
			for (int i : step(goal.height())) {
				for (int j : step(goal.width())) {
					_set(j, i, goal[i][j]);
				}
			}
		}

		void print() {
			for (int i = 0; i < height; i++) {
				for (int j = 0; j < width; j++) {
					std::cout << getGrid(j, i) << " ";
				}
				std::cout << "\n";
			}
		}

		void shift_up(const Grid<bool>& isRemoved) {
			for (int x = 0; x < width; ++x) {
				std::vector<int> column;
				std::vector<int> removed_in_col;
				for (int y = 0; y < height; ++y) {
					if (!isRemoved[y][x]) {
						column.push_back(getGrid(x, y));
					}
					else {
						removed_in_col.push_back(getGrid(x, y));
					}
				}
				int y = 0;
				for (int value : column) {
					set(x, y, value);
					++y;
				}
				for (int value : removed_in_col) {
					set(x, y, value);
					++y;
				}
			}
		}

		void shift_down(const Grid<bool>& isRemoved) {
			for (int x = 0; x < width; ++x) {
				std::vector<int> column;
				std::vector<int> removed_in_col;
				for (int y = height - 1; y >= 0; --y) {
					if (!isRemoved[y][x]) {
						column.push_back(getGrid(x, y));
					}
					else {
						removed_in_col.insert(removed_in_col.begin(), getGrid(x, y));
					}
				}
				int y = height - 1;
				for (int value : column) {
					set(x, y, value);
					--y;
				}
				std::reverse(removed_in_col.begin(), removed_in_col.end());
				for (int value : removed_in_col) {
					set(x, y, value);
					--y;
				}
			}
		}

		void shift_left(const Grid<bool>& isRemoved) {
			for (int y = 0; y < height; ++y) {
				std::vector<int> row;
				std::vector<int> removed_in_row;
				for (int x = 0; x < width; ++x) {
					if (!isRemoved[y][x]) {
						row.push_back(getGrid(x, y));
					}
					else {
						removed_in_row.push_back(getGrid(x, y));
					}
				}
				int x = 0;
				for (int value : row) {
					set(x, y, value);
					++x;
				}
				for (int value : removed_in_row) {
					set(x, y, value);
					++x;
				}
			}
		}

		void shift_right(const Grid<bool>& isRemoved) {
			for (int y = 0; y < height; ++y) {
				std::vector<int> row;
				std::vector<int> removed_in_row;
				for (int x = width - 1; x >= 0; --x) {
					if (!isRemoved[y][x]) {
						row.push_back(getGrid(x, y));
					}
					else {
						removed_in_row.insert(removed_in_row.begin(), getGrid(x, y));
					}
				}
				int x = width - 1;
				for (int value : row) {
					set(x, y, value);
					--x;
				}
				std::reverse(removed_in_row.begin(), removed_in_row.end());
				for (int value : removed_in_row) {
					set(x, y, value);
					--x;
				}
			}
		}


		void apply_pattern(const Pattern& pattern, Point pos, int direction) {
			Grid<bool> isRemoved(width, height, false);
			for (int y = 0; y < pattern.grid.height(); ++y) {
				for (int x = 0; x < pattern.grid.width(); ++x) {
					if (pattern.grid[y][x] == 1) {
						int bx = pos.x + x, by = pos.y + y;
						if (0 <= bx && bx < width && 0 <= by && by < height) {
							isRemoved[by][bx] = true;
						}
					}
				}
			}

			switch (direction) {
			case 0: // up
				shift_up(isRemoved);
				break;
			case 1: // down
				shift_down(isRemoved);
				break;
			case 2: // left
				shift_left(isRemoved);
				break;
			case 3: // right
				shift_right(isRemoved);
				break;
			}

		}

		int getCorrectCountAll() const {
			int count = 0;
			for (size_t i = 0; i < grid.size(); ++i) {
				uint64_t g = grid[i];
				uint64_t go = goal[i];
				for (size_t bit = 0; bit < sizeof(uint64_t) * 8; bit += 2) {
					// Extract 2-bit segments from both grid and goal
					uint64_t gSegment = (g >> bit) & 0b11;
					uint64_t goSegment = (go >> bit) & 0b11;
					// Check if they match
					if (gSegment == goSegment) {
						count += 1; // Increment count if the 2-bit segments match
					}
				}
			}
			return count;
		}

		// 何マスまで揃っているか
		int getCorrectCount() const {
			int count = 0;
			int totalCells = width * height;
			int uint64Count = (totalCells + CELLS_PER_UINT64 - 1) / CELLS_PER_UINT64;


			for (int i = 0; i < uint64Count; ++i) {
				uint64_t diff = grid[i] ^ goal[i];
				if (diff != 0) {
					for (int j = 0; j < CELLS_PER_UINT64 && count < totalCells; ++j) {
						if ((diff & MASK) != 0) {
							return count;
						}
						diff >>= 2;
						count++;
					}
				}
				else {
					count += CELLS_PER_UINT64;
					if (count > totalCells) {
						count = totalCells;
						break;
					}
				}
			}
			return count;
		}
		Point findClosestPointWithSameValue(int a, int b) const {
			int targetValue = getGoal(a, b);
			int targetIndex = calculateIndex(a, b);
			std::vector<std::pair<int, int>> candidates;

			// Step 1 & 2: Find all points with the same value
			for (int y = b; y < height; ++y) {
				for (int x = a; x < width; ++x) {
					if (getGrid(x, y) == targetValue) {
						candidates.emplace_back(x, y);
						//int index = calculateIndex(x, y);
						//if (index > targetIndex) {  // Step 4: Filter condition
						//	candidates.emplace_back(x, y);
						//}
					}
				}
			}

			if (candidates.empty()) {
				return { -1, -1 };  // No valid point found
			}

			// Step 3 & 5: Calculate popcount differences and find the minimum
			int minPopcountDiff = std::numeric_limits<int>::max();
			// std::pair<int, int> closestPoint = { -1, -1 };
			Point closestPoint = { -1, -1 };
			for (const auto& [x, y] : candidates) {
				int popcountDiff = popcount(y - b);
				if (popcountDiff < minPopcountDiff) {
					minPopcountDiff = popcountDiff;
					closestPoint = { x,y };
				}
			}
			/*int targetPopcount = popcount(targetIndex);
			for (const auto& [x, y] : candidates) {
				int index = calculateIndex(x, y);
				int popcountDiff = std::abs(popcount(index) - targetPopcount);

				if (popcountDiff < minPopcountDiff) {
					minPopcountDiff = popcountDiff;
					closestPoint = { x, y };
				}
			}*/

			return closestPoint;
		}
		// (x, y)
		std::vector<std::pair<int, int>> findPointsWithSameValueAndYPopcountDiff1(int a, int b) const {
			int targetValue = getGoal(a, b);
			// int targetIndex = calculateIndex(a, b);
			// int targetYPopcount = popcount(getYFromIndex(targetIndex));
			std::vector<std::pair<int, int>> result;
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<> dis(0.0, 0.3);
			const double breakProb = 0.7;
			for (const int dy : {1, 2, 4, 8, 16, 32, 64}) {
				int ny = b + dy;
				if (ny >= height) break;
				int cnt = 0;
				for (int x : step(width)) {
					if (getGrid(x, ny) == getGoal(a, b)) {
						cnt++;
						result.emplace_back(x, ny);

						if (dis(gen) * cnt > breakProb) {
							break;
						}
					}
				}
			}

			return result;
		}

		bool isGoal()const {
			// Console << getCorrectCountAll();
			for (int i = 0; i < grid.size(); i++) {
				if (grid[i] != goal[i])return false;
			}
			return true;
		}
	};

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


	Solution  simulatedAnnealing(const Board& initialBoard, const Array<Pattern>& patterns) {
		Solution solution;
		const int width = initialBoard.width, height = initialBoard.height;
		OptimizedBoard board(width, height, initialBoard.grid, initialBoard.goal);

		struct State {
			OptimizedBoard board;
			Solution solution;
			double score;
			int32 rowProgress;
			int32 colProgress;
			State(OptimizedBoard b, Solution so) :
				board(b), solution(so), score(0), rowProgress(0), colProgress(0) {}
			State(OptimizedBoard b, Solution so, double sc, int32 rp, int32 cp) :
				board(b), solution(so), score(sc), rowProgress(rp), colProgress(cp) {}

			int32 patternIndex(int siz) {
				return (siz == 0) ? 0 : 3 * (siz - 1) + 1;
			}

			void calculateRowProgress() {
				rowProgress = board.getCorrectCount();
			}

			void calculateColProgress() {
				for (int32 x = colProgress / board.height; x < board.width; x++) {
					for (int32 y = colProgress % board.height; y < board.height; y++) {
						if (board.getGrid(x, y) != board.getGoal(x, y)) {
							colProgress = x * board.height + y;
							return;
						}
					}
				}
			}

			Array<Solution> nextStateByRow(const Array<Pattern>& patterns) {
				calculateRowProgress();
				Array<Solution> solutions;
				int sy = rowProgress / board.width, sx = rowProgress % board.width;
				const int target = board.getGoal(sx, sy);
				// for (int ny = sy; ny < board.height; ny++) {
				for (const int& dy : { 0,1,2,4,8,16,32,64,128 }) {
					int ny = dy + sy;
					// for (int nx = sx; nx < board.width; nx++) {
					for (const int& dx : { 0,1,2,4,8,16,32,64,128 }) {
						int nx = dx + sx;
						if (target != board.getGrid(nx, ny))continue;
						Solution solution;
						for (int bit : step(8)) {
							if (((ny - sy) >> bit) & 1) {
								solution.steps.emplace_back(patterns[patternIndex(bit)],
									Point(nx, sy), 0);
							}
						}
						for (int bit : step(8)) {
							if (((nx - sx) >> bit) & 1) {
								solution.steps.emplace_back(patterns[patternIndex(bit)],
									Point(sx, sy), 2);
							}
						}
						solutions.emplace_back(solution);
					}
				}
				if (solutions.empty()) return nextStateByCol(patterns);
				return solutions;
			}

			Array<Solution> nextStateByCol(const Array<Pattern>& patterns) {
				Array<Solution> solutions;
				calculateColProgress();
				int sy = colProgress % board.height, sx = colProgress / board.height;
				Console << Point(sx, sy);
				int target = board.getGoal(sx, sy);
				// for (int nx = sx; nx < board.width; nx++) {
				for (const int& dx : { 0,1,2,4,8,16,32,64,128 }) {
					int nx = dx + sx;
					// for (int ny = sy; ny < board.height; ny++) {
					for (const int& dy : { 0,1,2,4,8,16,32,64,128 }) {
						int ny = dy + sy;
						if (target != board.getGrid(nx, ny))continue;
						Solution solution;
						for (int bit : step(8)) {
							if (((nx - sx) >> bit) & 1) {
								solution.steps.emplace_back(patterns[patternIndex(bit)],
									Point(sx, ny), 2);
							}
						}
						for (int bit : step(8)) {
							if (((ny - sy) >> bit) & 1) {
								solution.steps.emplace_back(patterns[patternIndex(bit)],
									Point(sx, sy), 0);
							}
						}
						solutions.emplace_back(solution);
					}
				}

				return solutions;
			}

			Array<Solution> nextState(const Array<Pattern>& patterns) {
				Array<Solution> solutions;
				calculateRowProgress();
				int sy = rowProgress / board.width, sx = rowProgress % board.width;
				int target = board.getGoal(sx, sy);
				for (const int& dx : { 0,1,4,8,16,32,64,128 }) {
					int nx = dx + sx;
					if (target != board.getGrid(nx, sy)) continue;
					if (nx >= board.width) break;
					Solution solution;
					for (int bit : step(8)) {
						if ((dx >> bit) & 1) {
							solution.steps.emplace_back(patterns[patternIndex(bit)], Point(sx, sy), 2);
							Point(sx, sy);
						}
					}
					solutions.emplace_back(solution);
				}
				for (const int& dy : { 0,1,2,4,8,16,32, 64, 128 }) {
					int ny = dy + sy;
					if (target != board.getGrid(sx, ny)) continue;
					if (ny >= board.height) break;
					Solution solution;
					for (int bit : step(8)) {
						if ((dy >> bit) & 1) {
							solution.steps.emplace_back(patterns[patternIndex(bit)], Point(sx, sy), 0);
							Point(sx, sy);
						}
					}
					solutions.emplace_back(solution);
				}
				if (solutions.empty())
					for (const int& dy : { 0,1,2,4,8,16,32,64,128 }) {
						int ny = dy + sy;
						// for (int nx = sx; nx < board.width; nx++) {
						for (const int& dx : { 0,1,2,4,8,16,32,64,128 }) {
							int nx = dx + sx;
							if (target != board.getGrid(nx, ny))continue;
							Solution solution;
							for (int bit : step(8)) {
								if (((ny - sy) >> bit) & 1) {
									solution.steps.emplace_back(patterns[patternIndex(bit)],
										Point(nx, sy), 0);
								}
							}
							for (int bit : step(8)) {
								if (((nx - sx) >> bit) & 1) {
									solution.steps.emplace_back(patterns[patternIndex(bit)],
										Point(sx, sy), 2);
								}
							}
							solutions.emplace_back(solution);
						}
					}
				calculateColProgress();
				sy = colProgress % board.height, sx = colProgress / board.height;
				Console << Point(sx, sy);
				target = board.getGoal(sx, sy);
				for (const int& dx : { 0,1,4,8,16,32,64,128 }) {
					int nx = dx + sx;
					if (target != board.getGrid(nx, sy)) continue;
					if (nx >= board.width) break;
					Solution solution;
					for (int bit : step(8)) {
						if ((dx >> bit) & 1) {
							solution.steps.emplace_back(patterns[patternIndex(bit)], Point(sx, sy), 2);
						}
					}
					solutions.emplace_back(solution);
				}
				for (const int& dy : { 0,1,2,4,8,16,32, 64, 128 }) {
					int ny = dy + sy;
					if (target != board.getGrid(sx, ny)) continue;
					if (ny >= board.height) break;
					Solution solution;
					for (int bit : step(8)) {
						if ((dy >> bit) & 1) {
							solution.steps.emplace_back(patterns[patternIndex(bit)], Point(sx, sy), 0);
						}
					}
					solutions.emplace_back(solution);
				}
				if(solutions.empty())
				for (const int& dx : { 0,1,2,4,8,16,32,64,128 }) {
					int nx = dx + sx;
					for (const int& dy : { 0,1,2,4,8,16,32,64,128 }) {
						int ny = dy + sy;
						if (target != board.getGrid(nx, ny))continue;
						Solution solution;
						for (int bit : step(8)) {
							if (((nx - sx) >> bit) & 1) {
								solution.steps.emplace_back(patterns[patternIndex(bit)],
									Point(sx, ny), 2);
							}
						}
						for (int bit : step(8)) {
							if (((ny - sy) >> bit) & 1) {
								solution.steps.emplace_back(patterns[patternIndex(bit)],
									Point(sx, sy), 0);
							}
						}
						solutions.emplace_back(solution);
					}
				}
				return solutions;
			}

			void scoring() {
				double ans = pow(rowProgress + colProgress, 2) / solution.steps.size();
			}
		};

		auto compareStates = [](const State& a, const State& b) {
			return a.score > b.score; // スコアが高い方が優先
			};

		std::priority_queue<State, std::vector<State>, decltype(compareStates)> beam(compareStates);
		beam.emplace(board, Solution());

		State bestState = beam.top();

		auto startTime = std::chrono::high_resolution_clock::now();

		int beamWidth = 10;
		int maxSteps = Min(width * height, 1000);
		for (int32 t = 0; t < maxSteps; t++) {
			Console << U"{}tern"_fmt(t);
			std::priority_queue<State, std::vector<State>, decltype(compareStates)> nextBeam(compareStates);
			for (int32 w = 0; w < beamWidth && !beam.empty(); w++) {
				State currentState = beam.top();
				beam.pop();
				/*const auto& legalActions = (currentState.rowProgress < currentState.colProgress) ?
					currentState.nextStateByRow(patterns) : currentState.nextStateByCol(patterns);*/
				const auto& legalActions = currentState.nextState(patterns);
				for (const auto& newSolution : legalActions) {
					OptimizedBoard nextBoard = currentState.board;
					int32 stepSize = newSolution.steps.size();
					if (stepSize == 0) continue;
					Solution nextSolution = currentState.solution;
					for (const auto& action : newSolution.steps) {
						const auto& [pattern, point, direction] = action;
						nextBoard.apply_pattern(pattern, point, direction);
						nextSolution.steps.emplace_back(action);
					}

					State newState(nextBoard, nextSolution);
					newState.calculateColProgress();
					newState.calculateRowProgress();
					newState.scoring();
					double delta = (currentState.rowProgress + currentState.colProgress - newState.rowProgress - newState.colProgress) / stepSize;
					newState.score *= delta;
					nextBeam.emplace(newState);
				}
			}

			beam = nextBeam;
			if (!beam.empty()) {
				bestState = beam.top();
			}

			if (bestState.board.isGoal()) {
				auto currentTime = std::chrono::high_resolution_clock::now();
				double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();
				Console << elapsedTime << U"sec";
				break;
			}
		}
		return bestState.solution;
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


	Array<Solution> optimizedNextState(const OptimizedBoard& initialBoard, const Array<Pattern>& patterns) {
		Array<Solution> solutions;
		int32 width = initialBoard.width, height = initialBoard.height;

		static const int bitDy[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
		static const int bitDx[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
		int correctCount = initialBoard.getCorrectCount();
		int y = correctCount / width, x = correctCount % width;
		// Console << U"start : " << Point(x, y);
		const auto& candidates = initialBoard.findPointsWithSameValueAndYPopcountDiff1(x, y);
		// Console << candidates;
		for (const auto& candidate : candidates) {
			// Console << U"x,y:" << Point(x, y) << U" cand:" << candidate;
			Solution solution;
			int  dy = candidate.second - y, dx = candidate.first - x;
			int bit = static_cast<int>(std::log2(dy));

			if (dx > 0) {
				solution.steps.emplace_back(patterns[22], Point(candidate.first - x - 256, y + 1), 2);
			}
			else if (dx < 0) {
				solution.steps.emplace_back(patterns[22], Point(candidate.first + (width - x), y + 1), 3);
			}
			const Pattern& pattern = (bit == 0) ? patterns[0] : patterns[3 * (bit - 1) + 1];
			solution.steps.emplace_back(pattern, Point(x, y), 0);
			solutions.emplace_back(solution);
		}
		if (!solutions.empty()) return solutions;
		//if (!solutions.empty()) return solutions;
		//// 1手で到達可能
		//for (const auto& dy : bitDy) {
		//	Solution solution;
		//	int ny = y + dy;
		//	if (ny >= height) break;
		//	if (initialBoard.getGrid(x, ny) != initialBoard.getGoal(x, y)) continue;
		//	int bit = static_cast<int>(std::log2(dy));
		//	const Pattern& pattern = (bit == 0) ? patterns[0] : patterns[3 * (bit - 1) + 1];
		//	int dir = (dy < 0) ? 1 : 0; // 0 for up, 1 for down
		//	solution.steps.emplace_back(pattern, Point(x, y), dir);
		//	if (!solution.steps.empty()) solutions.push_back(solution);
		//}
		//if (!solutions.empty()) return solutions;





		//for (const auto& dx : bitDx) {
		//	Solution solution;
		//	int nx = x + dx;
		//	if (nx >= width) break;
		//	if (initialBoard.getGrid(nx, y) != initialBoard.getGoal(x, y)) continue;
		//	int bit = static_cast<int>(std::log2(dx));
		//	const Pattern& pattern = (bit == 0) ? patterns[0] : patterns[3 * (bit - 1) + 1];
		//	solution.steps.emplace_back(pattern, Point(x, y), 2); // 2 for left
		//	if (!solution.steps.empty()) solutions.push_back(solution);
		//}
		//if (!solutions.empty()) return solutions;

		std::vector<int>targets;
		int target = initialBoard.getGoal(x, y);

		for (int32 nx = x + 1; nx < width; nx++) {
			if (initialBoard.getGrid(nx, y) == target) {
				targets.emplace_back(nx);
			}
		}

		for (const auto& gx : targets) {
			Solution solution;
			for (int bit : step(8)) {
				if (((gx - x) >> bit) & 1) {
					const auto& pattern = (bit == 0) ? patterns[0] : patterns[3 * (bit - 1) + 1];
					solution.steps.emplace_back(pattern, Point(x, y), 2);
				}
			}
			Console << U"size:" << solutions.size();
			solutions.emplace_back(solution);
		}
		return solutions;
	}

	Solution chokudaiSearch(const Board& initialBoard, const Array<Pattern>& patterns) {
		const int32 height = initialBoard.height;
		const int32 width = initialBoard.width;
		const int beamDepth = 1;
		struct State {
			OptimizedBoard board;
			Solution solution;
			double score;
			int32 progress;
			State(const OptimizedBoard& b, const Solution& s, double sc, int32 prog) : board(b), solution(s), score(sc), progress(prog) {}
		};
		auto progress = [](const Board& a) {

			int32 res = 0;
			for (int32 y : step(a.height)) {
				for (int32 x : step(a.width)) {
					if (a.grid[y][x] != a.goal[y][x]) {
						return res;
					}
					res++;
				}
			}
			};
		//const auto& solution = beamSearch(initialBoard, patterns, 10, 1024);
		OptimizedBoard board(width, height, initialBoard.grid, initialBoard.goal);

		auto calculateScore = [](const OptimizedBoard& board, int stepCount) {
			int correctCount = board.getCorrectCount();
			return static_cast<double>(correctCount) / (stepCount + 1);
			};


		auto compareStates = [](const State& a, const State& b) {
			// そろっている部分が多いときに使うと手数が削減される場合がある
			/*if (a.score == b.score) {
				return a.board.calculateDifference(a.board.grid) > b.board.calculateDifference(b.board.grid);
			}*/
			return a.score < b.score; // スコアが高い方が優先
			// return a.score > b.score; // スコアが低い方が優先
			};


		auto beamSearchLambda = [&calculateScore, &compareStates, &patterns](std::priority_queue<State, std::vector<State>, decltype(compareStates)>& beam,
			OptimizedBoard& board) -> Solution {
				int beamWidth = 3;
				auto startTime = std::chrono::high_resolution_clock::now();
				State bestState = beam.top();
				for (int32 t : step(16)) {
					Console << U"t:{}"_fmt(t);
					std::priority_queue<State, std::vector<State>, decltype(compareStates)> nextBeam;
					for (int32 w = 0; w < beamWidth; w++) {
						if (beam.empty())break;
						State currentState = beam.top();
						Console << U"progress:" << currentState.progress << U"/{}steps"_fmt(currentState.solution.steps.size());
						beam.pop();
						// if (currentState.board.isGoal()) continue;
						const auto& legalActions = optimizedNextState(currentState.board, patterns);
						for (const auto& solutions : legalActions) {
							OptimizedBoard nextBoard = currentState.board;
							int32 stepSize = solutions.steps.size();
							if (stepSize == 0) {

							}
							Solution nextSolution;
							for (const auto& action : solutions.steps) {
								const auto& [pattern, point, direction] = action;
								// Console << U"point{}, dir{}"_fmt(point, direction);
								nextBoard.apply_pattern(pattern, point, direction);
								nextSolution.steps.emplace_back(action);
								currentState.solution.steps.emplace_back(action);
							}
							int32 prog = nextBoard.getCorrectCount();
							double delta = prog - currentState.progress;
							// double acc = 1 - double(nextBoard.calculateDifference(nextBoard.grid) / (height * width));
							double newScore = delta / stepSize * pow(prog, 3) / currentState.solution.steps.size() * board.getCorrectCountAll();
							// double newScore = delta / stepSize * acc;
							// Console << U"score:" << newScore;
							State newState = State(nextBoard, currentState.solution, newScore, prog);

							if (t == 0) {
								newState.solution = nextSolution;
							}
							for (int32 _ : step(stepSize)) currentState.solution.steps.pop_back();
							nextBeam.emplace(newState);
							/*if (nextBoard.isGoal()) {
								bestState = currentState;
								break;
							}*/
						}
					}

					beam = nextBeam;
					if (beam.empty()) return bestState.solution;
					bestState = beam.top();
					int left = board.width * board.height - bestState.board.getCorrectCount();
					/*if (left < 500) {
						beamWidth = Max(20, beamWidth + 1);
					}*/
					if (bestState.board.isGoal()) {
						auto currentTime = std::chrono::high_resolution_clock::now();
						double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();
						Console << elapsedTime << U"sec";
						break;
					}

				}

				return bestState.solution;
			};

		auto startTime = std::chrono::high_resolution_clock::now();
		Solution solution;
		while (!board.isGoal()) {
			// if (beam.empty()) break;
			std::priority_queue<State, std::vector<State>, decltype(compareStates)> beam(compareStates);
			// beam.emplace(initialBoard, Solution(), initialBoard.calculateDifference(initialBoard.goal));
			beam.emplace(State(board, Solution(), 0, board.getCorrectCount()));
			State bestState = beam.top();
			const auto& legalActions = beamSearchLambda(beam, board);
			if (legalActions.steps.empty()) break;
			for (const auto& action : legalActions.steps) {
				const auto& [pattern, point, direction] = action;
				board.apply_pattern(pattern, point, direction);
				solution.steps.emplace_back(action);
			}
			if (board.isGoal()) break;
		}
		auto currentTime = std::chrono::high_resolution_clock::now();
		double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();
		Console << elapsedTime << U"sec";
		return solution;
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
			if (a.score == b.score) {
				return a.board.calculateDifference(a.board.grid) > b.board.calculateDifference(b.board.grid);
			}
			return a.score < b.score; // スコアが高い方が優先
			// return a.score > b.score; // スコアが低い方が優先
			};

		std::priority_queue<State, std::vector<State>, decltype(compareStates)> beam(compareStates);
		// beam.emplace(initialBoard, Solution(), initialBoard.calculateDifference(initialBoard.goal));
		beam.emplace(initialBoard, Solution(), progress(initialBoard) * 1 - double(initialBoard.calculateDifference(initialBoard.grid) / (height * width)),
			progress(initialBoard));

		State bestState = beam.top();

		auto startTime = std::chrono::high_resolution_clock::now();


		for (int32 t : step(height)) {
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
						nextSolution.steps.emplace_back(action);
						currentState.solution.steps.emplace_back(action);
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
			return beamSearch(initialBoard, patterns, 50, 100);

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
