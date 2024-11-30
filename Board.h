#pragma once
// Board.hpp

#include <Siv3D.hpp>
#include "Pattern.h"

/**
 * @brief ビジュアライザ用のクラス
 *
 * 問題解決時には Algorithm.cpp にある `OptimizedBoard` を使用。
 */
class Board {
public:
	// 現在の盤面
	Grid<int32> grid;

	// 目的の盤面
	Grid<int32> goal;

	// サイズ
	int32 width, height;

	/**
	 * @brief コンストラクタ
	 * @param w 盤面の幅
	 * @param h 盤面の高さ
	 */
	Board(int32 w, int32 h);

	/**
	 * @brief JSONデータから盤面を初期化
	 * @param json JSONデータ
	 * @return Board 初期化されたBoardオブジェクト
	 * @details JSON形式のデータを解析し、盤面を生成します。
	 */
	static Board fromJSON(const JSON& json);

	/**
	 * @brief ゴール判定
	 * @return true 目標の盤面と一致している場合
	 * @return false 一致していない場合
	 */
	bool is_goal() const;

	/**
	 * @brief パターンを指定位置に適用
	 * @param pattern 適用する抜き型
	 * @param pos 適用位置
	 * @param direction 適用方向（0: 上, 1: 右, 2: 下, 3: 左）
	 */
	void apply_pattern(const Pattern& pattern, Point pos, int32 direction);

	/**
	 * @brief Siv3Dの描画処理
	 * @details 現在の盤面をSiv3Dで描画します。
	 */
	void draw() const;

	/**
	 * @brief ゴール状態との差異を計算
	 * @param otherGrid 比較対象のグリッド
	 * @return int32 差異の数
	 */
	int32 calculateDifference(const Grid<int32>& otherGrid) const;

	/**
	 * @brief パターンを適用した結果の盤面を返す（実際には適用しない）
	 * @param pattern 適用する抜き型
	 * @param pos 適用位置
	 * @param direction 適用方向
	 * @return Board 適用後の盤面
	 */
	Board applyPatternCopy(const Pattern& pattern, Point pos, int32 direction) const;

	/**
	 * @brief ハッシュ関数を提供
	 * @return size_t ハッシュ値
	 */
	size_t hash() const;

	/**
	 * @brief 等価比較
	 * @param other 比較対象のBoard
	 * @return true 一致している場合
	 * @return false 一致していない場合
	 */
	bool operator==(const Board& other) const;
	bool operator!=(const Board& other) const;

	/**
	 * @brief パターン適用後の盤面全体の差異を計算
	 * @param pattern 適用する抜き型
	 * @param pos 適用位置
	 * @param direction 適用方向
	 * @return int32 差異の数
	 */
	int32 calculateNextDifference(const Pattern& pattern, Point pos, int32 direction) const;

	/**
	 * @brief パターン適用後の進捗を計算
	 * @param pattern 適用する抜き型
	 * @param pos 適用位置
	 * @param direction 適用方向
	 * @return int32 進捗量
	 */
	int32 calculateNextProgress(const Pattern& pattern, Point pos, int32 direction) const;

private:
	// 上下左右への適用処理
	void shift_up(const Grid<bool>& isRemoved);
	void shift_down(const Grid<bool>& isRemoved);
	void shift_left(const Grid<bool>& isRemoved);
	void shift_right(const Grid<bool>& isRemoved);
};

// ハッシュ関数の定義
namespace std {
	template <>
	struct hash<Board> {
		/**
		 * @brief Boardオブジェクトのハッシュ関数
		 * @param b ハッシュ対象のBoard
		 * @return size_t ハッシュ値
		 * @details 現在の盤面と目標盤面の両方を使用してハッシュを生成。
		 */
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
