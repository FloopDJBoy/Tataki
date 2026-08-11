//
// Created by FloopDJBoy on 07/08/2026.
//

#ifndef CHESSENGINE_EVAL_H
#define CHESSENGINE_EVAL_H
#include "Types.h"


namespace ChessCore {
    class Position;
}

namespace Engine::Eval {
    using PieceSquareTable = std::array<Score, 64>;
    struct PieceEval {
        Value value;
        PieceSquareTable pst;
    };

    struct EvaluationParams {
        std::array<PieceEval,7> piece_eval;
    };
    constexpr Score INF = 32000;
    constexpr Score MATE_SCORE = 30000;
    constexpr Score NEG_INF = -INF;
    Score evaluate(const ChessCore::Position& pos);
    Value pst_value(Piece piece,Square s);
    Value material_value(PieceType p);
} // Engine

#endif //CHESSENGINE_EVAL_H
