//
// Created by FloopDJBoy on 16/08/2026.
//

#ifndef CHESSENGINE_HISTORY_H
#define CHESSENGINE_HISTORY_H
#include <algorithm>
#include <type_traits>

#include "Types.h"

namespace Engine::History {
    constexpr int CAPTURE_HISTORY_MAX = 10692;            //stockfish val don't question it
    constexpr int Capture_Stat_Victim_Numerator   = 873;  //stockfish val don't question it
    constexpr int Capture_Stat_Victim_Denominator = 128;  //stockfish val don't question it

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
    constexpr Score capture_stat_victim_value(const Value piece_value) {
        return History::Capture_Stat_Victim_Numerator * piece_value //// NOLINT
             / History::Capture_Stat_Victim_Denominator;
    }
    using CaptureHistoryEntry = StatsEntry<int16_t, CAPTURE_HISTORY_MAX>;
    //[piece][to][captured piece type]
    using CaptureHistory =std::array<std::array<std::array<CaptureHistoryEntry,PT_NUMBER>,SQUARE_NUMBER>,PIECE_NUMBER>;


} // Engine

#endif //CHESSENGINE_HISTORY_H
