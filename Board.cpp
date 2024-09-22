// Board.cpp

#include "Board.h"


Board::Board(int32 w, int32 h) : width(w), height(h), grid(w, h, 0), goal(w, h, 0) {}


Board Board::fromJSON(const JSON& json) {
	Console << json;
	const int32 width = json[U"width"].get<int32>();
	const int32 height = json[U"height"].get<int32>();
	Board board(width, height);
	// const String start = json[U"start"].getString();
	// const String goal = json[U"goal"].getString();
	const JSONArrayView startArray = json[U"start"].arrayView();
	const JSONArrayView goalArray = json[U"goal"].arrayView();

	// startの処理
	for (int32 y = 0; y < height; ++y) {
		const String& row = startArray[y].getString();
		for (int32 x = 0; x < width; ++x) {
			board.grid[y][x] = row[x] - '0';
		}
	}

	// goal
	for (int32 y = 0; y < height; ++y) {
		const String& row = goalArray[y].getString();
		for (int32 x = 0; x < width; ++x) {
			board.goal[y][x] = row[x] - '0';
		}
	}

	return board;
}

bool Board::is_goal() const {
	return grid == goal;
}

// ゴール状態との差異を計算
int32 Board::calculateDifference(const Grid<int32>& otherGrid) const {
	int32 diff = 0;
	for (int32 y = 0; y < height; ++y) {
		for (int32 x = 0; x < width; ++x) {
			if (goal[y][x] != otherGrid[y][x]) {
				diff++;
			}
		}
	}
	// Console << U"diff: {}"_fmt(diff);
	return diff;
}

void Board::apply_pattern(const Pattern& pattern, Point pos, int32 direction) {
	// Array<Point> removed_cells;
	Grid<bool> isRemoved(width, height, 0);
	for (int32 y = 0; y < pattern.grid.height(); ++y) {
		for (int32 x = 0; x < pattern.grid.width(); ++x) {
			if (pattern.grid[y][x] == 1) {
				int32 bx = pos.x + x, by = pos.y + y;
				if (0 <= bx && bx < width && 0 <= by && by < height) {
					isRemoved[by][bx] = 1;
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

// パターンを適用した結果のボードを返す（実際には適用しない）
Board Board::applyPatternCopy(const Pattern& pattern, Point pos, int32 direction) const {
	Board newBoard = *this;
	newBoard.apply_pattern(pattern, pos, direction);
	return newBoard;
}

void Board::draw() const {

	const int32 cellSize = Min(1024 / grid.width(), 1024 / grid.height());
	static const ColorF gridColor(U"#594a4e");  // グリッドの色（灰色）
	static const ColorF cellColor(0.8, 0.9, 1.0);  // セルの色（薄い青）
	static const ColorF correctTileColor(U"#3da9fc");
	static  const ColorF wrongTileColor(U"#fffffe");
	static const ColorF textColor(U"#d8eefe");
	static const ColorF frameColor(U"#594a4e");
	static const ColorF colors[] = { ColorF(U"#5f6c7b") , ColorF(U"#ef4565") ,ColorF(U"#094067") ,ColorF(U"#33272a") };
	FontAsset::Register(U"Cell", 20);
	static const Font font{ 8 };
	for (int32 y = 0; y < height; ++y) {
		for (int32 x = 0; x < width; ++x) {
			// セルの描画

			// Rect(x * cellSize, y * cellSize, cellSize).draw((grid[y][x] == goal[y][x] ? colors[grid[y][x]] : wrongTileColor));

			// FontAsset(U"Cell")(grid[y][x]).drawAt(x * cellSize + cellSize / 2, y * cellSize + cellSize / 2, textColor);
			if (Max(height, width) < 128) {
				Rect(x * cellSize, y * cellSize, cellSize).draw(colors[grid[y][x]]);
				if (goal[y][x] != grid[y][x]) font(goal[y][x]).drawAt(x * cellSize + cellSize / 2, y * cellSize + cellSize / 2, textColor);
			}
			else {
				Rect(x * cellSize, y * cellSize, cellSize).draw(goal[y][x] == grid[y][x] ? colors[grid[y][x]] : wrongTileColor);
			}


			// グリッドの線を描画
			Rect(x * cellSize, y * cellSize, cellSize).drawFrame(1, gridColor);
		}
	}

	// ボード全体の外枠を描画
	Rect(0, 0, width * cellSize, height * cellSize).drawFrame(2, frameColor);

}

void Board::shift_up(const Grid<bool>& isRemoved) {
	for (int32 x = 0; x < width; ++x) {
		Array<int32> column;
		Array<int32> removed_in_col;
		for (int32 y = 0; y < height; ++y) {
			if (!isRemoved[y][x]) {
				column << grid[y][x];
			}
			else {
				removed_in_col << grid[y][x];
			}
		}
		int32 y = 0;
		for (int32& value : column) {
			grid[y][x] = value;
			++y;
		}
		for (int32& value : removed_in_col) {
			grid[y][x] = value;
			++y;
		}
	}
}

void Board::shift_down(const Grid<bool>& isRemoved) {
	for (int32 x = 0; x < width; ++x) {
		Array<int32> column;
		Array<int32> removed_in_col;
		for (int32 y = height - 1; y >= 0; --y) {
			if (!isRemoved[y][x]) {
				column << grid[y][x];
			}
			else {
				removed_in_col.insert(removed_in_col.begin(), grid[y][x]);
			}
		}
		int32 y = height - 1;
		for (int32& value : column) {
			grid[y][x] = value;
			--y;
		}
		removed_in_col.reverse();
		for (int32& value : removed_in_col) {
			grid[y][x] = value;
			--y;
		}
	}
}

void Board::shift_left(const Grid<bool>& isRemoved) {
	for (int32 y = 0; y < height; ++y) {
		Array<int32> row;
		Array<int32> removed_in_row;
		for (int32 x = 0; x < width; ++x) {
			if (!isRemoved[y][x]) {
				row << grid[y][x];
			}
			else {
				removed_in_row << grid[y][x];
			}
		}
		int32 x = 0;
		for (int32& value : row) {
			grid[y][x] = value;
			++x;
		}

		for (int32& value : removed_in_row) {
			grid[y][x] = value;
			++x;
		}
	}
}

void Board::shift_right(const Grid<bool>& isRemoved) {
	for (int32 y = 0; y < height; ++y) {
		Array<int32> row;
		Array<int32> removed_in_row;
		for (int32 x = width - 1; x >= 0; --x) {
			if (!isRemoved[y][x]) {
				row << grid[y][x];
			}
			else {
				removed_in_row.insert(removed_in_row.begin(), grid[y][x]);
			}
		}
		int32 x = width - 1;
		for (int32& value : row) {
			grid[y][x] = value;
			--x;
		}
		removed_in_row.reverse();
		for (int32& value : removed_in_row) {
			grid[y][x] = value;
			--x;
		}
	}
}


size_t Board::hash() const {
	size_t seed = 0;
	for (int32 y = 0; y < height; ++y) {
		for (int32 x = 0; x < width; ++x) {
			// boost::hash_combine(seed, grid[y][x]);
			seed ^= grid[y][x] + 0x9e3779b9 + (seed < 6) + (seed >> 2);
		}
	}
	return seed;
}


bool Board::operator==(const Board& other) const {
	return grid == other.grid;
}

bool Board::operator!=(const Board& other) const {
	return !(*this == other);
}

int32 Board::calculateDifferenceByRow(int32 row, const Grid<int32>& otherGrid) const {
	int32 diff = 0;
	for (int32 x = 0; x < width; ++x) {
		if (otherGrid[row][x] != goal[row][x]) {
			diff++;
		}
	}
	return diff;
}

bool Board::isRowMatched(int32 row, const Grid<int32>& otherGrid) const {
	for (int32 x = 0; x < width; ++x) {
		if (otherGrid[row][x] != goal[row][x]) {
			return false;
		}
	}
	return true;
}

int32 Board::calculateDistance(int32 x1, int32 y1, int32 x2, int32 y2) const {
	return std::abs(x1 - x2) + std::abs(y1 - y2);  // マンハッタン距離
}

int32 Board::calculateAdvancedDifference(const Grid<int32>& otherGrid) const {
	int32 totalDiff = 0;
	for (int32 y = 0; y < height; ++y) {
		totalDiff += calculateAdvancedDifferenceByRow(y, otherGrid);
	}
	Console << U"advenced Diff : " << totalDiff;
	return totalDiff;
}

int32 Board::calculateAdvancedDifferenceByRow(int32 row, const Grid<int32>& otherGrid) const {
	int32 rowDiff = 0;
	for (int32 x = 0; x < width; ++x) {
		if (otherGrid[row][x] != goal[row][x]) {
			int32 minDistance = std::numeric_limits<int32>::max();
			for (int32 gy = 0; gy < height; ++gy) {
				for (int32 gx = 0; gx < width; ++gx) {
					if (goal[gy][gx] == otherGrid[row][x]) {
						int32 distance = calculateDistance(x, row, gx, gy);
						minDistance = std::min(minDistance, distance);
					}
				}
			}
			rowDiff += minDistance;
		}
	}
	Console << U"adv Row Diff : " << rowDiff;
	return rowDiff;
}


Grid<int32> Board::partialGrid(int32 sy, int32 sx) const {
	Grid<int32> partialGrid(width - sx, height - sy);
	Console << height - sy << U", " << width - sx;
	for (int32 i = sy; i < height; i++) {
		for (int32 j = sx; j < width; j++) {
			partialGrid[i - sy][j - sx] = grid[i][j];
			Console << i - sy << U", " << j - sx;
		}
	}
	return partialGrid;
}

Grid<int32> Board::partialGoal(int32 sy, int32 sx) const {
	Grid<int32> partialGoal(width - sx, height - sy);
	Console << height - sy << U", " << width - sx;
	for (int32 i = sy; i < height; i++) {
		for (int32 j = sx; j < width; j++) {
			partialGoal[i - sy][j - sx] = goal[i][j];
			Console << i - sy << U", " << j - sx;
		}
	}
	return partialGoal;
}

Point Board::BFS(Point start, int32 target) const {
	int32 sy = start.y, sx = start.x;
	Point pos = start;
	int32 minDist = 1e9;
	std::queue<Point> que;
	que.push(Point(sx, sy));
	Grid<int32> dist(grid.height(), grid.width(), -1);
	int32 dy[] = { 1, 0, -1, 0 }, dx[] = { 0, 1, 0, -1 };
	dist[sy][sx] = 0;
	while (!que.empty()) {
		Point nextPos = que.front();
		que.pop();
		int32 y = nextPos.y, x = nextPos.x;

		Console << nextPos;

		for (int32 i : step(4)) {
			int32 ny = dy[i] + y, nx = dx[i] + x;
			// Console << U"ny, nx:" << ny << U",  " << nx;
			if (ny < 0 || ny >= grid.height() || nx < 0 || nx >= grid.width()) continue;
			if (dist[ny][nx] != -1) continue;

			dist[ny][nx] = dist[y][x] + 1;
			// Console << U"dist::" << dist[ny][nx];
			que.push(Point(nx, ny));

			if (goal[sy][sx] == grid[ny][nx] && grid[ny][nx] != goal[ny][nx]) {
				return Point(nx, ny);
				if (minDist > dist[ny][nx]) {
					minDist = dist[ny][nx];
					pos = Point(nx, ny);
				}
			}

		}
	}
	// Console << U"dist: " << dist;
	return pos;
}

Point Board::BFSbyPopcount(Point start, int32 target) const {
	int32 sy = start.y, sx = start.x;
	Point best = Point(-1, -1);

	static Grid<int32> distances(width * height, width * height);
	static Grid<Array<std::pair<int32, Point>>> sortedDistances(width, height); // (sy, sx) に近いものを順に入れたい

	static bool initialized;
	if (sortedDistances.width() != width || sortedDistances.height() != height) {
		distances = Grid<int32>(width * height, width * height);
		sortedDistances = Grid<Array<std::pair<int32, Point>>>(width, height);
		initialized = false;
	}
	if (initialized) {
		// Console << U"calculated";
	}
	else {

		for (int32 y1 : step(height)) {
			Console << U"y1: " << y1;
			for (int32 x1 : step(width)) {
				for (int32 y2 : step(height)) {
					for (int32 x2 : step(width)) {
						int32 dx = x1 - x2;
						int32 dy = y1 - y2;
						int32 dist = std::popcount(static_cast<uint32>(Abs(dx))) + std::popcount(static_cast<uint32>(Abs(dy)));
						// int32 dist = std::popcount(static_cast<uint32>(Abs(dx))) + (dy > 0); // yに関して，1手で変更可能
						if (dist == 0) continue;
						if (abs(dx) + abs(dy) == 1) dist += 2; // 大きいものを使いたい
						// Point(y1, x1) -> int32
						distances[y1 * width + x1][y2 * width + x2] = dist;
						// sortedDistances[y1][x1].push_back({ dist, Point(x2, y2) });
						sortedDistances[y1][x1].emplace_back(std::pair(dist, Point(x2, y2)));
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

	for (const auto& p : sortedDistances[sy][sx]) {
		const auto& x = p.second.x, & y = p.second.y;
		if (grid[y][x] == target && grid[y][x] != goal[y][x]) {
			// return p.second;
			best = Point(x, y);
			break;
		}
	}



	/* 愚直
	for (int32 y : step(height)) {
		for (int32 x : step(width)) {
			if (grid[y][x] == target && grid[y][x] != goal[y][x]) {
				uint32 dy = AbsDiff(y, sy), dx = AbsDiff(x, sx);
				int32 pcount = std::popcount(dx) + std::popcount(dy);
				if (dist > pcount) {
					best = Point(x, y);
					dist = pcount;
				}
			}
		}
	}
	*/
	return best;
}

std::vector<std::pair<int32, int32>> Board::sortToMatchPartially(int32 targetRow)const {
	Array<int32> A(width), B(width);
	for (int32 i : step(width)) {
		A[i] = grid[targetRow][i];
		B[i] = goal[targetRow][i];
	}
	std::vector<std::pair<int32, int32>> swaps;
	std::unordered_map<int32, int32> countA, countB;

	// 各要素の出現回数をカウント
	for (int32 num : A) countA[num]++;
	for (int32 num : B) countB[num]++;

	// 第1段階: 共通要素を正しい位置に配置
	for (int32 i = 0; i < A.size(); ++i) {
		if (A[i] != B[i] && countA[B[i]] > 0) {
			for (int32 j = i + 1; j < A.size(); ++j) {
				if (A[j] == B[i]) {
					swaps.emplace_back(i, j);  // インデックスを記録
					std::swap(A[i], A[j]);
					break;
				}
			}
		}
		if (countA[B[i]] > 0) countA[B[i]]--;
	}

	// 第2段階: 残りの要素をできるだけBの順序に近づける
	std::vector<int32> remainingB;
	for (int32 num : B) {
		if (countB[num] > 0) {
			remainingB.push_back(num);
			countB[num]--;
		}
	}

	int32 j = 0;
	for (int32 i = 0; i < A.size(); ++i) {
		if (std::find(B.begin(), B.end(), A[i]) == B.end()) {
			while (j < remainingB.size() && std::find(A.begin(), A.end(), remainingB[j]) != A.end()) {
				j++;
			}
			if (j < remainingB.size()) {
				swaps.emplace_back(i, -1);  // -1は新しい値を示す特別なインデックス
				A[i] = remainingB[j];
				j++;
			}
		}
	}

	return swaps;
}

int32 Board::calculateNextDifference(const Pattern& pattern, Point pos, int32 direction) const {
	Board newBoard = this->applyPatternCopy(pattern, pos, direction);
	int32 diff = 0;
	for (int32 y = 0; y < height; ++y) {
		for (int32 x = 0; x < width; ++x) {
			if (goal[y][x] != newBoard.grid[y][x]) {
				diff++;
			}
		}
	}
	return diff;
}

int32 Board::calculateNextProgress(const Pattern& pattern, Point pos, int32 direction) const {
	return height * width - calculateNextDifference(pattern, pos, direction);
}
