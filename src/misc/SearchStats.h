//
// Created by FloopDJBoy on 15/08/2026.
//

#ifndef CHESSENGINE_SEARCHSTATS_H
#define CHESSENGINE_SEARCHSTATS_H
#include <cstdint>

namespace Engine {
    struct SearchStats {
        uint64_t nodes = 0;
        uint64_t qnodes = 0;

        // TT
        uint64_t tt_probes = 0;
        uint64_t tt_hits = 0;
        uint64_t tt_depth_sufficient = 0;
        uint64_t tt_cutoffs = 0;

        uint64_t tt_exact_cutoffs = 0;
        uint64_t tt_lower_cutoffs = 0;
        uint64_t tt_upper_cutoffs = 0;

        // Search
        uint64_t beta_cutoffs = 0;
        uint64_t fail_high = 0;
        uint64_t fail_low = 0;

        // Evaluation
        uint64_t eval_calls = 0;

        // Move generation
        uint64_t movegen_calls = 0;
        uint64_t legal_moves = 0;
        uint64_t qmovegen_calls = 0;
        uint64_t qmoves_generated = 0;

        void reset() {
            *this = {};
        }


    };
} // Engine

#endif //CHESSENGINE_SEARCHSTATS_H
