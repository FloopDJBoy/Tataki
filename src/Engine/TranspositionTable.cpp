//
// Created by FloopDJBoy on 10/08/2026.
//

#include "TranspositionTable.h"

#include <algorithm>
#include <limits>

namespace Engine {
    TranspositionTable::TranspositionTable(const size_t megabytes) :
        NUM_BUCKETS((megabytes * BYTES_PER_MB) /(sizeof(TTEntry) * BUCKET_SIZE)),
        tt(std::make_unique<TTEntry[]>(NUM_BUCKETS * BUCKET_SIZE))
    {}

    int TranspositionTable::replacement_score(const TTEntry &e) const {
        // Unsigned 8-bit underflow naturally handles generation wrap-around
        const uint8_t age_diff = generation - e.age;

        int quality = e.depth - (age_diff * AGE_PENALTY);
        if (e.bound == Bound::EXACT) {
            quality += EXACT_BONUS;
        }
        return quality;
    }
    void TranspositionTable::insert(const Key key, const Score score, const int16_t depth,
                                    const ChessCore::Move best_move, const Bound bound) {
        TTEntry* bucket = &tt[(key & (NUM_BUCKETS - 1)) * BUCKET_SIZE];

        TTEntry* replace_candidate = nullptr;
        int min_quality = std::numeric_limits<int>::max();

        for (int i = 0; i < BUCKET_SIZE; ++i) {
            TTEntry& entry = bucket[i];

            // 1. Same position: overwrite and preserve best move if new move is empty
            if (entry.key == key) {
                const ChessCore::Move move_to_store = (best_move != ChessCore::Move{})
                                                ? best_move
                                                : entry.best_move;

                // Overwrite if new search is deeper, or from a new search, or exact bound
                entry = TTEntry{.key = key, .score = score, .depth = depth, .best_move = move_to_store, .bound = bound, .age = generation};
                return;
            }

            // 2. Empty slot: use immediately
            if (entry.key == 0) {
                entry = TTEntry{.key = key, .score = score, .depth = depth, .best_move = best_move, .bound = bound, .age = generation};
                return;
            }

            // 3. Find the entry in the bucket with the lowest quality score
            int quality = replacement_score(entry);
            if (quality < min_quality) {
                min_quality = quality;
                replace_candidate = &entry;
            }
        }

        // Replace the least valuable entry in the bucket
        if (replace_candidate) {
            *replace_candidate = TTEntry{.key = key, .score = score, .depth = depth, .best_move = best_move, .bound = bound, .age = generation};
        }
    }
} // Engine