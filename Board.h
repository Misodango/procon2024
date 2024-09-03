#pragma once
// Board.hpp

#include <Siv3D.hpp>
#include "Pattern.h"

class Board {
public:
	Grid<int32> grid;
	Grid<int32> goal;
	int32 width, height;

	Board(int32 w, int32 h);

	static Board fromJSON(const JSON& json);

	bool is_goal() const;

	void apply_pattern(const Pattern& pattern, Point pos, int32 direction);

	void draw() const;

	// ゴール状態との差異を計算
	int32 calculateDifference(const Grid<int32>& otherGrid) const;

	// パターンを適用した結果のボードを返す（実際には適用しない）
	Board applyPatternCopy(const Pattern& pattern, Point pos, int32 direction) const;

	// ハッシュ関数のサポート
	size_t hash() const;

	// 等価性比較のサポート
	bool operator==(const Board& other) const;
	bool operator!=(const Board& other) const;

	int32 calculateDifferenceByRow(int32 row, const Grid<int32>& otherGrid) const;
	bool isRowMatched(int32 row, const Grid<int32>& otherGrid) const;

	int32 calculateAdvancedDifference(const Grid<int32>& otherGrid) const;
	int32 calculateAdvancedDifferenceByRow(int32 row, const Grid<int32>& otherGrid) const;

	// 部分グリッドを取得
	//  sy, sxを左上始点に直す
	Grid<int32> partialGrid(int32 sy, int32 sx) const;
	Grid<int32> partialGoal(int32 sy, int32 sx) const;

	
	Point BFS(Point start, int32 target) const;
	Point BFSbyPopcount(Point start, int32 target) const;
	std::vector<std::pair<int32, int32>> sortToMatchPartially(int32 targetRow) const;

	int32 calculateNextDifference(const Pattern& pattern, Point pos, int32 direction) const;

	int32 calculateNextProgress(const Pattern& pattern, Point pos, int32 direction) const;


private:
	void shift_up(const Grid<bool>& isRemoved);
	void shift_down(const Grid<bool>& isRemoved);
	void shift_left(const Grid<bool>& isRemoved);
	void shift_right(const Grid<bool>& isRemoved);
	int32 calculateDistance(int32 x1, int32 y1, int32 x2, int32 y2) const;
	
};

// std::unordered_mapでBoardをキーとして使用するためのハッシュ関数
/*
namespace std {
	template <>
	struct hash<Board> {
		size_t operator()(const Board& b) const {
			return b.hash();
		}
	};
}
*/

namespace std {
	template <>
	struct hash<Board> {
		size_t operator()(const Board& b) const {
			size_t seed = 0;
			for (int y = 0; y < b.height; y++) {
				for (int x = 0; x < b.width; x++) {
					seed ^= std::hash<int32>()(b.grid[y][x]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				}
			}
			for (int y = 0; y < b.height; y++) {
				for (int x = 0; x < b.width; x++) {
					seed ^= std::hash<int32>()(b.goal[y][x]) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
				}
			}
			return seed;
		}
	};
}
