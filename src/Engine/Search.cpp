//
// Created by FloopDJBoy on 07/08/2026.
//

#include "Search.h"
#include "Eval.h"
namespace Engine {
    using namespace ChessCore;
    using Clock = std::chrono::steady_clock;
    using Duration = std::chrono::milliseconds;
    static int get_piece_value(PieceType pt) {
        using namespace ChessCore;
        switch(pt) {
            case PieceType::PAWN:   return 1;
            case PieceType::KNIGHT: return 2;
            case PieceType::BISHOP: return 3;
            case PieceType::ROOK:   return 4;
            case PieceType::QUEEN:  return 5;
            default: return 0; // King or Empty
        }
    }
    static int score_move(const Position& pos, const Move move, const Move pv_move) {
        // 1. Principal Variation Move
        if (move == pv_move) return 2000000;

        using namespace ChessCore;
        const MoveType type = move.get_type();

        // 2. Promotions
        if (type == MoveType::PROMOTION) {
            return 1000000 + get_piece_value(move.promotion_type()) * 10;
        }

        // 3. Captures (MVV-LVA)
        Piece captured = pos.square(move.to());
        if (captured != Pieces::EMPTY || type == MoveType::EN_PASSANT) {
            // En Passant captures a pawn. Otherwise, get the captured piece type.
            PieceType victim = (type == MoveType::EN_PASSANT)
                               ? PieceType::PAWN
                               : Pieces::getType(captured);

            PieceType attacker = Pieces::getType(pos.square(move.from()));

            // Formula: Victim * 10 - Attacker
            // e.g., PxQ = 50 - 1 = 49. QxP = 10 - 5 = 5.
            return 100000 + (get_piece_value(victim) * 10) - get_piece_value(attacker);
        }

        // 4. Quiet Moves
        return 0;
    }
    template <bool in_check>
    Score Search::quiesce(Score alpha, Score beta, SearchStack *ss) {
        if ((++nodes & 1023) == 0) {
            if (should_stop()) {
                stop_search();
            }
        }
        if (stop.load(std::memory_order_relaxed)) {
            return 0;
        }
        // Draw by threefold repetition
        if (pos.is_3fold()) {
            return 0;
        }
        if constexpr (!in_check) {
            // Stand-pat
            const Score stand_pat = Eval::evaluate(pos);

            if (stand_pat >= beta)
                return stand_pat;

            if (stand_pat > alpha)
                alpha = stand_pat;
        }
        auto moves = pos.generate_moves<in_check ? MoveGen::GenType::EVASIONS : MoveGen::GenType::CAPTURES>();

        if (moves.empty()) {
            if constexpr (in_check)
                return static_cast<Score>(-Eval::MATE_SCORE + pos.ply());

            return alpha;
        }
        const int move_count = moves.size();
        int scores[256];
        Score best_score = in_check ? -Eval::INF : alpha;
        const Move pv_move = ss->pv.empty() ? Move::none() : ss->pv[0];
        ss->pv.clear();
        bool found_move = false;
        for (int i = 0; i < move_count; ++i) {
            scores[i] = score_move(pos, moves[i],pv_move);
        }
        for (int i = 0; i < move_count; ++i) {
            // Incremental Selection Sort: find the best move remaining in the list
            int best_idx = i;
            for (int j = i + 1; j < move_count; ++j) {
                if (scores[j] > scores[best_idx]) {
                    best_idx = j;
                }
            }
            std::swap(scores[i], scores[best_idx]);
            std::swap(moves[i], moves[best_idx]);

            const Move move = moves[i];
            if (pos.legal(move)) {
                found_move = true;
                pos.make_move(move);
                Score score;
                if (pos.in_check()) {
                    score = static_cast<Score>(-quiesce<true>(static_cast<Score>(-beta), static_cast<Score>(-alpha),ss + 1));
                }else {
                    score =  static_cast<Score>(-quiesce<false>(static_cast<Score>(-beta), static_cast<Score>(-alpha),ss + 1));
                }
                pos.undo_move();
                if (score > best_score) {
                    best_score = score;


                    if (score > alpha) {
                        alpha = score;
                        ss->pv.update(move, (ss + 1)->pv);
                    }
                }
                // Beta cutoff
                if (alpha >= beta) {
                    break;
                }
            }
        }
        if (!found_move) {
            if (pos.in_check()) {
                return static_cast<Score>(-Eval::MATE_SCORE + pos.ply());
            }
            return 0; // stalemate
        }
        return best_score;

    }

    Score Search::alpha_beta(Score alpha, const Score beta, const int depth, SearchStack* ss) {

        if ((++nodes & 1023) == 0) {
            if (should_stop()) {
                stop_search();
            }
        }
        if (stop.load(std::memory_order_relaxed)) {
            return 0;
        }
        // Draw by threefold repetition
        if (pos.is_3fold()) {
            return 0;
        }
        if (depth <= 0) {
            if (pos.in_check()) {
                return quiesce<true>(alpha,beta,ss+1);
            }
            return quiesce<false>(alpha,beta,ss+1);
        }
        const Key zobrist_key = pos.zobrist_key();
        const auto tt_entry = (*tt)[zobrist_key];
        Move tt_move = Move::none();
        Score original_alpha = alpha;
        if (tt_entry) {
            if (tt_entry->depth >= depth) {
                switch (tt_entry->bound) {
                    case Bound::EXACT:
                        return tt_entry->score;

                    case Bound::LOWER:
                        if (tt_entry->score >= beta)
                            return tt_entry->score;
                        break;

                    case Bound::UPPER:
                        if (tt_entry->score <= alpha)
                            return tt_entry->score;
                        break;
                }
                // Even if the entry can't cause a cutoff, its move is useful.
                if (tt_entry->best_move != ChessCore::Move::none()) {
                    tt_move = tt_entry->best_move;
                }
            }
        }
        const Move pv_move = ss->pv.empty() ? Move::none() : ss->pv[0];
        ss->pv.clear();
        Score best_score = -Eval::INF;
        Move best_move = Move::none();
        bool found_move = false;

        auto moves = pos.legal_moves();
        const int move_count = moves.size();
        int scores[256];

        for (int i = 0; i < move_count; ++i) {
            scores[i] = score_move(pos, moves[i], pv_move);
        }

        for (int i = 0; i < move_count; ++i) {
            // Incremental Selection Sort: find the best move remaining in the list
            int best_idx = i;
            for (int j = i + 1; j < move_count; ++j) {
                if (scores[j] > scores[best_idx]) {
                    best_idx = j;
                }
            }

            // Bring the best move and its score to the front
            std::swap(scores[i], scores[best_idx]);
            std::swap(moves[i], moves[best_idx]);

            const Move move = moves[i];
            found_move = true;

            // Make move and evaluate child
            pos.make_move(move);
            const auto score = static_cast<Score>(-alpha_beta(static_cast<Score>(-beta), static_cast<Score>(-alpha), depth - 1, ss + 1));
            pos.undo_move();

            if (score > best_score) {
                best_score = score;
                best_move = move;


                if (score > alpha) {
                    alpha = score;
                    ss->pv.update(move, (ss + 1)->pv);
                }
            }

            // Beta cutoff
            if (alpha >= beta) {
                break;
            }
        }
        Bound bound;
        if (best_score <= original_alpha)
            bound = Bound::UPPER;
        else if (best_score >= beta)
            bound = Bound::LOWER;
        else
            bound = Bound::EXACT;
        tt->insert(pos.zobrist_key(), best_score, depth, best_move, bound);
        if (!found_move) {
            if (pos.in_check()) {
                return static_cast<Score>(-Eval::MATE_SCORE + pos.ply());
            }
            return best_score;  // quiet position, stand-pat
        }
        return best_score;
    }

    Duration Search::calculate_time_limit() const {
        if (limits.movetime_ms > 0) {
            return Duration{
                std::max(1, limits.movetime_ms - search_time_margin)
            };
        }

        int time_ms;
        int increment_ms;

        if (pos.side_to_move() == Color::WHITE) {
            time_ms = limits.wtime_ms;
            increment_ms = limits.winc_ms;
        } else {
            time_ms = limits.btime_ms;
            increment_ms = limits.binc_ms;
        }

        const int allocated_ms = time_ms / 20 + increment_ms / 2;

        return Duration{
            std::max(1, allocated_ms - search_time_margin)
        };
    }

    bool Search::should_stop() const {
        if (stop.load(std::memory_order_relaxed)) {
            return true;
        }
        if (limits.nodes && nodes>= limits.nodes) {
            return true;
        }
        if (Clock::now() >= search_deadline) {
            return true;
        }
        return false;

    }
    Move Search::search(const int depth, SearchStack* ss) {
        Score best_score = -Eval::INF;
        Move best_move = Move::none();
        Score alpha = best_score;
        constexpr Score beta = Eval::INF;

        ss->pv.clear();

        for (const auto move : pos.legal_moves()) {
            pos.make_move(move);
            Score score = -alpha_beta(-beta, -alpha, depth - 1, ss + 1);
            pos.undo_move();

            if (stop.load(std::memory_order_relaxed)) break; // Don't record unfinished PVs when time runs out

            if (score > best_score) {
                best_score = score;
                best_move = move;
                if (score > alpha) {
                    alpha = score;
                    // Update root PV
                    ss->pv.update(move, (ss + 1)->pv);
                }
            }
        }
        return best_move;
    }

    Move Search::find_best_move() {
        nodes = 0;
        stop = false;
        Move best_move = Move::none();
        tt->new_search();
        search_deadline = Clock::now() + calculate_time_limit();
        for (int depth =1; ;++depth) {
            SearchStack* ss = stack; // Start at the root of the stack array
            const Move m = search(depth,ss);
            if (stop.load(std::memory_order_relaxed)) {
                return best_move;
            }
            best_move = m;
            if (limits.depth && depth >= limits.depth) {
                return best_move;
            }
        }
    }
} // Engine