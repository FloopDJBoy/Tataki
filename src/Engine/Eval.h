//
// Created by FloopDJBoy on 07/08/2026.
//

#ifndef CHESSENGINE_EVAL_H
#define CHESSENGINE_EVAL_H
#include "magic_enum.hpp"
#include "PawnTT.h"
#include "Types.h"
#include "ChessCore/Pieces.h"


namespace ChessCore {
    class Position;
}

namespace Engine::Eval {
    constexpr int PAWN_PHASE =   0;
    constexpr int KNIGHT_PHASE = 1;
    constexpr int BISHOP_PHASE = 1;
    constexpr int ROOK_PHASE   = 2;
    constexpr int QUEEN_PHASE  = 4;
    constexpr int KING_PHASE =   0;
    constexpr inline std::array phase_arr = {0,PAWN_PHASE,KNIGHT_PHASE,BISHOP_PHASE,ROOK_PHASE,QUEEN_PHASE,KING_PHASE};

    constexpr int MAX_PHASE =
        4 * KNIGHT_PHASE +
        4 * BISHOP_PHASE +
        4 * ROOK_PHASE +
        2 * QUEEN_PHASE;


    constexpr ScorePair mobility_bonus[4][28] = {
        // [0] KNIGHT (9 active elements: 0..8)
        {
            ScorePair(-62,-79), ScorePair(-53,-57), ScorePair(-12,-31), ScorePair( -3,-17), ScorePair( 3, 7),
            ScorePair( 12, 13), ScorePair( 21, 16), ScorePair( 28, 21), ScorePair( 37, 26)
            // Indices 9..27 auto-fill with ScorePair(0, 0)
        },
        // [1] BISHOP (14 active elements: 0..13)
    {
            ScorePair(-48,-59), ScorePair(-20,-23), ScorePair( 16, -3), ScorePair( 26, 13), ScorePair( 38, 24),
            ScorePair( 51, 42), ScorePair( 55, 54), ScorePair( 63, 57), ScorePair( 63, 65), ScorePair( 68, 73),
            ScorePair( 81, 78), ScorePair( 81, 86), ScorePair( 91, 88), ScorePair( 98, 97)
            // Indices 14..27 auto-fill with ScorePair(0, 0)
        },

        // [2] ROOK (15 active elements: 0..14)
        {
            ScorePair(-60,-82), ScorePair(-20,-15), ScorePair( 15, 17), ScorePair( 34, 43), ScorePair( 34, 66),
            ScorePair( 36, 86), ScorePair( 37,100), ScorePair( 40,111), ScorePair( 42,121), ScorePair( 47,126),
            ScorePair( 56,128), ScorePair( 56,132), ScorePair( 57,133), ScorePair( 58,133), ScorePair( 62,134)
            // Indices 15..27 auto-fill with ScorePair(0, 0)
        },

        // [3] QUEEN (28 elements: 0..27)
        {
            ScorePair(-39,-36), ScorePair(-21,-15), ScorePair(  3,  8), ScorePair(  3, 18), ScorePair( 14, 34),
            ScorePair( 22, 54), ScorePair( 27, 61), ScorePair( 31, 73), ScorePair( 40, 79), ScorePair( 40, 92),
            ScorePair( 41, 94), ScorePair( 48,104), ScorePair( 50,113), ScorePair( 56,120), ScorePair( 61,123),
            ScorePair( 61,123), ScorePair( 65,128), ScorePair( 65,128), ScorePair( 67,130), ScorePair( 67,130),
            ScorePair( 67,130), ScorePair( 67,130), ScorePair( 67,130), ScorePair( 67,130), ScorePair( 67,130),
            ScorePair( 67,130), ScorePair( 67,130), ScorePair( 67,130)
        }
    };
    // Base penalties
    constexpr ScorePair ISOLATED_PAWN_PENALTY = ScorePair(-15, -25);
    constexpr ScorePair DOUBLED_PAWN_PENALTY  = ScorePair(-11, -20);
    constexpr ScorePair BACKWARD_PAWN_PENALTY = ScorePair(-9, -24);
    constexpr ScorePair PAWN_ISLAND_PENALTY   = ScorePair(-25, -35); // Applied per island after the 1st
    constexpr ScorePair CONNECTED_PAWN_BONUS  = ScorePair( 7,   8);  // Flat bonus per connected pawn

    // Rank-based bonuses (Index is the Rank 0-7)
    // Passed pawns get exponentially stronger as they advance.
    constexpr std::array<ScorePair, 8> PASSED_PAWN_BONUS = {
        ScorePair( 0,   0), ScorePair( 5,  10), ScorePair(10,  25), ScorePair(20,  45),
        ScorePair(35,  75), ScorePair(60, 120), ScorePair(90, 170), ScorePair( 0,   0)
    };

    // Advancement alone is a weakness; connected advancement is a strength.
    constexpr std::array<ScorePair, 8> CONNECTED_ADVANCEMENT_BONUS = {
        ScorePair( 0,  0), ScorePair( 0,  0), ScorePair( 2,  4), ScorePair( 4,  8),
        ScorePair(10, 16), ScorePair(20, 32), ScorePair(30, 48), ScorePair( 0,  0)
    };
    using PieceSquareTable = std::array<Score, 64>;
    struct PieceEval {
        Value mg_value;
        Value eg_value;

        PieceSquareTable mg_pst;
        PieceSquareTable eg_pst;
    };
    struct EvaluationParams {
        std::array<PieceEval,7> piece_eval;
    };
    constexpr static EvaluationParams default_params = {
    .piece_eval = {{
        // index 0: none
        {},

        // index 1: pawn
        {
            .mg_value = 82,
            .eg_value = 94,

            .mg_pst = {
                 0,   0,   0,   0,   0,   0,   0,   0, // Rank 1
               -35,  -1, -20, -23, -15,  24,  38, -22, // Rank 2
               -26,  -4,  -4, -10,   3,   3,  33, -12, // Rank 3
               -27,  -2,  -5,  12,  17,   6,  10, -25, // Rank 4
               -14,   3,   6,  16,  12,   9,  22,  23, // Rank 5
                -4,  24,  38,  61,  51,  22,  -8,  -5, // Rank 6
                98, 134,  61,  95,  68, 126,  34, -11, // Rank 7
                 0,   0,   0,   0,   0,   0,   0,   0  // Rank 8
            },

            .eg_pst = {
                 0,   0,   0,   0,   0,   0,   0,   0, // Rank 1
                13,   8,   8,  10,  13,   0,   2,  -7, // Rank 2
                 4,   7,  -6,   1,   0,  -5,  -1,  -8, // Rank 3
                13,   9,  -3,  -7,  -7,  -8,   3,  -1, // Rank 4
                32,  24,  13,   5,  -2,   4,  17,  17, // Rank 5
                94, 100,  85,  67,  56,  53,  82,  84, // Rank 6
               178, 173, 158, 134, 147, 132, 165, 187, // Rank 7
                 0,   0,   0,   0,   0,   0,   0,   0  // Rank 8
            }
        },

        // index 2: knight
        {
            .mg_value = 337,
            .eg_value = 281,

            .mg_pst = {
               -53, -42, -31, -25, -18, -17, -30, -46, // Rank 1
               -44, -16, -20,  -9,  -1,  11, -17, -35, // Rank 2
               -22,   5,  19,  20,  19,  10,   6,  -2, // Rank 3
               -10,  12,  10,  11,  25,  15,  22,  -5, // Rank 4
               -13,   4,  16,  13,  28,  19,  21,  -8, // Rank 5
               -23,  -9,  12,  10,  19,  17,  25, -16, // Rank 6
               -29, -53, -12, -22, -20,  -5, -24, -38, // Rank 7
              -105, -21, -58, -33, -17, -28, -19, -23  // Rank 8
            },

            .eg_pst = {
               -64, -50, -23, -15, -22, -23, -51, -29,
               -41, -20, -10,  -5,  -2, -10, -20, -42,
               -24,  -3,  -5,   2,   4,   4,  -8, -20,
               -17,  -1,   2,   7,   7,   6,  -4, -17,
               -18,  -8,  -3,   2,   3,  -1, -10, -18,
               -23, -13,  -4,  -1,   3,   3, -16, -27,
               -42, -20, -10,  -5,  -2, -20, -23, -44,
               -29, -51, -23, -15, -22, -18, -50, -64
            }
        },

        // index 3: bishop
        {
            .mg_value = 365,
            .eg_value = 297,

            .mg_pst = {
               -23,  -9, -23,  -5,  -9, -16,  -5, -17, // Rank 1
               -17,  -1,  -2,  -8,  -5,  -2, -12, -17, // Rank 2
               -10,   5,   5,   7,   4,  -3, -10, -19, // Rank 3
               -16,   3,  -3,   9,   9,   7,  -4,  -7, // Rank 4
               -17,   0,  -1,   2,   8,   8,   6, -10, // Rank 5
                -3,  13,   7,  10,  11,   6,  11,  -7, // Rank 6
                 4,  15,  16,   0,   7,  21,  33,   1, // Rank 7
               -33,  -3, -14, -21, -13, -12, -39, -21  // Rank 8
            },

            .eg_pst = {
               -24, -17,  -9,  -7,  -8, -11, -21, -14,
               -12,  -3,   0,   2,   2,   0,  -3, -12,
                -4,   1,   5,   7,   7,   4,   0,  -5,
                -2,   3,   5,   8,   8,   5,   3,  -2,
                -2,   4,   6,   8,   7,   7,   4,  -3,
                -6,   2,   4,   2,   3,   2,  -3,  -8,
               -10,  -8,  -2,   2,  -2,  -1, -11, -18,
               -14, -21, -11,  -8,  -7,  -9, -17, -24
            }
        },

        // index 4: rook
        {
            .mg_value = 477,
            .eg_value = 512,

            .mg_pst = {
               -36, -26, -12,  -1,   9,  -7,   6, -23, // Rank 1
               -15,   8,   7, -37, -34, -38, -19, -25, // Rank 2
                -5,  -8,   7,  -6,   1, -15, -15, -21, // Rank 3
               -14, -14, -20, -10,  -5, -12, -18, -24, // Rank 4
               -24, -24,  -1,   4,   3, -16, -27, -25, // Rank 5
                -5,  19,  26,  36,  17,  45,  61,  -6, // Rank 6
                27,  32,  58,  62,  80,  67,  26,  44, // Rank 7 (Massive bonus for 7th rank!)
                32,  42,  32,  51,  63,   9,  31,  43  // Rank 8
            },

            .eg_pst = {
                 5,   1,   5,   3,   1,  -2,   2,   4,
                 4,   5,   7,  -6,   1,   0,  -3,   4,
                 4,   3,   6,   0,   2,  -1,  -5,  -2,
                 3,   5,   8,   4,  -1,  -5,  -5,  -1,
                 4,   3,   1,  -5,  -1,  -7,  -6,  -4,
                 7,   7,   7,   5,   4,  -3,  -5,  -3,
                11,  13,  13,  11,  -3,   3,   8,   3,
                13,  10,  18,  15,  12,  12,   8,   5
            }
        },

        // index 5: queen
        {
            .mg_value = 1025,
            .eg_value = 936,

            .mg_pst = {
                 2,  -1,   3,  -3,  -2,   2,   1,  -3, // Rank 1
                -9,   4,   3,  -2,  -1,   3,  -1,  -5, // Rank 2
                -1,   5,   6,   3,   2,  -1,   2,  -2, // Rank 3
                 1,  -2,  -1,   2,   2,  -2,  -1,   2, // Rank 4
                -2,  -1,   7,  -3,   2,  -1,  -5,  -1, // Rank 5
                -3,   5,   8,   5,   6,   2,  -5,  -6, // Rank 6
                 1,  -1,  12,   7,  -1,  -3,   3,  11, // Rank 7
                -2,  -8,   0,   9,   9,   3,  -4,  -5  // Rank 8
            },

            .eg_pst = {
                -9,   4,   3,  -5,  -5,  -3,  -3,  -9,
                -2,   1,   1,  -1,   0,   0,  -1,  -2,
                 1,   1,   2,   2,   2,   2,   1,   1,
                 0,   1,   2,   3,   3,   2,   1,   0,
                 0,   1,   2,   3,   3,   2,   1,   0,
                 1,   1,   2,   2,   2,   2,   1,   1,
                -2,   1,   1,  -1,   0,   0,  -1,  -2,
                -9,   4,   3,  -5,  -5,  -3,  -3,  -9
            }
        },

        // index 6: king
        {
            .mg_value = 10000,
            .eg_value = 10000,

            .mg_pst = {
               -65,  23,  16, -15, -56, -34,   2,  13, // Rank 1
                29,  -1, -20, -13, -27, -38, -33, -14, // Rank 2
                -9,  24,   2, -16, -20,   6,  22, -22, // Rank 3
               -17, -20, -12, -27, -30, -25, -14, -36, // Rank 4
               -49,  -1, -27, -39, -46, -44, -33, -51, // Rank 5
               -14, -14, -22, -46, -44, -30, -15, -27, // Rank 6
                 1,   7,  -8, -64, -43, -16,   9,   8, // Rank 7
               -15,  36,  12, -54,   8, -28,  24,  14  // Rank 8
            },

            .eg_pst = {
               -74, -35, -18, -18, -11,  15,   4, -17,
               -12,  17,  14,  17,  17,  38,  23,  11,
                10,  17,  23,  15,  20,  45,  44,  13,
                -8,  22,  24,  27,  26,  33,  26,   3,
               -18,  -4,  21,  24,  27,  23,   9, -11,
               -19,  -3,  11,  21,  23,  16,   7,  -9,
               -27, -11,   4,  13,  14,   4,  -5, -17,
               -53, -34, -21, -11, -28, -14, -24, -43
            }
        }
    }}
};
    constexpr std::array Capture_Piece_Value = {
        0,      // none
        208,     // pawn
        781,    // knight
        825,    // bishop
        1276,    // rook
        2538,    // queen
        0   // king
    };
    constexpr Score INF = 32000;
    constexpr Score MATE_SCORE = 30000;
    constexpr Score NEG_INF = -INF;
    constexpr static Score MATE_THRESHOLD = Eval::MATE_SCORE -1000;
    static constexpr auto make_PSQT() {
        std::array<std::array<ScorePair,64>,15> arr{};
        for (const auto c : magic_enum::enum_values<Color>()) {
            for (const auto p : magic_enum::enum_values<PieceType>()) {
                const auto piece = ChessCore::Pieces::makePiece(p,c);
                const auto& param = default_params.piece_eval[static_cast<int>(p)];
                const auto value_score =  ScorePair(param.mg_value,param.eg_value);
                for (Square s=0; s<64; ++s) {
                    const bool should_mirror = ChessCore::Pieces::getColor(piece) != Color::WHITE;
                    const Square ss = should_mirror ? s^56 : s;
                    const auto pst_score = ScorePair(param.mg_pst[ss],param.eg_pst[ss]);
                    arr[piece][s] = value_score + pst_score;
                }
            }
        }
        return arr;
    }
    inline constexpr auto PSQT = make_PSQT();
    inline constexpr int phase_value(PieceType p) {
        return phase_arr[static_cast<int>(p)];
    }
    Score evaluate(const ChessCore::Position& pos);
    Score evaluate(const ChessCore::Position& pos,PawnTT* pawn_tt,Score alpha,Score beta);
    inline ScorePair evaluate_piece(const Piece piece, const Square s){
        return PSQT[piece][s];
    }

    constexpr Value piece_value(const PieceType piece) {
        return Capture_Piece_Value[static_cast<int>(piece)];
    }
    constexpr Value piece_value(const Piece piece) {
        return piece_value(ChessCore::Pieces::getType(piece));
    }
    Value pst_value(Piece piece,Square s,bool eg);
    Value material_value(PieceType p,bool eg);
} // Engine

#endif //CHESSENGINE_EVAL_H
