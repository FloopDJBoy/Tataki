//
// Created by FloopDJBoy on 07/08/2026.
//

#include "Eval.h"

#include "ChessCore/Pieces.h"
#include "ChessCore/Position.h"
namespace Engine::Eval {
    using namespace ChessCore;
    namespace {

    }
    static constexpr Square mirror(const Square s) {
        return s ^ 56;
    }
    Value pst_value(const Piece piece,const Square s, const bool eg) {
        const bool should_mirror = Pieces::getColor(piece) != Color::WHITE;
        PieceType type = Pieces::getType(piece);
        const Square relative_s = should_mirror ? mirror(s) : s;
        const auto& eval = default_params.piece_eval[static_cast<int>(type)];
        return eg ? eval.eg_pst[relative_s] : eval.mg_pst[relative_s];
    }
    Value material_value(const PieceType p, const bool eg) {
        const auto& eval = default_params.piece_eval[static_cast<int>(p)];
        return eg ? eval.eg_value : eval.mg_value;
    }
    template <PieceType Pt>
    static ScorePair evaluate_mobility(const Position& pos, const Color us, const BitBoard mobility_area) {
        ScorePair score;
        BitBoard pieces = pos.piece_bb(Pt,us);

        while (pieces) {
            Square sq = BitBoards::pop_lsb(pieces);

            // 1. Calculate attacks for this piece given current occupancy
            const BitBoard attacks = BitBoards::get_attacks_bb<Pt>(sq, pos.all_bb());

            // 2. Count squares that fall inside the restricted Mobility Area
            const int mob = std::popcount(attacks & mobility_area);

            // 3. Accumulate score from lookup table
            score += mobility_bonus[static_cast<int>(Pt) - static_cast<int>(PieceType::KNIGHT)][mob];
        }

        return score;
    }
    Score evaluate(const Position& pos) {
    // 1. Setup Base Boards (Assuming standard A1=0 mapping)
    const BitBoard occ = pos.all_bb();
    const BitBoard pawns_w = pos.piece_bb(PieceType::PAWN, Color::WHITE);
    const BitBoard pawns_b = pos.piece_bb(PieceType::PAWN, Color::BLACK);

    // 2. Compute Pawn Attacks (Branchless)
    const BitBoard pawn_attacks_w = BitBoards::get_pawns_attacks<Color::WHITE>(pawns_w);
    const BitBoard pawn_attacks_b = BitBoards::get_pawns_attacks<Color::BLACK>(pawns_b);

    // 3. Compute Blocked Pawns
    // A White pawn is blocked if there is ANY piece on the square directly above it (+8)
    // We shift the occupancy board down by 8 to overlap with the pawn's square.
    const BitBoard blocked_pawns_w = pawns_w & (occ >> 8);
    const BitBoard blocked_pawns_b = pawns_b & (occ << 8);

    // 4. Compute Mobility Areas
    const BitBoard area_w = ~(pos.color_bb(Color::WHITE) | pawn_attacks_b | blocked_pawns_w);
    const BitBoard area_b = ~(pos.color_bb(Color::BLACK) | pawn_attacks_w | blocked_pawns_b);

    // 5. Helper lambda to keep mobility accumulation DRY
    auto eval_mob = [&](const Color c, const BitBoard area) {
        return evaluate_mobility<PieceType::KNIGHT>(pos, c, area) +
               evaluate_mobility<PieceType::BISHOP>(pos, c, area) +
               evaluate_mobility<PieceType::ROOK>(pos, c, area) +
               evaluate_mobility<PieceType::QUEEN>(pos, c, area);
    };

    // 6. Accumulate scores statically by color
    auto score = pos.state().material_score;
    score[color_idx(Color::WHITE)] += eval_mob(Color::WHITE, area_w);
    score[color_idx(Color::BLACK)] += eval_mob(Color::BLACK, area_b);

    // 7. Phase Interpolation
    const int mg_weight = std::clamp(static_cast<int>(pos.state().phase), 0, Eval::MAX_PHASE);
    const int eg_weight = Eval::MAX_PHASE - mg_weight;

    const auto score_w = score[color_idx(Color::WHITE)];
    const auto score_b = score[color_idx(Color::BLACK)];

    const int final_w = (score_w.mg * mg_weight + score_w.eg * eg_weight) / Eval::MAX_PHASE;
    const int final_b = (score_b.mg * mg_weight + score_b.eg * eg_weight) / Eval::MAX_PHASE;

    // 8. Calculate absolute advantage and flip if Black to move
    const int evaluation = final_w - final_b;
    return static_cast<Score>(pos.side_to_move() == Color::WHITE ? evaluation : -evaluation);
}
} // Engine