//
// Created by FloopDJBoy on 07/08/2026.
//

#include "Search.h"

#include <iostream>

#include "Eval.h"
namespace Engine {
    using namespace ChessCore;
    using Clock = std::chrono::steady_clock;
    using Duration = std::chrono::milliseconds;
    static auto start_time = Clock::now();
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
    static int score_move(const Position& pos, const Move move, const Move tt_move) {
        // 1. Transposition Table / Hash Move
        if (move == tt_move && tt_move != Move::none()) return 2000000;

        const MoveType type = move.get_type();

        // 2. Promotions
        if (type == MoveType::PROMOTION) {
            return 1000000 + get_piece_value(move.promotion_type()) * 10;
        }

        // 3. Captures (MVV-LVA)
        Piece captured = pos.square(move.to());
        if (captured != Pieces::EMPTY || type == MoveType::EN_PASSANT) {
            PieceType victim = (type == MoveType::EN_PASSANT)
                               ? PieceType::PAWN
                               : Pieces::getType(captured);

            PieceType attacker = Pieces::getType(pos.square(move.from()));

            return 100000 + (get_piece_value(victim) * 10) - get_piece_value(attacker);
        }

        // 4. Quiet Moves
        return 0;
    }
   template <bool in_check>
    Score Search::quiesce(Score alpha, Score beta, SearchStack *ss) {
        assert(ss >= stack);
        assert(ss < stack + std::size(stack));
        ss->pv.clear();

        if ((++nodes & 1023) == 0 && should_stop()) {
            stop_search();
        }
        if (stop.load(std::memory_order_relaxed)) {
            return 0;
        }

        if (pos.ply() >= PV::MAX_PLY - 1)
            return Eval::evaluate(pos);

        if (pos.is_draw()) {
            return 0;
        }
        Score best_score;

        if constexpr (!in_check) {
            const Score stand_pat = Eval::evaluate(pos,alpha,beta);

            if (stand_pat >= beta)
                return stand_pat;

            if (stand_pat > alpha)
                alpha = stand_pat;

            best_score = stand_pat;
        } else {
            best_score = -Eval::INF;
        }

        auto moves = pos.generate_moves<in_check ? MoveGen::GenType::EVASIONS : MoveGen::GenType::CAPTURES>();

        if (moves.empty()) {
            if constexpr (in_check)
                return static_cast<Score>(-Eval::MATE_SCORE + pos.ply());

            return best_score;
        }

        const int move_count = moves.size();
        int scores[256];
        bool found_move = false;

        for (int i = 0; i < move_count; ++i) {
            scores[i] = score_move(pos, moves[i], Move::none());
        }

        for (int i = 0; i < move_count; ++i) {
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

                Score score = pos.in_check()
                    ? static_cast<Score>(-quiesce<true>(-beta, -alpha, ss + 1))
                    : static_cast<Score>(-quiesce<false>(-beta, -alpha, ss + 1));

                pos.undo_move();
                if (stop.load(std::memory_order_relaxed)) {
                    return 0;
                }

                if (score > best_score) {
                    best_score = score;

                    if (score > alpha) {
                        alpha = score;
                        ss->pv.update(move, (ss + 1)->pv);
                    }
                }

                if (alpha >= beta) {
                    break;
                }
            }
        }

        if constexpr (in_check) {
            if (!found_move) {
                return static_cast<Score>(-Eval::MATE_SCORE + pos.ply());
            }
        }

        return best_score;
    }

    Score Search::alpha_beta(Score alpha, const Score beta, const int depth, SearchStack* ss) {
        ss->pv.clear();
        if ((++nodes & 1023) == 0 && should_stop()) {
            stop_search();
        }
        if (stop.load(std::memory_order_relaxed)) {
            return 0;
        }

        if (pos.ply() >= PV::MAX_PLY - 1)
            return Eval::evaluate(pos);

        if (pos.is_draw()) {
            return 0;
        }

        if (depth <= 0) {
            return pos.in_check() ? quiesce<true>(alpha, beta, ss) : quiesce<false>(alpha, beta, ss);
        }



        const Key zobrist_key = pos.zobrist_key();
        const auto tt_entry = tt[zobrist_key];
        Move tt_move = Move::none();
        const Score original_alpha = alpha;

        if (tt_entry) {
            // Extract TT move regardless of depth for move ordering
            if (tt_entry->best_move != ChessCore::Move::none()) {
                tt_move = tt_entry->best_move;
            }

            if (tt_entry->depth >= depth) {
                const Score tt_score = TranspositionTable::value_from_tt(tt_entry->score, pos.ply());
                switch (tt_entry->bound) {
                    case Bound::EXACT:
                        if (tt_move != Move::none()) {
                            ss->pv.update(tt_move);
                        }
                        return tt_score;

                    case Bound::LOWER:
                        if (tt_score >= beta) {
                            if (tt_move != Move::none()) {
                                ss->pv.update(tt_move);
                            }
                            return tt_score;
                        }
                        break;

                    case Bound::UPPER:
                        if (tt_score <= alpha)
                            return tt_score;
                        break;
                }
            }
        }

        Score best_score = -Eval::INF;
        Move best_move = Move::none();
        bool found_move = false;

        auto moves = pos.legal_moves();
        const int move_count = moves.size();
        std::array<int, 256> scores{};

        for (int i = 0; i < move_count; ++i) {
            scores[i] = score_move(pos, moves[i], tt_move);
        }

        for (int i = 0; i < move_count; ++i) {
            int best_idx = i;
            for (int j = i + 1; j < move_count; ++j) {
                if (scores[j] > scores[best_idx]) {
                    best_idx = j;
                }
            }

            std::swap(scores[i], scores[best_idx]);
            std::swap(moves[i], moves[best_idx]);

            const Move move = moves[i];
            found_move = true;

            pos.make_move(move);
            const auto score = static_cast<Score>(-alpha_beta(-beta, -alpha, depth - 1, ss + 1));
            pos.undo_move();

            if (stop.load(std::memory_order_relaxed)) {
                return 0;
            }

            if (score > best_score) {
                best_score = score;
                best_move = move;

                if (score > alpha) {
                    alpha = score;
                    ss->pv.update(move, (ss + 1)->pv);
                }
            }

            if (alpha >= beta) {
                break;
            }
        }

        if (!found_move) {
            if (pos.in_check()) {
                return static_cast<Score>(-Eval::MATE_SCORE + pos.ply());
            }
            return 0; // Stalemate
        }

        if (!stop.load(std::memory_order_relaxed)) {
            Bound bound;
            if (best_score <= original_alpha)
                bound = Bound::UPPER;
            else if (best_score >= beta)
                bound = Bound::LOWER;
            else
                bound = Bound::EXACT;

            tt.insert(zobrist_key, TranspositionTable::value_to_tt(best_score, pos.ply()), depth, best_move, bound);
        }
        return best_score;
    }

    Duration Search::calculate_time_limit() const {
        if (limits.movetime_ms > 0) {
            // Deduct a scaled margin up to 50ms
            int margin = std::min(20, limits.movetime_ms / 10);
            return Duration{ std::max(1, limits.movetime_ms - margin) };
        }

        int time_ms = (pos.side_to_move() == Color::WHITE) ? limits.wtime_ms : limits.btime_ms;
        int inc_ms  = (pos.side_to_move() == Color::WHITE) ? limits.winc_ms  : limits.binc_ms;

        // FIX: If no time limit set (e.g. infinite search or fixed depth), return max duration
        if (time_ms == 0 && inc_ms == 0) {
            return Duration::max();
        }

        // Use moves_to_go if specified, otherwise assume ~30 moves remaining
        int moves = limits.moves_to_go > 0 ? limits.moves_to_go : 30;
        int allocated_ms = (time_ms / moves) + (inc_ms * 3 / 4);

        // Dynamic margin calculation that prevents negative/near-zero limits on low time
        int margin = std::min(50, allocated_ms / 5);
        int target_ms = std::max(10, allocated_ms - margin);

        // Never exceed remaining total time minus safety buffer
        target_ms = std::min(target_ms, std::max(1, time_ms - 50));

        return Duration{ target_ms };
    }

    bool Search::should_stop() const {
        if (stop.load(std::memory_order_relaxed)) {
            return true;
        }
        if (limits.nodes && nodes >= limits.nodes) {
            return true;
        }
        const auto now = Clock::now();
        if (search_deadline != std::chrono::time_point<Clock>::max() && Clock::now() >= search_deadline) {
            std::cerr << "STOP: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     now - start_time
                 ).count()
              << " ms\n";

            return true;
        }
        return false;
    }
    SearchResult Search::search(const int depth, SearchStack* ss) {
        Score best_score = -Eval::INF;
        Move best_move = Move::none();
        Score alpha = -Eval::INF;
        constexpr Score beta = Eval::INF;

        ss->pv.clear();
        auto moves = pos.legal_moves();
        const int move_count = moves.size();

        if (move_count > 0) {
            best_move = moves[0]; // Legal move fallback
        }

        const auto tt_entry = tt[pos.zobrist_key()];
        Move tt_move = tt_entry ? tt_entry->best_move : Move::none();

        std::array<int, 256> scores{};
        for (int i = 0; i < move_count; ++i) {
            scores[i] = score_move(pos, moves[i], tt_move);
        }

        for (int i = 0; i < move_count; ++i) {
            if (stop.load(std::memory_order_relaxed)) break;

            int best_idx = i;
            for (int j = i + 1; j < move_count; ++j) {
                if (scores[j] > scores[best_idx]) {
                    best_idx = j;
                }
            }
            std::swap(scores[i], scores[best_idx]);
            std::swap(moves[i], moves[best_idx]);

            const Move move = moves[i];
            pos.make_move(move);
            Score score = -alpha_beta(-beta, -alpha, depth - 1, ss + 1);
            pos.undo_move();

            if (stop.load(std::memory_order_relaxed)) break;

            if (score > best_score) {
                best_score = score;
                best_move = move;
                if (score > alpha) {
                    alpha = score;
                    ss->pv.update(move, (ss + 1)->pv);
                }
            }
        }
        return { .move = best_move, .score = best_score };
    }


    Move Search::find_best_move() {
        nodes = 0;
        Move best_move = Move::none();

        // Fallback to first legal move to prevent returning Move::none() on instant stops
        auto root_moves = pos.legal_moves();
        if (!root_moves.empty()) {
            best_move = root_moves[0];
        }

        tt.new_search();
        start_time = Clock::now();
        const auto duration_limit = calculate_time_limit();

        search_deadline = (duration_limit == Duration::max())
            ? std::chrono::time_point<Clock>::max()
            : Clock::now() + duration_limit;

        for (int depth = 1; ; ++depth) {
            // FIX: Check time before starting a new depth
            if (should_stop()) {
                stop_search();
                break;
            }

            SearchStack* ss = stack;

            const auto [move, score] = search(depth, ss);
            const auto elapsed = std::chrono::duration_cast<Duration>(Clock::now() - start_time);

            const auto time_ms = std::max<int64_t>(1, elapsed.count());
            const auto nps = nodes * 1000 / time_ms;

            if (stop.load(std::memory_order_relaxed)) {
                std::cerr << "return STOP: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     Clock::now() - start_time
                 ).count()
              << " ms\n";
                return best_move;
            }

            if (move != Move::none()) {
                best_move = move;
            }

            std::cout
                << "info depth "
                << depth
                << " score cp "
                << score
                << " time "
                << time_ms
                << " nodes "
                << nodes
                << " nps "
                << nps
                << " pv ";
            for (const auto& pv_move : ss->pv) {
                std::cout << pv_move.to_string() << ' ';
            }
            std::cout << std::endl;

            if (limits.depth && depth >= limits.depth) {
                return best_move;
            }
        }
        std::cerr << "return STOP: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     Clock::now() - start_time
                 ).count()
              << " ms\n";
        return best_move;
    }
} // Engine