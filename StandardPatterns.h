// StandardPatterns.hpp

#pragma once
// #include <Siv3D.hpp>
// #include "Pattern.h"  // Pattern クラスのヘッダーをインクルード

namespace StandardPatterns {

	Grid<int32> createStandardPattern(int32 type, int32 size) {
		Grid<int32> pattern(size, size, 0);

		switch (type) {
		case 1: // タイプⅠ: すべてのセルが1
			pattern.fill(1);
			break;
		case 2: // タイプⅡ: 偶数行のセルが1で，奇数行のセルが0
			for (int32 y = 0; y < size; y += 2) {
				for (int32 x = 0; x < size; ++x) {
					pattern[y][x] = 1;
				}
			}
			break;
		case 3: // タイプⅢ: 偶数列のセルが1で，奇数列のセルが0
			for (int32 y = 0; y < size; ++y) {
				for (int32 x = 0; x < size; x += 2) {
					pattern[y][x] = 1;
				}
			}
			break;
		}

		return pattern;
	}

	// すべての定型抜き型を生成し、配列として返す関数
	Array<Grid<int32>> getAllStandardPatterns() {
		Array<Grid<int32>> patterns;
		Array<int32> sizes = { 1, 2, 4, 8, 16, 32, 64, 128, 256 };

		for (int32 size : sizes) {
			for (int32 type = 1; type <= 3; ++type) {
				// サイズ1の場合は1回だけ追加（3タイプは同型）
				if (size == 1 && type > 1) continue;
				patterns << createStandardPattern(type, size);
			}
		}

		return patterns;
	}

	// パターンの名前を生成する関数
	String getPatternName(int32 type, int32 size) {
		return U"Type{}{}"_fmt(type, size);
	}

	// すべての定型抜き型の名前と対応するGridを取得する関数
	std::pair<Array<String>, Array<Grid<int32>>> getNamedStandardPatterns() {
		Array<String> names;
		Array<Grid<int32>> patterns;
		Array<int32> sizes = { 1, 2, 4, 8, 16, 32, 64, 128, 256 };

		for (int32 size : sizes) {
			for (int32 type = 1; type <= 3; ++type) {
				if (size == 1 && type > 1) continue;
				names << getPatternName(type, size);
				patterns << createStandardPattern(type, size);
			}
		}

		return { names, patterns };
	}

	Array<Pattern> getAllStandardPatterns_Grid() {
		Array<Pattern> patterns;
		Array<int32> sizes = { 1, 2, 4, 8, 16, 32, 64, 128, 256 };
		int num = 0;
		for (int32 size : sizes) {
			for (int32 type = 1; type <= 3; ++type) {
				if (size == 1 && type > 1) continue;
				patterns << Pattern(createStandardPattern(type, size), num++, getPatternName(type, size));
			}
		}
		return patterns;
	}
}
