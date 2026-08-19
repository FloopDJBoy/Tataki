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
        BAD_CAPTURES,
        GEN_QUIETS,
        QUIETS,

        // generate evasion moves
        EVASION_TT,
        GEN_EVASION,
        EVASION,

        // generate qsearch moves
        QSEARCH_TT,
        GEN_QCAPTURE,
        QCAPTURE

    };

    struct ScoredMove : public ChessCore::Move {
        int score;
        ScoredMove& operator=(const Move move){data = move.raw();return *this;}
    };

    class MovePicker {
        constexpr static int MAX_MOVES = 256;
        const ChessCore::Position& pos;
        ChessCore::Move tt_move;
        const History::CaptureHistory& capture_history;
        const int depth;

        PickStage stage;

        ScoredMove *cur,*end,*end_bad_captures,*end_captures;

        ScoredMove moves[MAX_MOVES];

        template<ChessCore::MoveGen::GenType Type>
        ScoredMove* score(const ChessCore::MoveGen::MoveList<Type>& movelist);
        template <typename Filter>
        requires std::convertible_to<std::invoke_result_t<Filter&>, bool>
        ChessCore::Move select(Filter&& filter);
        static void partial_insertion_sort(ScoredMove* begin, const ScoredMove* end, int limit);

    public:
        // Constructor for Main Search
        MovePicker(const ChessCore::Position& p, ChessCore::Move tt,int depth,const History::CaptureHistory& ch);


        ChessCore::Move next_move();
    };

} // namespace Engine