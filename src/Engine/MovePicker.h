#pragma once

#include <array>
#include "ChessCore/MoveGen.h"
#include "ChessCore/Position.h"
#include "History.h"
#include "Types.h"

namespace Engine {

    enum PickStage {
        // Main Search Stages
        MAIN_TT,
        GEN_CAPTURES,
        GOOD_CAPTURES,
        GEN_QUIETS,
        GOOD_QUIETS,
        BAD_CAPTURES,
        BAD_QUIETS,

        // generate evasion moves
        EVASION_TT,
        GEN_EVASION,
        EVASION,

        // generate qsearch moves
        QSEARCH_TT,
        GEN_QCAPTURE,
        QCAPTURE

    };

    struct ScoredMove : ChessCore::Move {
        int score;
        ScoredMove& operator=(const Move move){data = move.raw();return *this;}
    };

    class MovePicker {
        constexpr static int GOOD_QUIET_THRESHOLD = -History::BUTTERFLY_MAX / 4;   // -1800
        constexpr static int quiet_sort_limit(const int depth) { return -600 * depth; }
        constexpr static int MAX_MOVES = 256;
        bool skip_quiets_ = false;
        const ChessCore::Position& pos;
        ChessCore::Move tt_move;
        const History::CaptureHistory& capture_history;
        const History::ButterflyHistory& butterfly_history;
        const std::array<ChessCore::Move,2>& killers;
        const int depth;

        PickStage stage;

        ScoredMove *cur, *end, *end_bad_captures, *begin_bad_quiets, *end_bad_quiets,*end_captures,*end_generated;

        ScoredMove moves[MAX_MOVES];

        template<ChessCore::MoveGen::GenType Type>
        ScoredMove* score(const ChessCore::MoveGen::MoveList<Type>& movelist);
        template <typename Filter>
        requires std::convertible_to<std::invoke_result_t<Filter&>, bool>
        ChessCore::Move select(Filter&& filter);
        static void partial_insertion_sort(ScoredMove* begin, const ScoredMove* end, int limit);

    public:
        MovePicker(const ChessCore::Position& p, ChessCore::Move tt,int depth,const History::CaptureHistory& ch,const History::ButterflyHistory& bh,const std::array<ChessCore::Move,2>& killer);
        ChessCore::Move next_move();
        void skip_quiets() {skip_quiets_ = true;}
    };

} // namespace Engine