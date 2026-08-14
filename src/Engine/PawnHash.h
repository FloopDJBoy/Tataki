//
// Created by FloopDJBoy on 13/08/2026.
//

#ifndef CHESSENGINE_PAWNHASH_H
#define CHESSENGINE_PAWNHASH_H
#include <array>

#include "Types.h"

namespace Engine::PawnHash {
    namespace detail {
        struct PawnTable {
            std::array<std::array<Key,64>,2> squares ;
        };
        consteval auto make_table() {
            SplitMix64 rng(0x123456789ABCDEF0ULL);
            PawnTable table{};
            for (const int c : {0, 1}) {
                for (Square sq = 0; sq < 64; ++sq) {
                    table.squares[c][sq] = rng.next();
                }
            }
            return table;
        }
        inline PawnTable table = make_table();
    }
    inline Key hash(const Color side,const Square square) {
        return detail::table.squares[color_idx(side)][square];
    }
} // Engine

#endif //CHESSENGINE_PAWNHASH_H
