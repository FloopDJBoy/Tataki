//
// Created by FloopDJBoy on 07/08/2026.
//

#include "Eval.h"

#include "PawnTT.h"
#include "ChessCore/Pieces.h"
#include "ChessCore/Position.h"
namespace Engine::Eval {
    using namespace ChessCore;
    

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
    ScorePair static evaluate_pawn_structure(const BitBoard pawns_w, const BitBoard pawns_b) {
        ScorePair score;
        // --- 1. DOUBLED ---
        const int doubled_w = BitBoards::count_doubled(pawns_w);
        const int doubled_b = BitBoards::count_doubled(pawns_b);
        score += DOUBLED_PAWN_PENALTY * static_cast<int16_t>(doubled_w);
        score -= DOUBLED_PAWN_PENALTY * static_cast<int16_t>(doubled_b);

        // --- 2. CONNECTED ---
        // Phalanx (side-by-side)
        const BitBoard phalanx_w = pawns_w & ((pawns_w << 1 & ~ChessCore::BitBoards::FILE_A) | (pawns_w >> 1 & ~ChessCore::BitBoards::FILE_H));
        const BitBoard phalanx_b = pawns_b & ((pawns_b << 1 & ~ChessCore::BitBoards::FILE_A) | (pawns_b >> 1 & ~ChessCore::BitBoards::FILE_H));

        // Supported (defended by a friendly pawn). get_pawns_attacks for White shifts UP, so it shows squares White defends
        const BitBoard supported_w = pawns_w & ChessCore::BitBoards::get_pawns_attacks<Color::WHITE>(pawns_w);
        const BitBoard supported_b = pawns_b & ChessCore::BitBoards::get_pawns_attacks<Color::BLACK>(pawns_b);

        const BitBoard connected_w = phalanx_w | supported_w;
        const BitBoard connected_b = phalanx_b | supported_b;

        score += CONNECTED_PAWN_BONUS * static_cast<int16_t>(std::popcount(connected_w));
        score -= CONNECTED_PAWN_BONUS * static_cast<int16_t>(std::popcount(connected_b));

        // --- 3. ISLANDS & ISOLATED ---
        const BitBoard fw = BitBoards::file_bits(pawns_w);
        const BitBoard fb = BitBoards::file_bits(pawns_b);

        const BitBoard lone_w = BitBoards::lone_files(fw);
        const BitBoard lone_b = BitBoards::lone_files(fb);

        // Islands spanning two or more files. One-file islands are charged by the isolated term instead.
        const int groups_w = std::popcount(fw & ~(fw << 1)) - std::popcount(lone_w);
        const int groups_b = std::popcount(fb & ~(fb << 1)) - std::popcount(lone_b);

        if (groups_w > 1) score += PAWN_ISLAND_PENALTY * static_cast<int16_t>(groups_w - 1);
        if (groups_b > 1) score -= PAWN_ISLAND_PENALTY * static_cast<int16_t>(groups_b - 1);

        const BitBoard isolated_w = pawns_w & BitBoards::fill_files(lone_w);
        const BitBoard isolated_b = pawns_b & BitBoards::fill_files(lone_b);

        score += ISOLATED_PAWN_PENALTY * static_cast<int16_t>(std::popcount(isolated_w));
        score -= ISOLATED_PAWN_PENALTY * static_cast<int16_t>(std::popcount(isolated_b));

        // --- 5. PASSED, BACKWARD & ADVANCEMENT
        BitBoard w_pawns = pawns_w;
        while (w_pawns) {
            const Square sq = ChessCore::BitBoards::pop_lsb(w_pawns);
            const int r = ChessCore::BitBoards::rank_of(sq);

            // Passed
            if ((ChessCore::BitBoards::PAWN_MASKS.passed_masks[0][sq] & pawns_b) == 0) {
                score += PASSED_PAWN_BONUS[r];
            }

            // Backward: Not isolated, no friendly pawns beside/behind it, and stop square attacked by enemy
            else if ((ChessCore::BitBoards::make_bitboard(sq) & isolated_w) == 0 &&
                     (ChessCore::BitBoards::PAWN_MASKS.backward_masks[0][sq] & pawns_w) == 0) {

                const Square stop_sq = sq + 8; // Square in front of white pawn
                if (ChessCore::BitBoards::get_single_pawn_attacks<Color::WHITE>(stop_sq) & pawns_b) {
                    score += BACKWARD_PAWN_PENALTY;
                }
            }

            // Connected Advancement
            if (ChessCore::BitBoards::make_bitboard(sq) & connected_w) {
                score += CONNECTED_ADVANCEMENT_BONUS[r];
            }
        }
        BitBoard b_pawns = pawns_b;
        while (b_pawns) {
            const Square sq = ChessCore::BitBoards::pop_lsb(b_pawns); //[cite: 6]
            const int r = ChessCore::BitBoards::rank_of(sq); //[cite: 6]

            // Passed (Remember to flip the rank index for Black so rank 1 equals rank 6 from black's POV)
            if ((ChessCore::BitBoards::PAWN_MASKS.passed_masks[1][sq] & pawns_w) == 0) {
                score -= PASSED_PAWN_BONUS[7 - r];
            }
            // Backward
            else if ((ChessCore::BitBoards::make_bitboard(sq) & isolated_b) == 0 &&
                     (ChessCore::BitBoards::PAWN_MASKS.backward_masks[1][sq] & pawns_b) == 0) {
                const Square stop_sq = sq - 8; // Square in front of black pawn
                if (ChessCore::BitBoards::get_single_pawn_attacks<Color::BLACK>(stop_sq) & pawns_w) { //[cite: 6]
                    score -= BACKWARD_PAWN_PENALTY;
                }
            }


            // Connected Advancement
            if (ChessCore::BitBoards::make_bitboard(sq) & connected_b) {
                score -= CONNECTED_ADVANCEMENT_BONUS[7 - r];
            }
        }
        return score;
    }
    template <PieceType Pt>
    static ScorePair evaluate_mobility(const ChessCore::Position& pos, const Color us, const BitBoard mobility_area) {
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
    // Largest amount the mobility term could possibly move the (white - black)
    // score, derived from mobility_bonus itself so it stays correct if the table changes.
    static constexpr ScorePair mobility_max_swing() {
        int32_t max_mg = 0, min_mg = 0, max_eg = 0, min_eg = 0;
        for (const auto& piece_table : mobility_bonus) {
            int16_t p_max_mg = piece_table[0].mg, p_min_mg = piece_table[0].mg;
            int16_t p_max_eg = piece_table[0].eg, p_min_eg = piece_table[0].eg;
            for (const auto& sp : piece_table) {
                p_max_mg = std::max(p_max_mg, sp.mg);
                p_min_mg = std::min(p_min_mg, sp.mg);
                p_max_eg = std::max(p_max_eg, sp.eg);
                p_min_eg = std::min(p_min_eg, sp.eg);
            }
            max_mg += p_max_mg; min_mg += p_min_mg;
            max_eg += p_max_eg; min_eg += p_min_eg;
        }
        return {static_cast<int16_t>(max_mg - min_mg), static_cast<int16_t>(max_eg - min_eg)};
    }
    constexpr ScorePair MOBILITY_SWING = mobility_max_swing();
    // +2 covers integer-division truncation during mg/eg interpolation.
    constexpr Score LAZY_EVAL_MARGIN = 715; //98.5% coverage. made using LazyTuning::run_lazy_tuning(edp_file)
    Score evaluate(const Position& pos) {
        return evaluate(pos,nullptr,NEG_INF, INF); // no window context -> always full eval
    }
    Score evaluate(const Position& pos,PawnTT* pawn_tt,const Score alpha, const Score beta) {
        const int mg_weight = std::clamp(static_cast<int>(pos.state().phase), 0, Eval::MAX_PHASE);
        const Key pawn_key = pos.pawn_key();

        const auto& mat = pos.state().material_score;
        const auto& mat_w = mat[color_idx(Color::WHITE)];
        const auto& mat_b = mat[color_idx(Color::BLACK)];

        // Cheap: material_score is already tracked incrementally in make_move/undo_move.
        const int  lazy_w = interpolate(mat_w,mg_weight);
        const int  lazy_b = interpolate(mat_b,mg_weight);
        const int tempo = interpolate(TEMPO_BONUS,mg_weight);
        const int w_tempo = pos.side_to_move() == Color::WHITE? tempo : -tempo;
        ScorePair pawn_score{0,0};
        int final_pawn = 0;
        const PawnEntry* pawn_entry= nullptr;
        if (pawn_tt) {
            pawn_entry = (*pawn_tt)[pawn_key];
            if (pawn_entry) {
                pawn_score = pawn_entry->score;
                final_pawn = interpolate(pawn_score,mg_weight);
            }
        }
        const int lazy_diff = lazy_w - lazy_b + final_pawn + w_tempo;

        const auto lazy_eval = static_cast<Score>(pos.side_to_move() == Color::WHITE ? lazy_diff : -lazy_diff);

        if (lazy_eval >= static_cast<int>(beta) + LAZY_EVAL_MARGIN ||
            lazy_eval <= static_cast<int>(alpha) - LAZY_EVAL_MARGIN) {
            return lazy_eval;
        }

        // Expensive: only reached when the position is actually close to the window.
        const BitBoard occ = pos.all_bb();
        const BitBoard pawns_w = pos.piece_bb(PieceType::PAWN, Color::WHITE);
        const BitBoard pawns_b = pos.piece_bb(PieceType::PAWN, Color::BLACK);
        const BitBoard pawn_attacks_w = BitBoards::get_pawns_attacks<Color::WHITE>(pawns_w);
        const BitBoard pawn_attacks_b = BitBoards::get_pawns_attacks<Color::BLACK>(pawns_b);
        const BitBoard blocked_pawns_w = pawns_w & (occ >> 8);
        const BitBoard blocked_pawns_b = pawns_b & (occ << 8);
        const BitBoard area_w = ~(pos.color_bb(Color::WHITE) | pawn_attacks_b | blocked_pawns_w);
        const BitBoard area_b = ~(pos.color_bb(Color::BLACK) | pawn_attacks_w | blocked_pawns_b);

        auto eval_mob = [&](const Color c, const BitBoard area) {
            return evaluate_mobility<PieceType::KNIGHT>(pos, c, area) +
                   evaluate_mobility<PieceType::BISHOP>(pos, c, area) +
                   evaluate_mobility<PieceType::ROOK>(pos, c, area) +
                   evaluate_mobility<PieceType::QUEEN>(pos, c, area);
        };

        auto score = mat;
        score[color_idx(Color::WHITE)] += eval_mob(Color::WHITE, area_w);
        score[color_idx(Color::BLACK)] += eval_mob(Color::BLACK, area_b);

        const auto score_w = score[color_idx(Color::WHITE)];
        const auto score_b = score[color_idx(Color::BLACK)];

        const int final_w = interpolate(score_w,mg_weight);
        const int final_b = interpolate(score_b,mg_weight);
        if (!pawn_entry) {
            pawn_score = evaluate_pawn_structure(pos.piece_bb(Pieces::WHITE_PAWN),pos.piece_bb(Pieces::BLACK_PAWN));
            final_pawn = interpolate(pawn_score,mg_weight);
            if (pawn_tt) {
                pawn_tt->insert(pawn_key,pawn_score);
            }
        }
        const int evaluation = (final_w - final_b) + final_pawn + w_tempo;
        return static_cast<Score>(pos.side_to_move() == Color::WHITE ? evaluation : -evaluation);
    }
} // Engine