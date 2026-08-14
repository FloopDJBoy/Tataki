//
// Created by FloopDJBoy on 10/08/2026.
//

#ifndef CHESSENGINE_TRANSPOSITIONTABLE_H
#define CHESSENGINE_TRANSPOSITIONTABLE_H
#include <memory>

#include "Eval.h"
#include "ChessCore/Move.h"

namespace Engine {
    struct alignas(16) TTEntry {
        Key key{0};                   // 8 bytes (offset 0)
        Score score{0};               // 2 bytes (offset 8)
        int16_t depth{0};             // 2 bytes (offset 10)
        ChessCore::Move best_move{};  // 2 bytes (offset 12)
        Bound bound{Bound::EXACT};    // 1 byte  (offset 14)
        uint8_t age{0};               // 1 byte  (offset 15)
    };                                // Total: 16 bytes
    static_assert(sizeof(TTEntry) == 16);
    class TranspositionTable {
        constexpr static size_t NUM_BUCKETS = 1ull << 24; //1GB or 16,777,216 buckets
        constexpr static int BUCKET_SIZE = 4;
        static_assert(sizeof(TTEntry)*BUCKET_SIZE == 64);
        constexpr static Score MATE_THRESHOLD = Eval::MATE_SCORE -1000;

        constexpr static int AGE_PENALTY = 4;
        constexpr static int EXACT_BONUS = 2;

        constexpr static Value w_depth = 8;
        constexpr static Value w_bound = 2;
        constexpr static Value w_age = 8;

        std::unique_ptr<TTEntry[]> tt;
        uint8_t generation = 0;
        public:
        [[nodiscard]] int replacement_score(const TTEntry& e) const;
        TranspositionTable();
        TTEntry* operator [](Key key);
        void insert(Key key,Score score,int16_t depth ,ChessCore::Move best_move,Bound bound);
        void new_search() {
            ++generation;
        }
        static Score value_to_tt(const Score score, const int ply) {
            if (score > MATE_THRESHOLD)
                return score + ply;

            if (score < -MATE_THRESHOLD)
                return score - ply;

            return score;
        }

        static Score value_from_tt(const Score score, const int ply) {
            if (score > MATE_THRESHOLD)
                return score - ply;

            if (score < -MATE_THRESHOLD)
                return score + ply;

            return score;
        }
        void clear();
    };
} // Engine

#endif //CHESSENGINE_TRANSPOSITIONTABLE_H
