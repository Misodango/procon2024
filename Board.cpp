// Board.cpp

#include "Board.h"


Board::Board(int32 w, int32 h) : width(w), height(h), grid(w, h, 0), goal(w, h, 0) {}

// JSONから初期化
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

// ゴールかどうか
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

// 抜き型の適用
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

// siv3dのUIに描画
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

// 上向き適用
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

// 下向き適用
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

// 左向き適用
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

// 右向き適用
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

// ハッシュ関数
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

// 一致
bool Board::operator==(const Board& other) const {
	return grid == other.grid;
}

// 不一致
bool Board::operator!=(const Board& other) const {
	return !(*this == other);
}

// 任意の抜き型を適用した時の盤面全体のゴールとの差異
// 実際には適用しない
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

// 任意の抜き型を適用した時の盤面の期待値
int32 Board::calculateNextProgress(const Pattern& pattern, Point pos, int32 direction) const {
	return height * width - calculateNextDifference(pattern, pos, direction);
}
