// 大きなグリッドに対応するビットボード構造
struct LargeBitBoard {
    uint64_t* bits;
    int width;
    int height;

    __host__ __device__
        LargeBitBoard(int w, int h) : width(w), height(h) {
        int size = (w * h + 63) / 64;
        bits = new uint64_t[size]();
    }

    __host__ __device__
        ~LargeBitBoard() {
        delete[] bits;
    }

    __host__ __device__
        void set(int x, int y, int value) {
        int idx = y * width + x;
        int board_idx = idx / 64;
        int bit_idx = idx % 64;
        bits[board_idx] |= (uint64_t)value << (bit_idx * 4);
    }

    __host__ __device__
        int get(int x, int y) const {
        int idx = y * width + x;
        int board_idx = idx / 64;
        int bit_idx = idx % 64;
        return (bits[board_idx] >> (bit_idx * 4)) & 0xF;
    }
};

// 並列処理のためのグリッド分割
struct GridPartition {
    int start_x, start_y, end_x, end_y;
};

// nextStateカーネル
__global__ void nextStateKernel(LargeBitBoard* boards, int num_boards, LargeBitBoard* patterns, int num_patterns, Solution* solutions, GridPartition* partitions) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= num_boards) return;

    LargeBitBoard& board = boards[idx];
    GridPartition partition = partitions[idx];

    // パーティション内でのnextState処理
    for (int y = partition.start_y; y < partition.end_y; ++y) {
        for (int x = partition.start_x; x < partition.end_x; ++x) {
            // パターンマッチングと移動処理
            // ...
        }
    }

    // 結果をsolutionsに書き込む
}

// ビームサーチカーネル
__global__ void beamSearchKernel(LargeBitBoard* boards, int num_boards, LargeBitBoard* patterns, int num_patterns, Solution* solutions, double* scores, GridPartition* partitions) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= num_boards) return;

    LargeBitBoard& board = boards[idx];
    GridPartition partition = partitions[idx];

    // パーティション内でのビームサーチ処理
    double score = 0.0;
    for (int y = partition.start_y; y < partition.end_y; ++y) {
        for (int x = partition.start_x; x < partition.end_x; ++x) {
            // 評価関数の計算
            // ...
        }
    }

    // スコアと解をグローバルメモリに書き込む
    scores[idx] = score;
    // solutions[idx] = ...
}

// シフト操作の最適化（例：上方向へのシフト）
__global__ void shiftUpKernel(LargeBitBoard* board, const Point* removed_cells, int num_removed) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    if (x >= board->width) return;

    // 列ごとの並列処理
    uint64_t column = 0;
    for (int y = 0; y < board->height; ++y) {
        int value = board->get(x, y);
        if (std::find(removed_cells, removed_cells + num_removed, Point{ x, y }) == removed_cells + num_removed) {
            column |= (uint64_t)value << (y * 4);
        }
    }

    // シフト処理と書き戻し
    // ...
}

// ホスト側のコード
void cudaLargeGridBeamSearch(const Board& initialBoard, const Array<Pattern>& patterns, int32 beamWidth, int32 maxSteps) {
    // GPU メモリの割り当てと初期化
    LargeBitBoard* d_boards;
    LargeBitBoard* d_patterns;
    Solution* d_solutions;
    double* d_scores;
    GridPartition* d_partitions;

    // メモリ割り当てとデータ転送
    // ...

    // グリッド分割の設定
    int partitionSize = 32; // または適切なサイズ
    setGridPartitions << <(initialBoard.width * initialBoard.height + 255) / 256, 256 >> > (d_partitions, initialBoard.width, initialBoard.height, partitionSize);

    // ビームサーチの実行
    for (int step = 0; step < maxSteps; ++step) {
        nextStateKernel << <(beamWidth + 255) / 256, 256 >> > (d_boards, beamWidth, d_patterns, patterns.size(), d_solutions, d_partitions);
        beamSearchKernel << <(beamWidth + 255) / 256, 256 >> > (d_boards, beamWidth, d_patterns, patterns.size(), d_solutions, d_scores, d_partitions);

        // ビームの更新（CPU側で処理するか、別のCUDAカーネルで実装）
        // ...
    }

    // 結果の回収と後処理
    // ...

    // GPU メモリの解放
    // ...
}