//
// Created by FloopDJBoy on 16/08/2026.
//

#ifndef CHESSENGINE_HISTORY_H
#define CHESSENGINE_HISTORY_H
#include <algorithm>
#include <type_traits>

#include "Types.h"

namespace Engine::History {
    constexpr int BUTTERFLY_MAX = 7200;
    constexpr int CAPTURE_MAX   = 10800;

    constexpr int BONUS_PER_DEPTH = 150;
    constexpr int BONUS_MAX       = 1500;   // caps at depth 10
    constexpr int MALUS_PER_DEPTH = 300;
    constexpr int MALUS_MAX       = 2250;   // caps at depth 8
    constexpr int BONUS_TT_MOVE   = 350;

    constexpr int stat_bonus(int depth) { return std::min(BONUS_PER_DEPTH * depth, BONUS_MAX); }
    constexpr int stat_malus(int depth) { return std::min(MALUS_PER_DEPTH * depth, MALUS_MAX); }



    template<typename T, int MAX_VALUE>
    struct StatsEntry {
        static_assert(std::is_integral_v<T> && std::is_signed_v<T>);

        T entry = 0;

        //implicit conversion to the value
        operator T() const {
            return entry;
        }
        StatsEntry& operator=(const T v) {
            entry = v;
            return *this;
        }

        void add(int bonus) {
            bonus = std::clamp(bonus, -MAX_VALUE, MAX_VALUE);

            T value = entry;
            entry = value
                  + bonus
                  - value * std::abs(bonus) / MAX_VALUE;
        }
    };
    using ButterflyEntry = StatsEntry<Score,7200>;
    using CaptureHistoryEntry = StatsEntry<int16_t, CAPTURE_MAX>;
    //[piece][to][captured piece type]
    using CaptureHistory =std::array<std::array<std::array<CaptureHistoryEntry,PT_NUMBER>,SQUARE_NUMBER>,PIECE_NUMBER>;
    //[color][from][to]
    using ButterflyHistory  = std::array<std::array<std::array<ButterflyEntry,SQUARE_NUMBER>,SQUARE_NUMBER>,COLOR_NUMBER>;


} // Engine

#endif //CHESSENGINE_HISTORY_H
