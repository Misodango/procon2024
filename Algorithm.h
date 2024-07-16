﻿// algorithm.hpp

#pragma once
#include <Siv3D.hpp>
#include "Board.h"
#include "Pattern.h"

namespace Algorithm {

	enum class Type {
		TimeLimitiedGreedy,
		Greedy,
		BeamSearch,
		DynamicProgramming,
		RowByGreedy,
		RowByRowAdvancedGreedy,
		OneByOne,
		DiagonalSearch,
		SimulatedAnnealing,
	};

	struct Solution {
		// 抜き型 座標 方向
		Array<std::tuple<Pattern, Point, int32>> steps;
		int32 score;
	};

	Solution timeLimitedGreedy(const Board& initialBoard, const Array<Pattern>& patterns, int32 time = -1);
	Solution greedy(const Board& initialBoard, const Array<Pattern>& patterns);
	Solution beamSearch(const Board& initialBoard, const Array<Pattern>& patterns, int32 beamWidth, int32 maxSteps = 1000);
	Solution dynamicProgramming(const Board& initialBoard, const Array<Pattern>& patterns, double timeLimit);
	Solution rowByRowAdvancedGreedy(const Board& initialBoard, const Array<Pattern>& patterns);
	Solution oneByOne(const Board& initialBoard, const Array<Pattern>& patterns);
	Solution diagonalSearch(const Board& initialBoard, const Array<Pattern>& patterns);
	Solution simulatedAnnealing(const Board& initialBoard, const Array<Pattern>& patterns, int32 sy = 0, int32 sx = 0,
	 int32 number = 100, double startTemp = 500, double endTemp = 100);
	//Solution dijkstra(const Board& initialBoard, const Array<Pattern>& patterns);

	Solution solve(Type algorithmType, const Board& initialBoard, const Array<Pattern>& patterns);


	// Solution geneticAlgorithm(const Board& initialBoard, const Array<Pattern>& patterns);
	// Solution simulatedAnnealing(const Board& initialBoard, const Array<Pattern>& patterns);
}