#pragma once
// Board.hpp

/*
*  このクラスはビジュアライザ用のクラスです。
*  問題を解くときには Algorithm.cppにある `OptimizedBoard`を使用します。
*/


#include <Siv3D.hpp>
#include "Pattern.h"

class Board {
public:

	// 現在の盤面
	Grid<int32> grid;

	// 目的の盤面
	Grid<int32> goal;

	// サイズ
	int32 width, height;

	// 初期化
	Board(int32 w, int32 h);

	// JSONで初期化
	static Board fromJSON(const JSON& json);

	// ゴール判定
	bool is_goal() const;

	// 抜き型の適用
	void apply_pattern(const Pattern& pattern, Point pos, int32 direction);

	// siv3dへの描画
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

	// 任意の抜き型を適用した時の盤面全体のゴールとの差異
	// 実際には適用しない
	int32 calculateNextDifference(const Pattern& pattern, Point pos, int32 direction) const;

	// 任意の抜き型を適用した時の期待値
	int32 calculateNextProgress(const Pattern& pattern, Point pos, int32 direction) const;


private:
	// 上下左右への適用
	void shift_up(const Grid<bool>& isRemoved);
	void shift_down(const Grid<bool>& isRemoved);
	void shift_left(const Grid<bool>& isRemoved);
	void shift_right(const Grid<bool>& isRemoved);

};

// ハッシュ関数
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
