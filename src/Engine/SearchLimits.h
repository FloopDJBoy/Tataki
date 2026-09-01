//
// Created by FloopDJBoy on 07/08/2026.
//

#ifndef CHESSENGINE_SEARCHLIMITS_H
#define CHESSENGINE_SEARCHLIMITS_H
#include <cstdint>

namespace Engine {
    // 0 = unlimited
    struct SearchLimits {
        int depth = 0;              // go depth N
        int mate = 0;               // go mate N
        int movetime_ms = 0;        // go movetime N
        uint64_t nodes = 0;         // go nodes N
        uint64_t nodes_soft = 0;

        int wtime_ms = 0;
        int btime_ms = 0;
        int winc_ms = 0;
        int binc_ms = 0;
        int moves_to_go = 0;

        bool infinite = false;
        bool ponder = false;
    };
} // Engine

#endif //CHESSENGINE_SEARCHLIMITS_H
