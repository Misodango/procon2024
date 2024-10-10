// Algorithm.cpp
#include "Algorithm.h"
#include <omp.h>

namespace Algorithm {

	class OptimizedBoard {
	private:
		std::vector<uint64_t> grid;
		std::vector<uint64_t> goal;
		std::vector<uint64_t> temp_grid;
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

		int estimateStepEffectFast(const Pattern& pattern, Point pos, int direction, int& lastCorrectX, int& lastCorrectY) const {
			int effect = 0;

			// パターンの適用位置が最後の正解位置より前なら、効果はほぼないと考える
			if (pos.y < lastCorrectY || (pos.y == lastCorrectY && pos.x < lastCorrectX)) {
				return 0;
			}

			// パターンのサイズに基づいて、影響を受ける可能性のある正解セルの数を推定
			int affectedCells = pattern.grid.width() * pattern.grid.height();

			switch (direction) {
			case 0: // up
				effect = estimateVerticalShiftEffect(pos, -affectedCells, lastCorrectY);
				break;
			case 1: // down
				effect = estimateVerticalShiftEffect(pos, affectedCells, lastCorrectY);
				break;
			case 2: // left
				effect = estimateHorizontalShiftEffect(pos, -affectedCells, lastCorrectX, lastCorrectY);
				break;
			case 3: // right
				effect = estimateHorizontalShiftEffect(pos, affectedCells, lastCorrectX, lastCorrectY);
				break;
			}

			// 最後の正解位置を更新
			lastCorrectY += effect / width;
			lastCorrectX = (lastCorrectX + effect) % width;

			return effect;
		}

		int estimateVerticalShiftEffect(Point pos, int shift, int lastCorrectY) const {
			int effect = 0;
			int startY = std::max(lastCorrectY, pos.y);
			int endY = std::min(height - 1, pos.y + abs(shift));

			for (int y = startY; y <= endY; ++y) {
				if (getGrid(pos.x, y) == getGoal(pos.x, y + shift)) {
					effect++;
				}
				else if (getGrid(pos.x, y) == getGoal(pos.x, y)) {
					effect--;
				}
			}

			return effect;
		}

		int estimateHorizontalShiftEffect(Point pos, int shift, int lastCorrectX, int lastCorrectY) const {
			int effect = 0;
			int startX = (pos.y == lastCorrectY) ? std::max(lastCorrectX, pos.x) : pos.x;
			int endX = std::min(width - 1, pos.x + abs(shift));

			for (int x = startX; x <= endX; ++x) {
				if (getGrid(x, pos.y) == getGoal(x + shift, pos.y)) {
					effect++;
				}
				else if (getGrid(x, pos.y) == getGoal(x, pos.y)) {
					effect--;
				}
			}

			return effect;
		}

	public:
		int width, height;
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

		int getCorrectCountByRow() const {
			int res = 0;
			for (int x : step(width)) {
				for (int y : step(height)) {
					if (getGrid(x, y) != getGoal(x, y)) return res;
					res++;
				}
			}
			return res;
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
		// (x, y)
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

		// 走査したい行をしていできる
		// popcount(specificY - b) == 1である必要がある
		std::vector<std::pair<int, int>> sortedFindPointsWithSameValueAndYPopcountDiff1(int a, int b, int specificY = -1) const {
			int targetValue = getGoal(a, b);
			std::vector<std::tuple<int, int, float>> result; // (x, y, count)

			auto calculateCount = [&](int sx, int sy, int nx, int ny) {
				int dy = ny - sy;
				for (int i = 0; i < dy; i++) {
					if (getGoal(sx + i, sy) != getGrid((nx + i) % width, ny)) return i;
				}
				int progress = sx + dy + sy * width;
				sx = progress % width; sy = progress / width;
				return dy + getCorrectCountFrom(sx, sy);
				};

			if (specificY == -1) {
				for (const int dy : { 1, 2, 4, 8, 16, 32, 64}) {
					if (dy == 1 && b < height - 2) continue;
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
			else {
				int ny = specificY;
				for (int x : step(width)) {
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
			float maxCount = 0;
			for (const auto& [x, y, count] : result) {
				maxCount = Max(maxCount, count);
				if (count < maxCount) break;
				sortedResult.emplace_back(x, y);
			}

			return sortedResult;
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

		std::vector<std::tuple<int, int, int, int>> findBestMatchingSegmentInPopcountOneRows(int baseRow) const {
			if (baseRow < 0 || baseRow >= height) {
				throw std::out_of_range("Invalid row index");
			}

			std::vector<std::tuple<int, int, int, int>> results; // (rowY, startX, length, score)

			for (int dy : {1, 2, 4, 8, 16, 32, 64, 128}) {
				int targetRow = baseRow + dy;
				if (targetRow >= height) break;

				int bestStartX = 0;
				int bestLength = 0;
				int maxScore = 0;

				for (int startX = 0; startX < width; ++startX) {
					int length = 0;
					int score = 0;
					for (int i = 0; i < width; ++i) {
						int x = (startX + i) % width;
						if (getGrid(x, targetRow) == getGoal(i, baseRow)) {
							length++;
							score += width - i; // Give higher score to earlier matches
						}
						else {
							break;
						}
					}
					if (score > maxScore) {
						maxScore = score;
						bestStartX = startX;
						bestLength = length;
					}
				}

				if (bestLength > 0) {
					results.emplace_back(targetRow, bestStartX, bestLength, maxScore);
				}
			}

			// Sort results by score in descending order
			std::sort(results.begin(), results.end(),
					  [](const auto& a, const auto& b) { return std::get<3>(a) > std::get<3>(b); });

			return results;
		}

		bool isGoal()const {
			// Console << getCorrectCountAll();
			for (int i = 0; i < grid.size(); i++) {
				if (grid[i] != goal[i])return false;
			}
			return true;
		}

	};


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

			if (bestSolution.steps.empty()) {
				int32 target = board.getGoal(sx, sy);
				progress = board.getCorrectCount();
				/*for (int32 ny = sy; ny < board.height; ny++) {
					for (int32 nx = sx; nx < board.width; nx++) {*/
				for (int32 i = progress; i < board.height * board.width; i++) {
					int32 ny = i / board.width, nx = i % board.width;
					if (board.getGrid(nx, ny) != target) continue;
					OptimizedBoard currentBoard = board;
					Solution currentSolution;
					uint32 dy = ny - sy, dx = nx - sx;
					// Console << U"nx, ny:" << Point(nx, ny);
					// １行でも(nx, ny)と(sx, sy)に空き行があれば１手で横移動できる
					if (dy > 1) {
						if (dx > 0) {
							currentBoard.apply_pattern(patterns[22], Point(dx - 256, ny), 2);
							currentSolution.steps.emplace_back(patterns[22], Point(dx - 256, ny), 2);
						}
						else if (dx < 0) {
							currentBoard.apply_pattern(patterns[22], Point(dx + board.width, ny), 3);
							currentSolution.steps.emplace_back(patterns[22], Point(dx + board.width, ny), 3);
						}
					}
					else {

						for (int bit : step(8)) {
							if ((dx >> bit) & 1) {
								const auto& pattern = bit == 0 ? patterns[0] : patterns[3 * (bit - 1) + 1];
								currentBoard.apply_pattern(pattern, Point(sx, sy), 2);
								currentSolution.steps.emplace_back(pattern, Point(sx, sy), 2);
							}
						}
					}

					if (dy > 0)for (int bit : step(8)) {
						if ((dy >> bit) & 1) {
							const auto& pattern = bit == 0 ? patterns[0] : patterns[3 * (bit - 1) + 1];
							currentBoard.apply_pattern(pattern, Point(sx, sy), 0);
							currentSolution.steps.emplace_back(pattern, Point(sx, sy), 0);
						}
					}

					if (currentSolution.steps.size() == 0) {
						// Console << U"step size 0(y popcount not 0) " << Point(nx, ny);
						continue;
					}
					double currentProgressDelta = double(currentBoard.getCorrectCount() - progress) / currentSolution.steps.size();

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

		std::vector<int>targets;
		int target = initialBoard.getGoal(x, y);

		for (int32 nx = x + 1; nx < width; nx++) {
			if (initialBoard.getGrid(nx, y) == target) {
				targets.emplace_back(nx);
			}
		}

		std::sort(targets.begin(), targets.end(), [&](int a, int b) {
			return std::popcount(static_cast<unsigned>(a)) < std::popcount(static_cast<unsigned>(b));
		});

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
			};


		auto beamSearchLambda = [&calculateScore, &compareStates, &patterns](std::priority_queue<State, std::vector<State>, decltype(compareStates)>& beam,
			OptimizedBoard& board) -> Solution {
				int beamWidth = 3;
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
			if (board.isGoal()) break;
		}
		auto currentTime = std::chrono::high_resolution_clock::now();
		double elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();
		Console << elapsedTime << U"sec";
		return solution;
	}


	Solution greedy2(const Board& initialBoard, const Array<Pattern>& patterns) {
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
				if (dx > 0) {
					currentBoard.apply_pattern(patterns[22], Point(dx - 256, ny), 2);
					currentSolution.steps.emplace_back(patterns[22], Point(dx - 256, ny), 2);
				}
				else if (dx < 0) {
					currentBoard.apply_pattern(patterns[22], Point(dx + board.width, ny), 3);
					currentSolution.steps.emplace_back(patterns[22], Point(dx + board.width, ny), 3);
				}

				// 縦移動する前に１つ下を少し揃える
				if (sx < board.height - 1) {
					// additional(x,y)
					const auto& additionalCandidates = currentBoard.sortedFindPointsWithSameValueAndYPopcountDiff1(sx, sy + 1, ny + 1);
					if (!additionalCandidates.empty()) {
						const auto& [ax, ay] = additionalCandidates[0];
						assert(ay == ny + 1);
						const int32 aDx = ax - sx;
						if (aDx > 0) {
							currentBoard.apply_pattern(patterns[22], Point(aDx - 256, ny + 1), 2);
							currentSolution.steps.emplace_back(patterns[22], Point(aDx - 256, ny + 1), 2);
						}
						else if (aDx < 0) {
							currentBoard.apply_pattern(patterns[22], Point(aDx + board.width, ny + 1), 3);
							currentSolution.steps.emplace_back(patterns[22], Point(aDx + board.width, ny + 1), 3);
						}
					}
				}

				if (dy > 0) {
					const int bit = log2(dy);
					const auto& pattern = bit == 0 ? patterns[0] : patterns[3 * (bit - 1) + 1];
					currentBoard.apply_pattern(pattern, Point(sx, sy), 0);
					currentSolution.steps.emplace_back(pattern, Point(sx, sy), 0);
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

			if (bestSolution.steps.empty()) {
				int32 target = board.getGoal(sx, sy);
				progress = board.getCorrectCount();
				/*for (int32 ny = sy; ny < board.height; ny++) {
					for (int32 nx = sx; nx < board.width; nx++) {*/
				for (int32 i = progress; i < board.height * board.width; i++) {
					int32 ny = i / board.width, nx = i % board.width;
					if (board.getGrid(nx, ny) != target) continue;
					OptimizedBoard currentBoard = board;
					Solution currentSolution;
					uint32 dy = ny - sy, dx = nx - sx;
					// Console << U"nx, ny:" << Point(nx, ny);
					// １行でも(nx, ny)と(sx, sy)に空き行があれば１手で横移動できる
					if (dy > 1) {
						if (dx > 0) {
							currentBoard.apply_pattern(patterns[22], Point(dx - 256, ny), 2);
							currentSolution.steps.emplace_back(patterns[22], Point(dx - 256, ny), 2);
						}
						else if (dx < 0) {
							currentBoard.apply_pattern(patterns[22], Point(dx + board.width, ny), 3);
							currentSolution.steps.emplace_back(patterns[22], Point(dx + board.width, ny), 3);
						}
					}
					else {

						for (int bit : step(8)) {
							if ((dx >> bit) & 1) {
								const auto& pattern = bit == 0 ? patterns[0] : patterns[3 * (bit - 1) + 1];
								currentBoard.apply_pattern(pattern, Point(sx, sy), 2);
								currentSolution.steps.emplace_back(pattern, Point(sx, sy), 2);
							}
						}
					}

					if (dy > 0)for (int bit : step(8)) {
						if ((dy >> bit) & 1) {
							const auto& pattern = bit == 0 ? patterns[0] : patterns[3 * (bit - 1) + 1];
							currentBoard.apply_pattern(pattern, Point(sx, sy), 0);
							currentSolution.steps.emplace_back(pattern, Point(sx, sy), 0);
						}
					}

					if (currentSolution.steps.size() == 0) {
						// Console << U"step size 0(y popcount not 0) " << Point(nx, ny);
						continue;
					}
					double currentProgressDelta = double(currentBoard.getCorrectCount() - progress) / currentSolution.steps.size();

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

	Solution solve(Type algorithmType, const Board& initialBoard, const Array<Pattern>& patterns) {
		switch (algorithmType) {
		case Type::Greedy:
			return greedy(initialBoard, patterns);

		case Type::BeamSearch:
			return beamSearch(initialBoard, patterns);

		case Type::Greedy2:
			return greedy2(initialBoard, patterns);
		default:
			throw Error(U"Unknown algorithm type");
		}
	}
}
