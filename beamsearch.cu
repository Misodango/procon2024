// �傫�ȃO���b�h�ɑΉ�����r�b�g�{�[�h�\��
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

// ���񏈗��̂��߂̃O���b�h����
struct GridPartition {
    int start_x, start_y, end_x, end_y;
};

// nextState�J�[�l��
__global__ void nextStateKernel(LargeBitBoard* boards, int num_boards, LargeBitBoard* patterns, int num_patterns, Solution* solutions, GridPartition* partitions) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= num_boards) return;

    LargeBitBoard& board = boards[idx];
    GridPartition partition = partitions[idx];

    // �p�[�e�B�V�������ł�nextState����
    for (int y = partition.start_y; y < partition.end_y; ++y) {
        for (int x = partition.start_x; x < partition.end_x; ++x) {
            // �p�^�[���}�b�`���O�ƈړ�����
            // ...
        }
    }

    // ���ʂ�solutions�ɏ�������
}

// �r�[���T�[�`�J�[�l��
__global__ void beamSearchKernel(LargeBitBoard* boards, int num_boards, LargeBitBoard* patterns, int num_patterns, Solution* solutions, double* scores, GridPartition* partitions) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx >= num_boards) return;

    LargeBitBoard& board = boards[idx];
    GridPartition partition = partitions[idx];

    // �p�[�e�B�V�������ł̃r�[���T�[�`����
    double score = 0.0;
    for (int y = partition.start_y; y < partition.end_y; ++y) {
        for (int x = partition.start_x; x < partition.end_x; ++x) {
            // �]���֐��̌v�Z
            // ...
        }
    }

    // �X�R�A�Ɖ����O���[�o���������ɏ�������
    scores[idx] = score;
    // solutions[idx] = ...
}

// �V�t�g����̍œK���i��F������ւ̃V�t�g�j
__global__ void shiftUpKernel(LargeBitBoard* board, const Point* removed_cells, int num_removed) {
    int x = blockIdx.x * blockDim.x + threadIdx.x;
    if (x >= board->width) return;

    // �񂲂Ƃ̕��񏈗�
    uint64_t column = 0;
    for (int y = 0; y < board->height; ++y) {
        int value = board->get(x, y);
        if (std::find(removed_cells, removed_cells + num_removed, Point{ x, y }) == removed_cells + num_removed) {
            column |= (uint64_t)value << (y * 4);
        }
    }

    // �V�t�g�����Ə����߂�
    // ...
}

// �z�X�g���̃R�[�h
void cudaLargeGridBeamSearch(const Board& initialBoard, const Array<Pattern>& patterns, int32 beamWidth, int32 maxSteps) {
    // GPU �������̊��蓖�ĂƏ�����
    LargeBitBoard* d_boards;
    LargeBitBoard* d_patterns;
    Solution* d_solutions;
    double* d_scores;
    GridPartition* d_partitions;

    // ���������蓖�Ăƃf�[�^�]��
    // ...

    // �O���b�h�����̐ݒ�
    int partitionSize = 32; // �܂��͓K�؂ȃT�C�Y
    setGridPartitions << <(initialBoard.width * initialBoard.height + 255) / 256, 256 >> > (d_partitions, initialBoard.width, initialBoard.height, partitionSize);

    // �r�[���T�[�`�̎��s
    for (int step = 0; step < maxSteps; ++step) {
        nextStateKernel << <(beamWidth + 255) / 256, 256 >> > (d_boards, beamWidth, d_patterns, patterns.size(), d_solutions, d_partitions);
        beamSearchKernel << <(beamWidth + 255) / 256, 256 >> > (d_boards, beamWidth, d_patterns, patterns.size(), d_solutions, d_scores, d_partitions);

        // �r�[���̍X�V�iCPU���ŏ������邩�A�ʂ�CUDA�J�[�l���Ŏ����j
        // ...
    }

    // ���ʂ̉���ƌ㏈��
    // ...

    // GPU �������̉��
    // ...
}