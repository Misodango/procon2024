// Algorithm.cpp
#include "Algorithm.h"
#include <omp.h>
#include <execution>
#include <thread>
#include <mutex>
#include <algorithm>
#include <atomic>


namespace Algorithm {

	class OptimizedBoard {
	private:
		// 盤面の1次元表現
		std::vector<uint64_t> grid;
		std::vector<uint64_t> goal;
		std::vector<uint64_t> temp_grid;

		// 64bitに2bitごとに入れるので64/2 = 32bit
		static constexpr int CELLS_PER_UINT64 = 32;

		// 2bitごとに1次元に直すので"11"でandをとる
		static constexpr uint64_t MASK = 0x3; // 11 in binary

		// 座標変換
		int calculateIndex(int x, int y) const {
			return y * width + x;
		}

		// ポップカウント
		int popcount(int n) const {
			// return std::popcount(static_cast<unsigned>(n));
			return std::popcount(static_cast<uint32_t>(n));
			// return __builtin_popcount(n);
		}

		// Y座標変換
		int getYFromIndex(int index) const {
			return index / width;
		}

	public:
		// サイズ
		int width, height;

		// 初期化
		OptimizedBoard(int w, int h) : width(w), height(h) {
			int cellCount = width * height;
			int uint64Count = (cellCount + CELLS_PER_UINT64 - 1) / CELLS_PER_UINT64;
			grid.resize(uint64Count, 0);
			goal.resize(uint64Count, 0);
			temp_grid.resize(uint64Count, 0);
		}

		OptimizedBoard(int w, int h, const Grid<int>& gr, const Grid<int>& go) :width(w), height(h) {
			int cellCount = width * height;
			int uint64Count = (cellCount + CELLS_PER_UINT64 - 1) / CELLS_PER_UINT64;
			grid.resize(uint64Count, 0);
			goal.resize(uint64Count, 0);
			temp_grid.resize(uint64Count, 0);
			setGrid(gr);
			setGoal(go);
		}

		// 1行だけのボード
		OptimizedBoard(int w, const std::vector<uint64_t>& gr, const std::vector<uint64_t>& go) :width(w), height(1) {
			int cellCount = width * height;
			int uint64Count = (cellCount + CELLS_PER_UINT64 - 1) / CELLS_PER_UINT64;
			grid.resize(uint64Count, 0);
			goal.resize(uint64Count, 0);
			temp_grid.resize(uint64Count, 0);
			grid = gr;
			goal = go;
		}

		// 比較関数
		bool operator==(const OptimizedBoard& other) const { return grid == other.grid; };

		// 現在の盤面の個々の値を設定
		void set(int x, int y, int value) {
			int index = y * width + x;
			int arrayIndex = index / CELLS_PER_UINT64;
			int bitIndex = (index % CELLS_PER_UINT64) * 2;

			uint64_t clearMask = ~(MASK << bitIndex);
			grid[arrayIndex] = (grid[arrayIndex] & clearMask) |
				(static_cast<uint64_t>(value) << bitIndex);
		}

		// ゴール盤面の個々の値を設定
		void _set(int x, int y, int value) {
			int index = y * width + x;
			int arrayIndex = index / CELLS_PER_UINT64;
			int bitIndex = (index % CELLS_PER_UINT64) * 2;

			uint64_t clearMask = ~(MASK << bitIndex);
			goal[arrayIndex] = (goal[arrayIndex] & clearMask) |
				(static_cast<uint64_t>(value) << bitIndex);
		}

		// 現在の盤面上の値を取得
		int getGrid(int x, int y) const {
			if (x >= width || y >= height)return -1;
			int index = y * width + x;
			int arrayIndex = index / CELLS_PER_UINT64;
			int bitIndex = (index % CELLS_PER_UINT64) * 2;

			return (grid[arrayIndex] >> bitIndex) & MASK;
		}

		// ゴール盤面上の値を取得
		int getGoal(int x, int y) const {
			if (x >= width || y >= height)return -1;
			int index = y * width + x;
			int arrayIndex = index / CELLS_PER_UINT64;
			int bitIndex = (index % CELLS_PER_UINT64) * 2;

			return (goal[arrayIndex] >> bitIndex) & MASK;
		}

		// グリッドを一度に設定
		void setGrid(const Grid<int>& grid) {
			for (int i : step(grid.height())) {
				for (int j : step(grid.width())) {
					set(j, i, grid[i][j]);
				}
			}
		}

		// ゴールを一度に設定
		void setGoal(const Grid<int>& goal) {
			for (int i : step(goal.height())) {
				for (int j : step(goal.width())) {
					_set(j, i, goal[i][j]);
				}
			}
		}

		// コンソールデバッグ用
		void print() {
			Grid<int> grid(width, height), goal(width, height);
			for (int i = 0; i < height; i++) {
				for (int j = 0; j < width; j++) {
					// std::cout << getGrid(j, i) << " ";
					grid[i][j] = getGrid(j, i);
					goal[i][j] = getGoal(j, i);
				}
			}
			Console << U"grid\n" << grid;
			Console << U"goal\n" << goal;

		}

		// 上向き適用
		void shift_up(const std::vector<bool>& isRemoved) {
			std::fill(temp_grid.begin(), temp_grid.end(), 0);
			for (int x = 0; x < width; ++x) {
				int writeY = 0;
				for (int y = 0; y < height; ++y) {
					if (!isRemoved[y * width + x]) {
						int value = getGrid(x, y);
						int writeIndex = writeY * width + x;
						int writeArrayIndex = writeIndex / CELLS_PER_UINT64;
						int writeBitIndex = (writeIndex % CELLS_PER_UINT64) * 2;
						temp_grid[writeArrayIndex] |= static_cast<uint64_t>(value) << writeBitIndex;
						++writeY;
					}
				}
				for (int y = 0; y < height; ++y) {
					if (isRemoved[y * width + x]) {
						int value = getGrid(x, y);
						int writeIndex = writeY * width + x;
						int writeArrayIndex = writeIndex / CELLS_PER_UINT64;
						int writeBitIndex = (writeIndex % CELLS_PER_UINT64) * 2;
						temp_grid[writeArrayIndex] |= static_cast<uint64_t>(value) << writeBitIndex;
						++writeY;
					}
				}
			}
			std::swap(grid, temp_grid);
		}

		// 下向き適用
		void shift_down(const std::vector<bool>& isRemoved) {
			std::fill(temp_grid.begin(), temp_grid.end(), 0);
			for (int x = 0; x < width; ++x) {
				int writeY = height - 1;
				for (int y = height - 1; y >= 0; --y) {
					if (!isRemoved[y * width + x]) {
						int value = getGrid(x, y);
						int writeIndex = writeY * width + x;
						int writeArrayIndex = writeIndex / CELLS_PER_UINT64;
						int writeBitIndex = (writeIndex % CELLS_PER_UINT64) * 2;
						temp_grid[writeArrayIndex] |= static_cast<uint64_t>(value) << writeBitIndex;
						--writeY;
					}
				}
				for (int y = height - 1; y >= 0; --y) {
					if (isRemoved[y * width + x]) {
						int value = getGrid(x, y);
						int writeIndex = writeY * width + x;
						int writeArrayIndex = writeIndex / CELLS_PER_UINT64;
						int writeBitIndex = (writeIndex % CELLS_PER_UINT64) * 2;
						temp_grid[writeArrayIndex] |= static_cast<uint64_t>(value) << writeBitIndex;
						--writeY;
					}
				}
			}
			std::swap(grid, temp_grid);
		}

		// 左向き適用
		void shift_left(const std::vector<bool>& isRemoved) {
			std::fill(temp_grid.begin(), temp_grid.end(), 0);
			for (int y = 0; y < height; ++y) {
				int writeX = 0;
				for (int x = 0; x < width; ++x) {
					if (!isRemoved[y * width + x]) {
						int value = getGrid(x, y);
						int writeIndex = y * width + writeX;
						int writeArrayIndex = writeIndex / CELLS_PER_UINT64;
						int writeBitIndex = (writeIndex % CELLS_PER_UINT64) * 2;
						temp_grid[writeArrayIndex] |= static_cast<uint64_t>(value) << writeBitIndex;
						++writeX;
					}
				}
				for (int x = 0; x < width; ++x) {
					if (isRemoved[y * width + x]) {
						int value = getGrid(x, y);
						int writeIndex = y * width + writeX;
						int writeArrayIndex = writeIndex / CELLS_PER_UINT64;
						int writeBitIndex = (writeIndex % CELLS_PER_UINT64) * 2;
						temp_grid[writeArrayIndex] |= static_cast<uint64_t>(value) << writeBitIndex;
						++writeX;
					}
				}
			}
			std::swap(grid, temp_grid);
		}

		// 右向き適用
		void shift_right(const std::vector<bool>& isRemoved) {
			std::fill(temp_grid.begin(), temp_grid.end(), 0);
			for (int y = 0; y < height; ++y) {
				int writeX = width - 1;
				for (int x = width - 1; x >= 0; --x) {
					if (!isRemoved[y * width + x]) {
						int value = getGrid(x, y);
						int writeIndex = y * width + writeX;
						int writeArrayIndex = writeIndex / CELLS_PER_UINT64;
						int writeBitIndex = (writeIndex % CELLS_PER_UINT64) * 2;
						temp_grid[writeArrayIndex] |= static_cast<uint64_t>(value) << writeBitIndex;
						--writeX;
					}
				}
				for (int x = width - 1; x >= 0; --x) {
					if (isRemoved[y * width + x]) {
						int value = getGrid(x, y);
						int writeIndex = y * width + writeX;
						int writeArrayIndex = writeIndex / CELLS_PER_UINT64;
						int writeBitIndex = (writeIndex % CELLS_PER_UINT64) * 2;
						temp_grid[writeArrayIndex] |= static_cast<uint64_t>(value) << writeBitIndex;
						--writeX;
					}
				}
			}
			std::swap(grid, temp_grid);
		}

		// 適用
		void apply_pattern(const Pattern& pattern, Point pos, int direction) {
			std::vector<bool> isRemovedVector(width * height, 0);
			for (int y = 0; y < pattern.grid.height(); ++y) {
				for (int x = 0; x < pattern.grid.width(); ++x) {
					if (pattern.grid[y][x] == 1) {
						int bx = pos.x + x, by = pos.y + y;
						if (0 <= bx && bx < width && 0 <= by && by < height) {
							isRemovedVector[by * width + bx] = 1;
						}
					}
				}
			}

			switch (direction) {
			case 0: // up
				shift_up(isRemovedVector);
				break;
			case 1: // down
				shift_down(isRemovedVector);
				break;
			case 2: // left
				shift_left(isRemovedVector);
				break;
			case 3: // right
				shift_right(isRemovedVector);
				break;
			}

		}

		// 盤面すべての揃っている個数のカウント
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

		// 何マスまで揃っているかのカウント
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

		//　任意の点から何マスまで揃っているか
		int getCorrectCountFrom(int startX, int startY) const {
			int count = 0;
			int totalCells = width * height;
			int currentIndex = startY * width + startX;

			while (count < totalCells) {
				int arrayIndex = currentIndex / CELLS_PER_UINT64;
				int bitIndex = (currentIndex % CELLS_PER_UINT64) * 2;

				uint64_t diff = (grid[arrayIndex] ^ goal[arrayIndex]) >> bitIndex;

				while (bitIndex < 64 && count < totalCells) {
					if ((diff & MASK) != 0) {
						return count;
					}
					diff >>= 2;
					bitIndex += 2;
					count++;
					currentIndex++;

					// Wrap around to the beginning if we reach the end
					if (currentIndex >= totalCells) {
						currentIndex = 0;
					}
				}
			}

			return count;
		}

		//　任意の行が何個揃っているか
		int getCorrectCountByRrow(int row)const {
			int res = 0;
			for (int x : step(width)) {
				res += getGoal(x, row) == getGrid(x, row);
			}
			return res;
		}

		// 任意のマス(x, y) = (a, b)と同じ値のマスで最も近い点
		Point findClosestPointWithSameValue(int a, int b) const {
			int targetValue = getGoal(a, b);
			int targetIndex = calculateIndex(a, b);
			std::vector<std::pair<int, int>> candidates;

			// Step 1 & 2: Find all points with the same value
			for (int y = b; y < height; ++y) {
				for (int x = a; x < width; ++x) {
					if (getGrid(x, y) == targetValue) {
						candidates.emplace_back(x, y);
					}
				}
			}

			if (candidates.empty()) {
				return { -1, -1 };  // No valid point found
			}

			// Step 3 & 5: Calculate popcount differences and find the minimum
			int minPopcountDiff = std::numeric_limits<int>::max();
			Point closestPoint = { -1, -1 };
			for (const auto& [x, y] : candidates) {
				int popcountDiff = popcount(y - b);
				if (popcountDiff < minPopcountDiff) {
					minPopcountDiff = popcountDiff;
					closestPoint = { x,y };
				}
			}
			return closestPoint;
		}

		// 任意のマス(x, y) = (a, b)と同じ値のマス
		std::vector<std::pair<int, int>> findPointsWithSameValue(int a, int b) const {
			int target = getGoal(a, b);
			std::vector<std::pair<int, int>> result;
			for (int y = b; y < height; y++) {
				for (int x = a; x < width; x++) {
					if (target == getGrid(x, y)) {
						result.emplace_back(x, y);
					}
				}
			}
			Console << U"res:" << result;
			return result;
		}

		// 任意のマス(x, y) = (a, b)と同じ値のマスをpopcountでソート
		std::vector<std::pair<int, int>> sortedFindPointsWithSameValue(int a, int b) const {
			const int targetValue = getGoal(a, b);
			// <座標, ポップカウント距離>
			// std::vector<std::tuple<Point, int>> candidates;
			std::vector<std::pair<int, int>> result;
			for (int y = b; y < height; y++) {
				for (int x = a; x < width; x++) {
					if (getGrid(x, y) != targetValue) continue;
					// 横向き移動は
					int distance = popcount(abs(x - a)) + y != b;
					// candidates.emplace_back(x, y, distance);
					result.emplace_back(x, y);
				}
			}
			return result;
		}

		// 任意のマス(x, y) = (a, b)と同じ値かつY軸のポップカウントが1の点
		std::vector<std::pair<int, int>> findPointsWithSameValueAndYPopcountDiff1(int a, int b) const {
			int targetValue = getGoal(a, b);
			std::vector<std::pair<int, int>> result;
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_real_distribution<> dis(0.0, 0.3);
			const double breakProb = 0.7;

			for (const int dy : {1, 2, 4, 8, 16, 32, 64}) {
				//for (const int dy : {64, 32, 16, 8, 4, 2, 1}) {
				int ny = b + dy;
				if (ny >= height) continue;
				int cnt = 0;
				for (int x : step(width)) {
					if (getGrid(x, ny) == getGoal(a, b)) {
						if (a == width - 1 || getGrid((x + 1) % width, ny) == getGoal(a + 1, b)) {
							cnt++;
							result.emplace_back(x, ny);
						}
					}
				}
			}

			/*int siz = result.size();
			if (width > 128) result.resize(siz / 3);*/
			return result;
		}

		// 任意のマス(x, y) = (a, b)と同じ値かつY軸のポップカウントが1の点を期待値の高い順にソート
		// 走査したい行を指定できる
		// popcount(specificY - b) == 1である必要がある
		std::vector<std::pair<int, int>> sortedFindPointsWithSameValueAndYPopcountDiff1(int a, int b, int specificY = -1) const {
			int targetValue = getGoal(a, b);
			std::vector<std::tuple<int, int, float>> result; // (x, y, count)

			auto calculateCount = [&](int sx, int sy, int nx, int ny) {
				int dy = ny - sy;
				if (dy == 0) {
					for (int i = nx; i < width; i++) {
						if (getGoal(i, sy) != getGrid(i, ny)) return i;
					}
					return nx - sx;
				}
				for (int i = 0; i < dy; i++) {
					if (getGoal(sx + i, sy) != getGrid((nx + i) % width, ny)) return i;
				}
				int progress = sx + dy + sy * width;
				sx = progress % width; sy = progress / width;
				return dy + getCorrectCountFrom(sx, sy);
				};

			if (specificY == -1) {
				// dy = 0のとき
				/*for (int dx : {1, 2, 4, 8, 16, 32, 64, 128}) {
					if (a + dx >= width) break;
					if (getGrid(a + dx, b) == targetValue) {
						int count = calculateCount(a, b, a + dx, b);
						int stepSize = 1;
						result.emplace_back(a + dx, b, count);
					}
				}*/
				for (const int dy : {1, 2, 4, 8, 16, 32, 64}) {
					// if (dy == 1 && b < height - 2) continue;
					int ny = b + dy;
					if (ny >= height) break;
					for (int x : step(width)) {
						if (getGrid(x, ny) == targetValue) {
							int count = calculateCount(a, b, x, ny);
							int stepSize = x == a ? 1 : 2;
							result.emplace_back(x, ny, count / stepSize);

						}
					}
				}
			}
			else if (specificY == b) {
				// popcount(dx) = 1である必要がある
				for (int dx : {1, 2, 4, 8, 16, 32, 64, 128}) {
					if (a + dx >= width) break;
					if (getGrid(a + dx, b) == targetValue) {
						int count = calculateCount(a, b, a + dx, b);
						int stepSize = 1;
						result.emplace_back(a + dx, b, count);
					}
				}
			}
			else {
				int ny = specificY;
				///*for (int x : step(width)) */{
				for (int dx : {1, 2, 4, 8, 16, 32, 64, 128}) {
					int x = a + dx;
					if (x >= width) break;
					if (getGrid(x, ny) == targetValue) {
						int count = calculateCount(a, b, x, ny);
						int stepSize = x == a ? 1 : 2;
						result.emplace_back(x, ny, count / stepSize);
					}
				}
			}

			// Sort result based on count in descending order
			std::sort(result.begin(), result.end(),
					  [](const auto& a, const auto& b) { return std::get<2>(a) > std::get<2>(b); });

			// Convert sorted result to vector of pairs (x, y)
			std::vector<std::pair<int, int>> sortedResult;
			sortedResult.reserve(result.size());
			float maxCount = -1;
			for (const auto& [x, y, count] : result) {
				maxCount = Max(maxCount, count);
				if (count < maxCount) break;
				sortedResult.emplace_back(x, y);
			}

			// dy = 0のとき
			for (int dx : {1, 2, 4, 8, 16, 32, 64, 128}) {
				if (a + dx >= width) break;
				if (getGrid(a + dx, b) == targetValue) {
					int count = calculateCount(a, b, a + dx, b);
					int stepSize = 1;
					sortedResult.emplace_back(a + dx, b);
				}
			}

			return sortedResult;
		}

		// 任意のマス(x, y) = (a, b)と同じ値のマスで、同じ行にあるものを探す
		// (a, b) より右にあるもの
		std::vector<std::pair<int, int>> findPointsWithSameValueInSameRow(int a, int b) {
			std::vector<std::pair<int, int>> result;
			int targetValue = getGoal(a, b);
			for (int x = a + 1; x < width; x++) {
				if (getGrid(x, b) != targetValue) continue;
				result.emplace_back(x, b);
			}
		}

		// 異なる二つの行でどれだけ連続して揃っているか
		// (sx, sy) ゴール盤面の始点
		// (nx, ny) 現在の盤面の始点
		int calculateSuccessiveArea(int sx, int sy, int nx, int ny) const {
			int res = 0;
			for (int cnt = 0; cnt < width; cnt++) {
				if (getGoal(sx + cnt, sy) != getGrid(nx + cnt, ny)) break;
				if (sx + cnt >= width || nx + cnt >= width) break;
				res++;
			}
			return res;
		}

		// 異なる二つの行でどれだけ揃っているか
		// (sx, sy) ゴール盤面の始点
		// (nx, ny) 現在の盤面の始点
		// nx == sx
		int compareRows(int sx, int sy, int nx, int ny) const {
			int cnt = 0;
			for (int x = sx; x < width; x++) {
				if (getGrid(x, ny) != getGoal(x, sy))continue;
				cnt++;
			}
			return cnt;
		}

		// 特定の行を抜き出す
		OptimizedBoard extractRow(int goalRow, int currentRow) const {
			if (currentRow < 0 || currentRow >= height || goalRow < 0 || goalRow >= height) {
				throw std::out_of_range("Invalid row index");
			}

			OptimizedBoard newBoard(width, 1);
			for (int x = 0; x < width; ++x) {
				newBoard.set(x, 0, getGrid(x, currentRow));
				newBoard._set(x, 0, getGoal(x, goalRow));
			}
			return newBoard;
		}

		// 正解かどうか
		bool isGoal()const {
			// Console << getCorrectCountAll();
			for (int i = 0; i < grid.size(); i++) {
				if (grid[i] != goal[i])return false;
			}
			return true;
		}

	};


	Array<Solution> optimizedNextState(const OptimizedBoard& initialBoard, const Array<Pattern>& patterns) {
		Array<Solution> solutions;
		int32 width = initialBoard.width, height = initialBoard.height;
		static const int bitDy[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
		static const int bitDx[] = { 1, 2, 4, 8, 16, 32, 64, 128 };
		int correctCount = initialBoard.getCorrectCount();
		int y = correctCount / width, x = correctCount % width;
		// Console << U"start : " << Point(x, y);
		const auto& candidates = initialBoard.sortedFindPointsWithSameValueAndYPopcountDiff1(x, y);

		solutions.reserve(candidates.size());
		auto evaluateSolution = [](Solution sol, OptimizedBoard b) -> double {
			double score = 0;

			};
		// Console << candidates;
		for (const auto& candidate : candidates) {
			Solution solution;
			int  dy = candidate.second - y, dx = candidate.first - x;
			int bit = static_cast<int>(std::log2(dy));
			if (dx == 0 && bit > 0) {
				Solution solutionUsingType2;
				const Pattern& pattern = patterns[3 * (bit - 1)];
				solutionUsingType2.steps.emplace_back(pattern, Point(x, y), 0);
				solutions.emplace_back(solutionUsingType2);
			}

			if (dy == 0) {
				const int bit = log2(dx);
				const auto& pattern = bit == 0 ? patterns[0] : patterns[3 * (bit - 1) + 1];
				solution.steps.emplace_back(pattern, Point(x, y), 2);
				continue;
			}
			else {
				if (dx > 0) {
					solution.steps.emplace_back(patterns[22], Point(candidate.first - x - 256, y + 1), 2);
				}
				else if (dx < 0) {
					solution.steps.emplace_back(patterns[22], Point(candidate.first + (width - x), y + 1), 3);
				}
			}
			const Pattern& pattern = (bit == 0) ? patterns[0] : patterns[3 * (bit - 1) + 1];
			solution.steps.emplace_back(pattern, Point(x, y), 0);
			solutions.emplace_back(solution);
		}

		if (!solutions.empty()) return solutions;


		std::vector<std::pair<int, int>> targets;
		int target = initialBoard.getGoal(x, y);
		//　同じ行で探す
		for (int nx = x; nx < width; nx++) {
			if (initialBoard.getGrid(nx, y) == target) {
				targets.emplace_back(nx, y);
			}
		}

		// 別の行( popcountで差が1じゃない行)
		for (int ny = y + 1; ny < height; ny++) {
			for (int nx : step(width)) {
				if (initialBoard.getGrid(nx, ny) != target) continue;
				targets.emplace_back(nx, ny);
			}
		}
		Console << U"targets size:" << targets.size();
		for (const auto& [gx, gy] : targets) {
			int dx = gx - x, dy = gy - y;
			Solution solution;
			if (dy > 0) {
				if (dx > 0) {
					solution.steps.emplace_back(patterns[22], Point(dx - 256, y + 1), 2);
				}
				else if (dx < 0) {
					solution.steps.emplace_back(patterns[22], Point(gx + (width - x), y + 1), 3);
				}
				for (int bit : step(8)) {
					if ((dy >> bit) & 1) {
						const auto& pattern = (bit == 0) ? patterns[0] : patterns[3 * (bit - 1) + 1];
						solution.steps.emplace_back(pattern, Point(x, y), 0);
					}
				}
				solutions.emplace_back(solution);
				continue;
			}
			for (int bit : step(8)) {
				if ((dx >> bit) & 1) {
					const auto& pattern = (bit == 0) ? patterns[0] : patterns[3 * (bit - 1) + 1];
					solution.steps.emplace_back(pattern, Point(x, y), 2);
				}
			}
			solutions.emplace_back(solution);
		}
		Console << U"current size:" << solutions.size();
		return solutions;
	}


	Solution beamSearch(const Board& initialBoard, const Array<Pattern>& patterns) {
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
			};


		auto beamSearchLambda = [&calculateScore, &compareStates, &patterns](std::priority_queue<State, std::vector<State>, decltype(compareStates)>& beam,
			OptimizedBoard& board) -> Solution {
				int beamWidth = 100;
				auto startTime = std::chrono::high_resolution_clock::now();
				State bestState = beam.top();
				for (int32 t : step(8)) {
					Console << U"t:{}"_fmt(t);
					std::priority_queue<State, std::vector<State>, decltype(compareStates)> nextBeam;
					for (int32 w = 0; w < beamWidth; w++) {
						if (beam.empty())break;
						State currentState = beam.top();
						Console << U"progress:" << currentState.progress << U"/{}steps"_fmt(currentState.solution.steps.size());
						beam.pop();
						if (currentState.board.isGoal()) continue;
						// if (currentState.board.isGoal()) continue;
						const auto& legalActions = optimizedNextState(currentState.board, patterns);
						for (const auto& solutions : legalActions) {
							OptimizedBoard nextBoard = currentState.board;
							int32 stepSize = solutions.steps.size();
							if (stepSize == 0) {
								continue;
							}
							Solution nextSolution;
							for (const auto& action : solutions.steps) {
								const auto& [pattern, point, direction] = action;
								nextBoard.apply_pattern(pattern, point, direction);
								nextSolution.steps.emplace_back(action);
								currentState.solution.steps.emplace_back(action);
							}
							int32 prog = nextBoard.getCorrectCount();
							double delta = prog - currentState.progress;
							// Console << U"score:{}, delta:{}"_fmt(solutions.score, delta/stepSize);
							double newScore = delta / stepSize * prog / currentState.solution.steps.size() * board.getCorrectCountAll();
							State newState = State(nextBoard, currentState.solution, newScore, prog);

							if (t == 0) {
								newState.solution = nextSolution;
							}
							for (int32 _ : step(stepSize)) currentState.solution.steps.pop_back();
							nextBeam.emplace(newState);
						}
					}

					beam = nextBeam;
					if (beam.empty()) return bestState.solution;
					bestState = beam.top();
					int left = board.width * board.height - bestState.board.getCorrectCount();
					if (left < 100) {
						beamWidth = Max(30, beamWidth + 1);
					}
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
			std::priority_queue<State, std::vector<State>, decltype(compareStates)> beam(compareStates);
			beam.emplace(State(board, Solution(), 0, board.getCorrectCount()));
			State bestState = beam.top();
			const auto& legalActions = beamSearchLambda(beam, board);
			if (legalActions.steps.empty()) break;
			for (const auto& action : legalActions.steps) {
				const auto& [pattern, point, direction] = action;
				board.apply_pattern(pattern, point, direction);
				solution.steps.emplace_back(action);
			}
			Console << U"progress:" << board.getCorrectCount();
			if (board.isGoal()) break;
		}
		auto currentTime = std::chrono::high_resolution_clock::now();
		double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();
		Console << elapsedTime << U"sec";
		return solution;
	}

	Solution greedy(const Board& initialBoard, const Array<Pattern>& patterns) {
		OptimizedBoard board(initialBoard.width, initialBoard.height, initialBoard.grid, initialBoard.goal);
		// Z字に進行(横書き文章の順)
		// 3HWで解く
		// 1番右の列を移動につかうことで3HWで解ける?
		auto startTime = std::chrono::high_resolution_clock::now();
		Solution solution;
		while (!board.isGoal()) {

			int32 progress = board.getCorrectCount();
			int32 sy = progress / board.width, sx = progress % board.width;
			Console << U"sy:" << sy;
			const auto& candidates = board.sortedFindPointsWithSameValueAndYPopcountDiff1(sx, sy);

			Solution bestSolution;
			double bestProgressDelta = 0;

			for (const auto& [nx, ny] : candidates) {
				OptimizedBoard currentBoard = board;
				// Console << U"nx, ny : " << Point(nx, ny);
				Solution currentSolution;
				const int32 dy = ny - sy, dx = nx - sx;

				if (dy == 0) {
					const int bit = log2(dx);
					const auto& pattern = bit == 0 ? patterns[0] : patterns[3 * (bit - 1) + 1];
					currentBoard.apply_pattern(pattern, Point(sx, sy), 2);
					currentSolution.steps.emplace_back(pattern, Point(sx, sy), 2);
				}
				else {
					if (dx > 0) {
						currentBoard.apply_pattern(patterns[22], Point(dx - 256, ny), 2);
						currentSolution.steps.emplace_back(patterns[22], Point(dx - 256, ny), 2);
					}
					else if (dx < 0) {
						currentBoard.apply_pattern(patterns[22], Point(dx + board.width, ny), 3);
						currentSolution.steps.emplace_back(patterns[22], Point(dx + board.width, ny), 3);
					}


					if (dy > 0) {
						const int bit = log2(dy);
						const auto& pattern = bit == 0 ? patterns[0] : patterns[3 * (bit - 1) + 1];
						currentBoard.apply_pattern(pattern, Point(sx, sy), 0);
						currentSolution.steps.emplace_back(pattern, Point(sx, sy), 0);
					}
				}
				if (currentSolution.steps.size() == 0) {
					// Console << U"step size 0 " << Point(nx, ny);
					continue;
				}
				double currentProgressDelta = double(currentBoard.getCorrectCount() - progress) / currentSolution.steps.size();
				if (currentProgressDelta > bestProgressDelta) {
					bestProgressDelta = currentProgressDelta;
					bestSolution = currentSolution;
				}
			}

			// 見つからなかったとき
			if (bestSolution.steps.empty()) {
				std::vector<std::pair<int, int>> targets;
				int target = board.getGoal(sx, sy);
				//　同じ行で探す
				for (int nx = sx; nx < board.width; nx++) {
					if (board.getGrid(nx, sy) == target) {
						targets.emplace_back(nx, sy);
					}
				}

				// 別の行( popcountで差が1じゃない行)
				for (int ny = sy + 1; ny < board.height; ny++) {
					for (int nx : step(board.width)) {
						if (board.getGrid(nx, ny) != target) continue;
						targets.emplace_back(nx, ny);
					}
				}

				for (const auto& [gx, gy] : targets) {
					int dx = gx - sx, dy = gy - sy;
					Solution currentSolution;
					OptimizedBoard currentBoard = board;
					if (dy > 0) {
						if (dx > 0) {
							currentSolution.steps.emplace_back(patterns[22], Point(dx - 256, sy + 1), 2);
							currentBoard.apply_pattern(patterns[22], Point(dx - 256, sy + 1), 2);
						}
						else if (dx < 0) {
							currentSolution.steps.emplace_back(patterns[22], Point(gx + (board.width - sx), sy + 1), 3);
							currentBoard.apply_pattern(patterns[22], Point(gx + (board.width - sx), sy + 1), 3);
						}
						for (int bit : step(8)) {
							if ((dy >> bit) & 1) {
								const auto& pattern = (bit == 0) ? patterns[0] : patterns[3 * (bit - 1) + 1];
								currentSolution.steps.emplace_back(pattern, Point(sx, sy), 0);
								currentBoard.apply_pattern(pattern, Point(sx, sy), 0);
							}
						}
					}
					else {
						for (int bit : step(8)) {
							if ((dx >> bit) & 1) {
								const auto& pattern = (bit == 0) ? patterns[0] : patterns[3 * (bit - 1) + 1];
								currentSolution.steps.emplace_back(pattern, Point(sx, sy), 2);
								currentBoard.apply_pattern(pattern, Point(sx, sy), 2);
							}
						}
					}

					double currentProgressDelta = double(currentBoard.getCorrectCount() - progress) / currentSolution.steps.size();
					// Console << U"best:{}, current:{}"_fmt(bestProgressDelta, currentProgressDelta);
					if (currentProgressDelta > bestProgressDelta) {
						bestProgressDelta = currentProgressDelta;
						bestSolution = currentSolution;
					}
				}
			}
			for (const auto& action : bestSolution.steps) {
				const auto& [pattern, point, direction] = action;
				board.apply_pattern(pattern, point, direction);
				solution.steps.emplace_back(action);
				// Console << U"pattern:{}, Point:{}, direction:{}"_fmt(pattern.p, point, direction);
			}

		}

		auto currentTime = std::chrono::high_resolution_clock::now();
		double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();
		Console << elapsedTime << U"sec";

		return solution;
	}


	Solution optimizedGreedy(const OptimizedBoard& initialBoard, const Array<Pattern>& patterns) {
		OptimizedBoard board = initialBoard;
		// Z字に進行(横書き文章の順)
		// 3HWで解く
		// 1番右の列を移動につかうことで3HWで解ける?
		auto startTime = std::chrono::high_resolution_clock::now();
		Solution solution;
		while (!board.isGoal()) {

			int32 progress = board.getCorrectCount();
			int32 sy = progress / board.width, sx = progress % board.width;

			const auto& candidates = board.sortedFindPointsWithSameValueAndYPopcountDiff1(sx, sy);

			Solution bestSolution;
			double bestProgressDelta = 0;

			for (const auto& [nx, ny] : candidates) {
				OptimizedBoard currentBoard = board;
				// Console << U"nx, ny : " << Point(nx, ny);
				Solution currentSolution;
				const int32 dy = ny - sy, dx = nx - sx;

				if (dy == 0) {
					const int bit = log2(dx);
					const auto& pattern = bit == 0 ? patterns[0] : patterns[3 * (bit - 1) + 1];
					currentBoard.apply_pattern(pattern, Point(sx, sy), 2);
					currentSolution.steps.emplace_back(pattern, Point(sx, sy), 2);
				}
				else {
					if (dx > 0) {
						currentBoard.apply_pattern(patterns[22], Point(dx - 256, ny), 2);
						currentSolution.steps.emplace_back(patterns[22], Point(dx - 256, ny), 2);
					}
					else if (dx < 0) {
						currentBoard.apply_pattern(patterns[22], Point(dx + board.width, ny), 3);
						currentSolution.steps.emplace_back(patterns[22], Point(dx + board.width, ny), 3);
					}


					if (dy > 0) {
						const int bit = log2(dy);
						const auto& pattern = bit == 0 ? patterns[0] : patterns[3 * (bit - 1) + 1];
						currentBoard.apply_pattern(pattern, Point(sx, sy), 0);
						currentSolution.steps.emplace_back(pattern, Point(sx, sy), 0);
					}
				}
				if (currentSolution.steps.size() == 0) {
					// Console << U"step size 0 " << Point(nx, ny);
					continue;
				}
				double currentProgressDelta = double(currentBoard.getCorrectCount() - progress) / currentSolution.steps.size();
				if (currentProgressDelta > bestProgressDelta) {
					bestProgressDelta = currentProgressDelta;
					bestSolution = currentSolution;
				}
			}

			// 見つからなかったとき
			if (bestSolution.steps.empty()) {
				std::vector<std::pair<int, int>> targets;
				int target = board.getGoal(sx, sy);
				//　同じ行で探す
				for (int nx = sx; nx < board.width; nx++) {
					if (board.getGrid(nx, sy) == target) {
						targets.emplace_back(nx, sy);
					}
				}

				// 別の行( popcountで差が1じゃない行)
				for (int ny = sy + 1; ny < board.height; ny++) {
					for (int nx : step(board.width)) {
						if (board.getGrid(nx, ny) != target) continue;
						targets.emplace_back(nx, ny);
					}
				}

				for (const auto& [gx, gy] : targets) {
					int dx = gx - sx, dy = gy - sy;
					Solution currentSolution;
					OptimizedBoard currentBoard = board;
					if (dy > 0) {
						if (dx > 0) {
							currentSolution.steps.emplace_back(patterns[22], Point(dx - 256, sy + 1), 2);
							currentBoard.apply_pattern(patterns[22], Point(dx - 256, sy + 1), 2);
						}
						else if (dx < 0) {
							currentSolution.steps.emplace_back(patterns[22], Point(gx + (board.width - sx), sy + 1), 3);
							currentBoard.apply_pattern(patterns[22], Point(gx + (board.width - sx), sy + 1), 3);
						}
						for (int bit : step(8)) {
							if ((dy >> bit) & 1) {
								const auto& pattern = (bit == 0) ? patterns[0] : patterns[3 * (bit - 1) + 1];
								currentSolution.steps.emplace_back(pattern, Point(sx, sy), 0);
								currentBoard.apply_pattern(pattern, Point(sx, sy), 0);
							}
						}
					}
					else {
						for (int bit : step(8)) {
							if ((dx >> bit) & 1) {
								const auto& pattern = (bit == 0) ? patterns[0] : patterns[3 * (bit - 1) + 1];
								currentSolution.steps.emplace_back(pattern, Point(sx, sy), 2);
								currentBoard.apply_pattern(pattern, Point(sx, sy), 2);
							}
						}
					}

					double currentProgressDelta = double(currentBoard.getCorrectCount() - progress) / currentSolution.steps.size();
					// Console << U"best:{}, current:{}"_fmt(bestProgressDelta, currentProgressDelta);
					if (currentProgressDelta > bestProgressDelta) {
						bestProgressDelta = currentProgressDelta;
						bestSolution = currentSolution;
					}
				}
			}
			for (const auto& action : bestSolution.steps) {
				const auto& [pattern, point, direction] = action;
				board.apply_pattern(pattern, point, direction);
				solution.steps.emplace_back(action);
				// Console << U"pattern:{}, Point:{}, direction:{}"_fmt(pattern.p, point, direction);
			}

		}

		auto currentTime = std::chrono::high_resolution_clock::now();
		double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();
		Console << elapsedTime << U"sec";

		return solution;
	}

	// 行の入れ替えでも試してみる
	Solution improveGreedy(const Board& initialBoard, const Array<Pattern>& patterns) {
		const double TIME_LIMIT = 200.0;
		auto startTime = std::chrono::high_resolution_clock::now();

		Solution bestSolution = greedy(initialBoard, patterns);
		int bestStepCount = bestSolution.steps.size();

		int64_t totalTrials = 0;  // 試行回数カウンター

		auto currentTime = std::chrono::high_resolution_clock::now();
		double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();

		while (elapsedTime < TIME_LIMIT) {
			totalTrials++;  // 試行回数をインクリメント

			Solution candidateSolution = bestSolution;
			if (candidateSolution.steps.empty()) continue;

			int changePos = rand() % candidateSolution.steps.size();

			OptimizedBoard tempBoard(initialBoard.width, initialBoard.height, initialBoard.grid, initialBoard.goal);
			Solution newSolution;

			for (int i = 0; i < changePos; i++) {
				const auto& [pattern, point, dir] = candidateSolution.steps[i];
				tempBoard.apply_pattern(pattern, point, dir);
				newSolution.steps.emplace_back(pattern, point, dir);
			}

			int patternIndex = 23;
			int currentProgress = tempBoard.getCorrectCount();
			int currentX = currentProgress % tempBoard.width, currentY = currentProgress / tempBoard.width;
			int direction = 0;
			int y = currentY;
			int x = Random<int>(currentX, initialBoard.width - 1);
			if (tempBoard.compareRows(x, y, x, y + 1) < tempBoard.compareRows(x, y, x, y)) {
				continue;
			}

			tempBoard.apply_pattern(patterns[patternIndex], Point(x, y), direction);
			newSolution.steps.emplace_back(patterns[patternIndex], Point(x, y), direction);

			OptimizedBoard remainingBoard = tempBoard;
			Solution remainingSolution = optimizedGreedy(remainingBoard, patterns);

			for (const auto& step : remainingSolution.steps) {
				newSolution.steps.emplace_back(step);
			}

			if (newSolution.steps.size() < bestStepCount) {
				bestSolution = newSolution;
				bestStepCount = newSolution.steps.size();
				Console << U"Improved! Steps: " << bestStepCount << U", Trials: " << totalTrials;
			}

			currentTime = std::chrono::high_resolution_clock::now();
			elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();
		}

		Console << U"Total trials: " << totalTrials;
		Console << U"Final step count: " << bestStepCount;
		Console << U"Time taken: " << elapsedTime << U" seconds";
		Console << U"Trials per second: " << (double)totalTrials / elapsedTime;
		return bestSolution;
	}


	Solution solve(Type algorithmType, const Board& initialBoard, const Array<Pattern>& patterns) {
		switch (algorithmType) {
		case Type::Greedy:
			return greedy(initialBoard, patterns);

		case Type::BeamSearch:
			return beamSearch(initialBoard, patterns);

		case Type::ImprovedGreedy:
			return improveGreedy(initialBoard, patterns);
		default:
			throw Error(U"Unknown algorithm type");
		}
	}
}
