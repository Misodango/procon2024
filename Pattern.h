// Pattern.h

#pragma once
// #include <Siv3D.hpp>

class Pattern {
public:
	// 抜き型のグリッド表現
	Grid<int32> grid;

	// 抜き型の番号
	int32 p;

	// 抜き型の名称
	String name;

	// 初期化
	Pattern(const Grid<int32>& g, const int32& num, const String& n = U"")
		: grid(g), p(num), name(n) {}

	// JSONから初期化
	static Pattern fromJSON(const JSON& json) {
		const int32 p = json[U"p"].get<int32>();
		const int32 width = json[U"width"].get<int32>();
		const int32 height = json[U"height"].get<int32>();
		const String name = U"test";

		Grid<int32> grid(width, height, 0);
		if (json[U"cells"].isArray()) {
			const JSONArrayView cellsArray = json[U"cells"].arrayView();
			for (int32 y = 0; y < height; ++y) {
				const String row = cellsArray[y].getString();
				for (int32 x = 0; x < width; ++x) {
					grid[y][x] = row[x] - '0';
				}
			}
		}
		else if (json[U"cells"].isString()) {
			const String cells = json[U"cells"].getString();
			for (int32 i = 0; i < cells.length(); ++i) {
				grid[i / width][i % width] = cells[i] - '0';
			}
		}
		else {
			throw Error(U"Unexpected format for 'cells' in Pattern JSON");
		}

		return Pattern(grid, p, name);
	}

	// siv3d用の描画
	void draw(const Point& pos, int32 cellSize, const ColorF& color = Palette::Red) const {
		for (int32 y = 0; y < grid.height(); ++y) {
			for (int32 x = 0; x < grid.width(); ++x) {
				if (grid[y][x] == 1) {
					Rect((pos.x + x) * cellSize, (pos.y + y) * cellSize, cellSize)
						.draw(ColorF(color, 0.5));
				}
			}
		}
	}
};
