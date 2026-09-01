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

    inline int interpolate(const ScorePair score,const int mg_weight) {
        const int eg_weight = MAX_PHASE - mg_weight;
        return (score.mg * mg_weight + score.eg * eg_weight) / Eval::MAX_PHASE;
    }


    constexpr ScorePair mobility_bonus[4][28] = {
        // KNIGHT
        {
            ScorePair(26, 71), ScorePair(41, 143), ScorePair(46, 181), ScorePair(49, 202), ScorePair(52, 215),
            ScorePair(55, 227), ScorePair(60, 226), ScorePair(69, 216), ScorePair(82, 193), ScorePair(0, 0),
            ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0),
            ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0),
            ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0),
            ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0)
          },
          // BISHOP
          {
              ScorePair(43, 98), ScorePair(47, 139), ScorePair(55, 166), ScorePair(61, 188), ScorePair(68, 210),
              ScorePair(72, 228), ScorePair(74, 238), ScorePair(74, 243), ScorePair(77, 249), ScorePair(83, 246),
              ScorePair(93, 241), ScorePair(115, 230), ScorePair(124, 243), ScorePair(124, 243), ScorePair(0, 0),
              ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0),
              ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0),
              ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0)
            },
            // ROOK
            {
                ScorePair(28, 290), ScorePair(36, 320), ScorePair(42, 324), ScorePair(46, 336), ScorePair(47, 350),
                ScorePair(55, 356), ScorePair(60, 367), ScorePair(69, 369), ScorePair(77, 375), ScorePair(84, 381),
                ScorePair(91, 386), ScorePair(98, 390), ScorePair(106, 392), ScorePair(125, 379), ScorePair(125, 379),
                ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0),
                ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0),
                ScorePair(0, 0), ScorePair(0, 0), ScorePair(0, 0)
              },
              // QUEEN
              {
                  ScorePair(170, 328), ScorePair(170, 328), ScorePair(168, 464), ScorePair(171, 504), ScorePair(173, 538),
                  ScorePair(176, 559), ScorePair(179, 578), ScorePair(181, 597), ScorePair(185, 609), ScorePair(187, 619),
                  ScorePair(189, 628), ScorePair(192, 635), ScorePair(194, 641), ScorePair(195, 648), ScorePair(195, 655),
                  ScorePair(194, 660), ScorePair(194, 663), ScorePair(195, 661), ScorePair(200, 659), ScorePair(208, 655),
                  ScorePair(217, 648), ScorePair(232, 635), ScorePair(241, 624), ScorePair(241, 624), ScorePair(241, 624),
                  ScorePair(241, 624), ScorePair(241, 624), ScorePair(241, 624)
                },
    };
    // Base penalties
    constexpr ScorePair PAWN_ISLAND_PENALTY = ScorePair(-16, 10);
    constexpr ScorePair DOUBLED_PAWN_PENALTY = ScorePair(-1, -28);
    constexpr ScorePair CONNECTED_PAWN_BONUS = ScorePair(13, 7);
    constexpr ScorePair ISOLATED_PAWN_PENALTY = ScorePair(-15, -12);
    constexpr ScorePair BACKWARD_PAWN_PENALTY = ScorePair(-3, -26);

    // Rank-based bonuses (Index is the Rank 0-7)
    // Passed pawns get exponentially stronger as they advance.
    constexpr std::array<ScorePair, 8> PASSED_PAWN_BONUS = {
        ScorePair(0, 0), ScorePair(-5, 14), ScorePair(-5, 19), ScorePair(0, 51),
        ScorePair(32, 83), ScorePair(50, 149), ScorePair(72, 150), ScorePair(0, 0)
    };
    // Advancement alone is a weakness; connected advancement is a strength.
    constexpr std::array<ScorePair, 8> CONNECTED_ADVANCEMENT_BONUS = {
        ScorePair(0, 0), ScorePair(-12, -10), ScorePair(5, 1), ScorePair(3, 6),
        ScorePair(7, 21), ScorePair(28, 63),
        ScorePair(60, 73),
        ScorePair(0, 0)
    };
    constexpr ScorePair TEMPO_BONUS = ScorePair(13, 18);
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
            // PAWN
            .mg_value = 102,

            .eg_value = 174,
            .mg_pst = {
                0, 0, 0, 0, 0, 0, 0, 0,
                -25, -30, -33, -24, -27, 13, 27, -2,
                -37, -44, -39, -34, -23, -23, -6, -15,
                -28, -32, -25, -22, -12, -14, -6, -17,
                -26, -21, -27, -7, 10, 14, 0, -9,
                -21, -6, 17, 17, 34, 96, 39, 2,
                102, 100, 88, 110, 101, 59, -101, -86,
                0, 0, 0, 0, 0, 0, 0, 0

            },
            .eg_pst = {
                0, 0, 0, 0, 0, 0, 0, 0,
                -2, -4, -2, -5, 12, 2, -15, -38,
                -9, -12, -15, -16, -9, -11, -26, -32,
                -2, -7, -26, -32, -30, -23, -24, -25,
                16, -1, -16, -49, -44, -36, -19, -18,
                58, 42, 3, -42, -54, -29, 0, 15,
                103, 91, 71, 15, 7, 30, 97, 109,
                0, 0, 0, 0, 0, 0, 0, 0
            }
        },

        // index 2: knight
        {
            // KNIGHT
            .mg_value = 349,

            .eg_value = 349,
            .mg_pst = {
                -116, -22, -49, -15, -9, -9, -19, -93,
                -47, -35, -13, 5, 2, 2, -14, -13,
                -29, -5, 2, 17, 24, 13, 9, -9,
                -10, 5, 18, 23, 31, 24, 48, 11,
                5, 17, 41, 56, 38, 65, 33, 50,
                -28, 20, 52, 66, 117, 134, 70, 48,
                -17, -13, 37, 72, 78, 113, -12, 34,
                37, 37, 37, 37, 37, 37, 37, 37

            },
            .eg_pst = {
                -47, -59, -25, -6, -10, -22, -42, -44,
                -28, -5, -22, -5, 0, -28, -9, -15,
                -48, -11, 1, 31, 26, 0, -8, -28,
                -8, 13, 47, 54, 55, 52, 21, 11,
                -10, 16, 46, 64, 65, 54, 38, 5,
                -19, 4, 42, 41, 25, 32, 5, -21,
                -32, -8, -9, 19, 7, -34, -5, -39,
                -72, -7, 24, -3, -9, 28, -10, -100
            }
        },

        // index 3: bishop
        {
            // BISHOP
            .mg_value = 378,
            .eg_value = 379,
            .mg_pst = {
                14, 19, -7, -17, -18, -6, 2, 6,
                11, 12, 20, -1, 5, 11, 28, 16,
                -5, 13, 8, 14, 10, 13, 9, 15,
                -10, 4, 12, 28, 32, 1, 6, 6,
                -20, 17, 18, 58, 35, 40, 15, 0,
                -6, 10, 36, 34, 58, 73, 56, 25,
                -50, -4, -10, -28, -4, 19, -13, 8,
                -10, -10, -10, -10, -10, -10, -10, -10
            },
            .eg_pst = {
                -21, -11, -15, -13, -9, -9, -20, -23,
                -15, -25, -19, -4, -7, -23, -20, -44,
                -12, -2, 6, 8, 13, -1, -9, -11,
                -15, -2, 15, 19, 13, 13, -1, -15,
                -3, 7, 7, 14, 24, 7, 19, 5,
                -2, 10, 6, 3, 1, 17, 8, 5,
                2, 5, 8, 11, 8, 0, 9, -13,
                14, 22, 22, 29, 15, 16, -8, 6
            }
        },

        // index 4: rook
        {
            // ROOK
            .mg_value = 521,
            .eg_value = 640,
            .mg_pst = {
                -40, -28, -16, -6, -10, -16, 1, -33,
                -85, -48, -36, -31, -32, -20, -7, -69,
                -64, -49, -52, -41, -41, -38, 0, -37,
                -57, -53, -47, -36, -37, -36, -3, -30,
                -36, -18, -7, 22, 11, 21, 36, 14,
                -27, 24, 16, 45, 76, 98, 134, 54,
                -6, -17, 19, 46, 41, 90, 38, 76,
                28, 30, 6, 25, 40, 72, 84, 90
            },
            .eg_pst = {
                -34, -26, -22, -27, -28, -21, -34, -51,
                -26, -33, -24, -27, -29, -38, -42, -28,
                -23, -8, -7, -11, -10, -13, -25, -31,
                1, 18, 20, 14, 11, 13, 1, -6,
                16, 19, 23, 15, 15, 9, 0, 4,
                26, 11, 25, 10, -2, 2, -24, 0,
                29, 40, 36, 33, 34, -2, 11, -1,
                25, 30, 41, 32, 29, 24, 19, 17
            }
        },

        // index 5: queen
        {
            // QUEEN
            .mg_value = 1098,
            .eg_value = 1188,
            .mg_pst = {
                -1, 4, 9, 13, 16, -13, 3, -4,
                -5, -3, 6, 6, 6, 22, 26, 19,
                -12, -3, -4, -9, -6, -3, 14, 13,
                -14, -14, -13, -19, -19, -3, 3, 15,
                -21, -23, -25, -24, -19, -1, 16, 21,
                -33, -27, -32, -11, 16, 67, 83, 44,
                -37, -80, -32, -58, -36, 27, -50, 38,
                -44, -28, -18, 4, 18, 96, 76, 50
            },
            .eg_pst = {
                -94, -108, -108, -80, -112, -101, -134, -99,
                -91, -80, -94, -71, -80, -133, -153, -123,
                -72, -49, -23, -33, -30, -11, -42, -58,
                -55, -7, -5, 45, 44, 42, 21, 31,
                -38, 11, 16, 66, 96, 108, 95, 68,
                -14, -2, 40, 56, 93, 103, 68, 89,
                9, 59, 48, 102, 131, 98, 118, 64,
                26, 43, 58, 57, 70, 37, 32, 44
            }
        },

        // index 6: king
        {
            // KING
            .mg_value = 9991,
            .eg_value = 10008,
            .mg_pst = {
                -6, 42, 1, -117, -37, -102, 20, 26, //1
                9, -28, -60, -118, -91, -88, -13, 9, //2
                -52, -41, -61, -86, -69, -65, -32, -60, //3
                -46, -2, 6, -59, -31, -27, -1, -94, //4
                -40, -40, -40, -40, -40, -40, -40, -40,     // 5
                 -60, -60, -60, -60, -60, -60, -60, -60,     // 6
                 -80, -80, -80, -80, -80, -80, -80, -80,     // 7
                -100,-100,-100,-100,-100,-100,-100,-100      // 8
            },
            .eg_pst = {
                -83, -50, -29, -25, -66, -14, -54, -117,
                -22, -3, 13, 25, 19, 19, -14, -46,
                -15, 12, 31, 51, 45, 31, 3, -12,
                -21, 25, 46, 68, 60, 47, 23, -4,
                6, 39, 55, 63, 60, 51, 44, 11,
                10, 44, 40, 43, 31, 49, 55, 11,
                -36, 30, 18, 7, 18, 24, 52, -26,
                -199, -94, -65, -23, -44, -34, -37, -170
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
    constexpr Score NO_SCORE = INF + 2;
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

    constexpr int phase_value(PieceType p) {
        return phase_arr[static_cast<int>(p)];
    }
    Score evaluate(const ChessCore::Position& pos);
    Score evaluate(const ChessCore::Position& pos,PawnTT* pawn_tt,Score alpha,Score beta);
    inline ScorePair evaluate_piece(const Piece piece, const Square s){
        return PSQT[piece][s];
    }
    constexpr int RULE50_DAMP = 212;

    static int damp(const int v, const int half_clock) {
        return v - v * std::min(half_clock, 100) / RULE50_DAMP;
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
