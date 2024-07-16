// StandardPatterns.hpp

#pragma once
#include <Siv3D.hpp>
#include "Pattern.h"  // Pattern クラスのヘッダーをインクルード

namespace StandardPatterns {

    // すべての定型抜き型を格納する配列
    inline Array<Pattern> AllPatterns;

    // 初期化関数
    void Initialize() {
        AllPatterns = {
            Pattern(Grid<int32>{ {1}}, 1, U"Type11"),
            Pattern(Grid<int32>{ {1,1},{1,1}}, 2, U"Type12"),
            Pattern(Grid<int32>(4, 4, 1), 3, U"Type14"),
            Pattern(Grid<int32>(8, 8, 1), 4, U"Type18"),
            Pattern(Grid<int32>(16, 16, 1), 5, U"Type116"),
            Pattern(Grid<int32>(32, 32, 1), 6, U"Type132"),
            Pattern(Grid<int32>(64, 64, 1), 7, U"Type164"),
            Pattern(Grid<int32>(128, 128, 1), 8, U"Type1128"),
            Pattern(Grid<int32>(256, 256, 1), 9, U"Type1256"),

            Pattern(Grid<int32>{ {1,1},{0,0}}, 10, U"Type22"),
            Pattern(Grid<int32>(4, 4, [](Point p) { return p.y % 2 == 0 ? 1 : 0; }), 11, U"Type24"),
            Pattern(Grid<int32>(8, 8, [](Point p) { return p.y % 2 == 0 ? 1 : 0; }), 12, U"Type28"),
            Pattern(Grid<int32>(16, 16, [](Point p) { return p.y % 2 == 0 ? 1 : 0; }), 13, U"Type216"),
            Pattern(Grid<int32>(32, 32, [](Point p) { return p.y % 2 == 0 ? 1 : 0; }), 14, U"Type232"),
            Pattern(Grid<int32>(64, 64, [](Point p) { return p.y % 2 == 0 ? 1 : 0; }), 15, U"Type264"),
            Pattern(Grid<int32>(128, 128, [](Point p) { return p.y % 2 == 0 ? 1 : 0; }), 16, U"Type2128"),
            Pattern(Grid<int32>(256, 256, [](Point p) { return p.y % 2 == 0 ? 1 : 0; }), 17, U"Type2256"),

            Pattern(Grid<int32>{ {1,0},{1,0}}, 18, U"Type32"),
            Pattern(Grid<int32>(4, 4, [](Point p) { return p.x % 2 == 0 ? 1 : 0; }), 19, U"Type34"),
            Pattern(Grid<int32>(8, 8, [](Point p) { return p.x % 2 == 0 ? 1 : 0; }), 20, U"Type38"),
            Pattern(Grid<int32>(16, 16, [](Point p) { return p.x % 2 == 0 ? 1 : 0; }), 21, U"Type316"),
            Pattern(Grid<int32>(32, 32, [](Point p) { return p.x % 2 == 0 ? 1 : 0; }), 22, U"Type332"),
            Pattern(Grid<int32>(64, 64, [](Point p) { return p.x % 2 == 0 ? 1 : 0; }), 23, U"Type364"),
            Pattern(Grid<int32>(128, 128, [](Point p) { return p.x % 2 == 0 ? 1 : 0; }), 24, U"Type3128"),
            Pattern(Grid<int32>(256, 256, [](Point p) { return p.x % 2 == 0 ? 1 : 0; }), 25, U"Type3256")
        };
    }

    // パターン名からパターンを取得する関数
    Optional<Pattern> getPatternByName(const String& name) {
        for (const auto& pattern : AllPatterns) {
            if (pattern.name == name) {
                return pattern;
            }
        }
        return none;
    }

    // すべてのパターン名を取得する関数
    Array<String> getAllPatternNames() {
        return AllPatterns.map([](const Pattern& p) { return p.name; });
    }
}
