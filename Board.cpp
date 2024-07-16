﻿// Board.cpp

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
	Console << otherGrid;
	Console << goal;
	Console << height << U" " << width;
	int32 diff = 0;
	for (int32 y = 0; y < height; ++y) {
		for (int32 x = 0; x < width; ++x) {
			if (goal[y][x] != otherGrid[y][x]) {
				diff++;
			}
		}
	}
	Console << U"diff: {}"_fmt(diff);
	return diff;
}

void Board::apply_pattern(const Pattern& pattern, Point pos, int32 direction) {
	Array<Point> removed_cells;
	for (int32 y = 0; y < pattern.grid.height(); ++y) {
		for (int32 x = 0; x < pattern.grid.width(); ++x) {
			if (pattern.grid[y][x] == 1) {
				int32 bx = pos.x + x, by = pos.y + y;
				if (0 <= bx && bx < width && 0 <= by && by < height) {
					removed_cells << Point{ bx, by };
				}
			}
		}
	}

	switch (direction) {
	case 0: // up
		shift_up(removed_cells);
		break;
	case 1: // down
		shift_down(removed_cells);
		break;
	case 2: // left
		shift_left(removed_cells);
		break;
	case 3: // right
		shift_right(removed_cells);
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
	const int32 cellSize = 30;
	const ColorF gridColor(0.5, 0.5, 0.5);  // グリッドの色（灰色）
	const ColorF cellColor(0.8, 0.9, 1.0);  // セルの色（薄い青）

	for (int32 y = 0; y < height; ++y) {
		for (int32 x = 0; x < width; ++x) {
			// セルの描画

			Rect(x * cellSize, y * cellSize, cellSize).draw((grid[y][x] == goal[y][x] ? Palette::Aqua : Palette::Black));
			FontAsset(U"Cell")(grid[y][x]).drawAt(x * cellSize + cellSize / 2, y * cellSize + cellSize / 2);


			// グリッドの線を描画
			Rect(x * cellSize, y * cellSize, cellSize).drawFrame(1, gridColor);
		}
	}

	// ボード全体の外枠を描画
	Rect(0, 0, width * cellSize, height * cellSize).drawFrame(2, Palette::Navy);

}

void Board::shift_up(const Array<Point>& removed_cells) {
	for (int32 x = 0; x < width; ++x) {
		Array<int32> column;
		Array<int32> removed_in_col;
		for (int32 y = 0; y < height; ++y) {
			if (std::find(removed_cells.begin(), removed_cells.end(), Point{ x, y }) == removed_cells.end()) {
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

void Board::shift_down(const Array<Point>& removed_cells) {
	for (int32 x = 0; x < width; ++x) {
		Array<int32> column;
		Array<int32> removed_in_col;
		for (int32 y = height - 1; y >= 0; --y) {
			if (std::find(removed_cells.begin(), removed_cells.end(), Point{ x, y }) == removed_cells.end()) {
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

void Board::shift_left(const Array<Point>& removed_cells) {
	for (int32 y = 0; y < height; ++y) {
		Array<int32> row;
		Array<int32> removed_in_row;
		for (int32 x = 0; x < width; ++x) {
			if (std::find(removed_cells.begin(), removed_cells.end(), Point{ x, y }) == removed_cells.end()) {
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

void Board::shift_right(const Array<Point>& removed_cells) {
	for (int32 y = 0; y < height; ++y) {
		Array<int32> row;
		Array<int32> removed_in_row;
		for (int32 x = width - 1; x >= 0; --x) {
			if (std::find(removed_cells.begin(), removed_cells.end(), Point{ x, y }) == removed_cells.end()) {
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
	for (int32 i = sy; i < height; i++ ) {
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