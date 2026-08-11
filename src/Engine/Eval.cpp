//
// Created by FloopDJBoy on 07/08/2026.
//

#include "Eval.h"

#include "ChessCore/Pieces.h"
#include "ChessCore/Position.h"
namespace Engine::Eval {
    using namespace ChessCore;
    namespace {
        constexpr EvaluationParams default_params = {
            .piece_eval = {{
                // index 0
                {},

                // index 1: pawn
                {
                    .value = 100,
                    .pst = {
                        0, 0, 0, 0, 0, 0, 0, 0,
                        50, 50, 50, 50, 50, 50, 50, 50,
                        10, 10, 20, 30, 30, 20, 10, 10,
                        5, 5, 10, 25, 25, 10, 5, 5,
                        0, 0, 0, 20, 20, 0, 0, 0,
                        5, -5, -10, 0, 0, -10, -5, 5,
                        5, 10, 10, -20, -20, 10, 10, 5,
                        0, 0, 0, 0, 0, 0, 0, 0
                    }
                },

                // index 2: knight
                {
                    .value = 320,
                    .pst = {
                        -50,-40,-30,-30,-30,-30,-40,-50,
                        -40,-20,  0,  0,  0,  0,-20,-40,
                        -30,  0, 10, 15, 15, 10,  0,-30,
                        -30,  5, 15, 20, 20, 15,  5,-30,
                        -30,  0, 15, 20, 20, 15,  0,-30,
                        -30,  5, 10, 15, 15, 10,  5,-30,
                        -40,-20,  0,  5,  5,  0,-20,-40,
                        -50,-40,-30,-30,-30,-30,-40,-50
                    }
                },

                // index 3: bishop
                {
                    .value = 330,
                    .pst = {}
                },

                // index 4: rook
                {
                    .value = 500,
                    .pst = {}
                },

                // index 5: queen
                {
                    .value = 900,
                    .pst = {}
                },

                // index 6: king
                {
                    .value = 20000,
                    .pst = {}
                }
            }}
        };
    }
    static constexpr Square mirror(const Square s) {
        return s ^ 56;
    }
    Value pst_value(const Piece piece,const Square s) {
        const bool should_mirror = Pieces::getColor(piece) != Color::WHITE;
        PieceType type = Pieces::getType(piece);
        return default_params.piece_eval[static_cast<int>(type)].pst[should_mirror ? mirror(s) : s];
    }
    Value material_value(const PieceType p) {
        return  default_params.piece_eval[static_cast<int>(p)].value ;
    }
    Score evaluate(const Position& pos) {
        const int us = color_idx(pos.side_to_move());
        const int them = color_idx(~pos.side_to_move());
        return pos.state().material_score[us] - pos.state().material_score[them] + pos.state().pst_score[us] - pos.state().pst_score[them];
    }
} // Engine