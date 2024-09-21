﻿// Algorithm.cpp
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
			// int targetIndex = calculateIndex(a, b);
			// int targetYPopcount = popcount(getYFromIndex(targetIndex));
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

		std::vector<std::pair<int, int>> sortedFindPointsWithSameValueAndYPopcountDiff1(int a, int b) const {
			int targetValue = getGoal(a, b);
			std::vector<std::tuple<int, int, float>> result; // (x, y, count)

			auto calculateCount = [&](int sx, int sy, int nx, int ny) {
				for (int i = sx; i < width; i++) {
					if (getGoal(i, sy) != getGrid((nx + i - sx) % width, ny)) return i - sx;
				}
				return width - sx;
				};

			for (const int dy : {1, 2, 4, 8, 16, 32, 64}) {
				int ny = b + dy;
				if (ny >= height) continue;
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

			// Optionally, limit the number of results
			// if (sortedResult.size() > someLimit) {
			//     sortedResult.resize(someLimit);
			// }

			return sortedResult;
		}


		bool isGoal()const {
			// Console << getCorrectCountAll();
			for (int i = 0; i < grid.size(); i++) {
				if (grid[i] != goal[i])return false;
			}
			return true;
		}

		int evaluateSolutionFast(const Solution& solution) const {
			int currentCorrectCount = getCorrectCount();
			int lastCorrectY = currentCorrectCount / width;
			int lastCorrectX = currentCorrectCount % width;

			for (const auto& [pattern, pos, direction] : solution.steps) {
				currentCorrectCount += estimateStepEffectFast(pattern, pos, direction, lastCorrectX, lastCorrectY);
			}

			return currentCorrectCount;
		}

		int calculateRowMatchScore(int goalY, int gridY, int startX) const {
			int score = 0;
			for (int x = startX; x < width; ++x) {
				if (getGoal(x, goalY) == getGrid(x, gridY)) {
					score++;
				}
				else {
					break;  // Stop at the first mismatch
				}
			}
			return score;
		}

		std::vector<std::pair<int, int>> findPromissingCandidates(int a, int b) const {
			int targetValue = getGoal(a, b);
			std::vector<std::pair<int, int>> candidates;
			std::vector<std::tuple<int, int, double>> scoredCandidates;

			// Search for candidates
			for (int y = b; y < height; ++y) {
				for (int x = 0; x < width; ++x) {
					if (getGrid(x, y) == targetValue) {
						int matchScore = calculateRowMatchScore(b, y, a);
						double score = static_cast<double>(matchScore) / (y - b + 1);  // Normalize by distance
						scoredCandidates.emplace_back(x, y, score);
					}
				}
			}

			// Sort candidates by score in descending order
			std::sort(scoredCandidates.begin(), scoredCandidates.end(),
					  [](const auto& a, const auto& b) { return std::get<2>(a) > std::get<2>(b); });

			// Select top N candidates
			const int N = 10;  // Adjust this value based on your needs
			for (int i = 0; i < std::min(N, static_cast<int>(scoredCandidates.size())); ++i) {
				candidates.emplace_back(std::get<0>(scoredCandidates[i]), std::get<1>(scoredCandidates[i]));
			}

			return candidates;
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
			Console << U"sx , sy:" << Point(sx, sy);
			const auto& candidates = board.sortedFindPointsWithSameValueAndYPopcountDiff1(sx, sy);
			// const auto& candidates = board.findPointsWithSameValueAndYPopcountDiff1(sx, sy);
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
				//int estimatedNewCorrectCount = board.evaluateSolutionFast(currentSolution);
				//double currentProgressDeltaTest = double(estimatedNewCorrectCount - progress) / currentSolution.steps.size();
				double currentProgressDelta = double(currentBoard.getCorrectCount() - progress) / currentSolution.steps.size();
				/*Console << U"currentProgressDelta{}, evaluation{}"_fmt(currentBoard.getCorrectCount(),
					estimatedNewCorrectCount);*/
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
					// Console << U"progress delta:" << currentProgressDelta;
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

		/*Board boardForTimeMeasurement = initialBoard;
		startTime = std::chrono::high_resolution_clock::now();
		for (const auto& [pattern, point, direction] : solution.steps) {
			boardForTimeMeasurement.apply_pattern(pattern, point, direction);
		}
		currentTime = std::chrono::high_resolution_clock::now();
		elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();
		Console << U"normal board:" << elapsedTime << U"sec";

		OptimizedBoard testBoard = board;
		startTime = std::chrono::high_resolution_clock::now();
		for (const auto& [pattern, point, direction] : solution.steps) {
			board.apply_pattern(pattern, point, direction);
		}
		currentTime = std::chrono::high_resolution_clock::now();
		elapsedTime = std::chrono::duration<double>(currentTime - startTime).count();
		Console << U"optimized board:" << elapsedTime << U"sec";*/
		return solution;
	}

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

					if (dx != 0) {
						if (dy > 0) {
							board.apply_pattern(patterns[22], Point(nearestPoint.x, dy - 256), 0);
							solution.steps.emplace_back(patterns[22], Point(nearestPoint.x, dy - 256), 0);
						}
						else if (dy < 0) {
							board.apply_pattern(patterns[22], Point(nearestPoint.x, dy + board.height), 1);
							solution.steps.emplace_back(patterns[22], Point(nearestPoint.x, dy + board.height), 1);
						}
					}
					else {
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
								solution.steps.emplace_back(pattern, Point(x + dx, y), dir);
							}
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
				if (solutions.empty())
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
		int maxSteps = Min(width * height, 600);
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
		const auto& candidates = initialBoard.sortedFindPointsWithSameValueAndYPopcountDiff1(x, y);
		// const auto& candidates = initialBoard.findPointsWithSameValueAndYPopcountDiff1(x, y);
		solutions.reserve(candidates.size());
		auto evaluateSolution = [](Solution sol, OptimizedBoard b) -> double {
			double score = 0;

			};
		// Console << candidates;
		for (const auto& candidate : candidates) {
			// Console << U"x,y:" << Point(x, y) << U" cand:" << candidate;
			Solution solution;
			int  dy = candidate.second - y, dx = candidate.first - x;
			int bit = static_cast<int>(std::log2(dy));
			if (dx == 0 && bit > 0) {
				Solution solutionUsingType2;
				const Pattern& pattern = patterns[3 * (bit - 1)];
				solutionUsingType2.steps.emplace_back(pattern, Point(x, y), 0);
				// solutionUsingType2.score = cnt;
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
			// solution.score = (double)cnt / (1 + dx != 0);
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
				for (int32 t : step(8)) {
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
