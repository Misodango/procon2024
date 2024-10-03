// algorithm.hpp

#pragma once
#include <Siv3D.hpp>
#include "Board.h"
#include "Pattern.h"

namespace Algorithm {

	enum class Type {
		Greedy,
		BeamSearch
	};

	struct Solution {
		// 抜き型 座標 方向
		Array<std::tuple<Pattern, Point, int32>> steps;
		int32 score = 0;
		Grid<int32> grid = Grid<int32>();
		void outuputToJson() {
			JSON output;
			output[U"n"] = static_cast<int32>(steps.size());
			Array<JSON> ops;

			for (const auto& [solvePattern, solvePos, solveDir] : steps)
			{
				JSON stepJson;
				stepJson[U"p"] = solvePattern.p;
				stepJson[U"x"] = solvePos.x;
				stepJson[U"y"] = solvePos.y;
				stepJson[U"s"] = solveDir;
				ops << stepJson;
			}

			output[U"ops"] = ops;
			Console << U"Writing ...";
			output.save(U"output.json");
			Console << U"Wrote ";
			JSON submission;
			try {
				// submission = JSON::Load(U"output.json");
				const FilePath path = FileSystem::FullPath(U"output.json");
				Console << U"Full path: " << path;
				JSON submission = JSON::Load(path);
				if (not submission) {
					throw Error(U"Failed to load JSON file");
				}
				Console << U"Loaded JSON: " << submission;
			}
			catch (const Error& error) {
				Console << U"Error loading JSON: " << error.what();
			}
		}
	};

	Solution greedy(const Board& initialBoard, const Array<Pattern>& patterns);

	Solution beamSearch(const Board& initialBoard, const Array<Pattern>& patterns);

	Solution solve(Type algorithmType, const Board& initialBoard, const Array<Pattern>& patterns);


}
