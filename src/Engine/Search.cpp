//
// Created by FloopDJBoy on 07/08/2026.
//

#include "Search.h"

#include <iostream>
#include "Engine/TranspositionTable.h"
#include "Eval.h"
#include "MovePicker.h"



namespace Engine {
    using namespace ChessCore;
    using Clock = std::chrono::steady_clock;
    using Duration = std::chrono::milliseconds;
    static auto start_time = Clock::now();
    constexpr int QSEARCH_DEPTH = 0;
    template <bool in_check>
    Score Search::quiesce(Score alpha, Score beta, SearchStack *ss) {
        assert(ss >= stack);
        assert(ss < stack + std::size(stack));
#if DEBUG_STATS
        ++stats.qnodes;
#endif
        ss->pv.clear();
        ss->stat_score=0;

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
//        Move best_move = Move::none();
//         const Key zobrist_key = pos.zobrist_key();
//         const auto tt_entry = tt[zobrist_key];
         constexpr  Move tt_move = Move::none();
//         if (tt_entry) {
//             const Score tt_score =TranspositionTable::value_from_tt(tt_entry->score, pos.ply());
//
// #if DEBUG_STATS
//             stats.tt_hits++;
// #endif
//
//             // Always use TT move for move ordering, regardless of depth.
//             if (tt_entry->best_move != Move::none()) {
//                 tt_move = tt_entry->best_move;
//             }
//
//             if (tt_entry->depth >= QSEARCH_DEPTH) {
// #if DEBUG_STATS
//                 stats.tt_depth_sufficient++;
// #endif
//
//                 if (tt_entry->bound == (tt_score >= beta ? Bound::LOWER : Bound::UPPER))
//                 {
// #if DEBUG_STATS
//                     stats.tt_cutoffs++;
//                     if (tt_score >= beta)
//                         stats.tt_lower_cutoffs++;
//                     else
//                         stats.tt_upper_cutoffs++;
// #endif
//
//                     if (tt_move != Move::none()) {
//                         ss->pv.update(tt_move);
//                     }
//
//                     return tt_score;
//                 }
//             }
//         }
        if constexpr (!in_check) {
            const Score stand_pat = Eval::evaluate(pos,&pawn_tt,alpha,beta);
#if DEBUG_STATS
            ++stats.eval_calls;
#endif


            if (stand_pat >= beta) {
                //tt.insert(zobrist_key,TranspositionTable::value_to_tt(stand_pat, pos.ply()),QSEARCH_DEPTH,Move::none(),Bound::LOWER);
                return stand_pat;
            }

            if (stand_pat > alpha)
                alpha = stand_pat;

            best_score = stand_pat;
        } else {
            best_score = -Eval::INF;
        }

        MovePicker move_picker(pos,tt_move,QSEARCH_DEPTH,capture_history);

        bool found_move = false;

        for (Move move = move_picker.next_move();move != Move::none();move = move_picker.next_move()) {

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
                    //best_move = move;

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
        // if (!stop.load(std::memory_order_relaxed)) {
        //     const Bound bound = best_score >= beta ? Bound::LOWER : Bound::UPPER;
        //     //tt.insert(zobrist_key, TranspositionTable::value_to_tt(best_score, pos.ply()), QSEARCH_DEPTH, best_move, bound);
        // }
        return best_score;
    }

   void Search::update_stats(const Move best_move, const Move tt_move, const int depth,const DumbVector<Move,SEARCHED_LIST_CAPACITY> &captures_searched ,const SearchStack* ss,const bool PVNode) {
        //stockfish magic numbers I have no idea how to name them
        assert((ss-1)>=stack);
        //const int prior_stat_score = ss > stack ? (ss - 1)->stat_score : 0;
        const int bonus = std::min(133 * depth - 81, 1487) + 364 * (best_move == tt_move) + (ss-1)->stat_score;
        const int malus = std::min(968 * depth - 235, 2244);


        if (pos.is_capture(best_move)) {
            const auto moved = pos.square(best_move.from());
            const auto captured_type = best_move.get_type() == MoveType::EN_PASSANT ?
                                   PieceType::PAWN :
                                   Pieces::getType(pos.square(best_move.to()));
            capture_history[moved][best_move.to()][static_cast<int>(captured_type)].add(bonus * 1427 / 1024); //more stockfish magic numbers
        }
        for (Move move : captures_searched) {
            const auto moved = pos.square(move.from());
            const auto captured_type = move.get_type() == MoveType::EN_PASSANT ?
                                       PieceType::PAWN :
                                       Pieces::getType(pos.square(move.to()));
            capture_history[moved][move.to()][static_cast<int>(captured_type)].add(-malus* 1489 / 1024);//even more stockfish magic numbers
        }

   }
    template<NodeType node_type>
    Score Search::alpha_beta(Score alpha, const Score beta, const int depth, SearchStack* ss) {
        constexpr bool PvNode   = node_type != NodeType::NonPV;
        constexpr bool RootNode = node_type == NodeType::Root;

        ss->pv.clear();
        ss->stat_score = 0;


        if ((++nodes & 1023) == 0 && should_stop()) stop_search();
        if (stop.load(std::memory_order_relaxed)) return 0;

        if constexpr (!RootNode) {
            if (pos.ply() >= PV::MAX_PLY - 1) return Eval::evaluate(pos);
            if (pos.is_draw()) return 0;
        }

        if (depth <= 0) {
            return pos.in_check() ? quiesce<true>(alpha, beta, ss) : quiesce<false>(alpha, beta, ss);
        }

        DumbVector<Move,SEARCHED_LIST_CAPACITY> captured_searched;
        const Key zobrist_key = pos.zobrist_key();
        const Engine::TTEntry* tt_entry = tt[zobrist_key];
        Move tt_move = Move::none();
        const Score original_alpha = alpha;

        if (tt_entry) {
            ss->ttPv = PvNode || tt_entry->is_pv();
            if (tt_entry->best_move != Move::none()) tt_move = tt_entry->best_move;

            if constexpr (!RootNode) {
                if (tt_entry->depth >= depth) {
                    const Score tt_score = TranspositionTable::value_from_tt(tt_entry->score, pos.ply());
                    switch (tt_entry->bound()) {
                        case Bound::EXACT:
                            if (tt_move != Move::none()) ss->pv.update(tt_move);
                            return tt_score;
                        case Bound::LOWER:
                            if (tt_score >= beta) {
                                if (tt_move != Move::none()) ss->pv.update(tt_move);
                                return tt_score;
                            }
                            break;
                        case Bound::UPPER:
                            if (tt_score <= alpha) return tt_score;
                            break;
                    }
                }
            }
        }

        constexpr int   RFP_MAX_DEPTH           = 8;   // free parameter
        constexpr Score RFP_MARGIN_PER_DEPTH    = 80;  // free parameter
        constexpr Score RFP_IMPROVING_REDUCTION = 60;  // free parameter



        ss->static_eval = pos.in_check()
            ? (pos.ply() >= 2 ? (ss - 2)->static_eval : 0)
            : Eval::evaluate(pos);
        const bool improving = pos.ply() >= 2 && ss->static_eval > (ss - 2)->static_eval;
        //RFP
        if (!pos.in_check()
            && depth <= RFP_MAX_DEPTH
            && (tt_move == Move::none() || pos.is_capture(tt_move))
            && beta > -Eval::MATE_THRESHOLD          // beta isn't already a near-mate loosing score
            && ss->static_eval < Eval::MATE_THRESHOLD // eval isn't already a near-mate winning score
        ) {


            Score margin = RFP_MARGIN_PER_DEPTH * depth;
            if (improving) margin -= RFP_IMPROVING_REDUCTION;

            if (ss->static_eval - margin >= beta)
                return ss->static_eval; // fail-soft; the (eval+beta)/2-style tweaks are a later tuning knob
        }

        if constexpr (!PvNode) {
            constexpr int   NMP_MIN_DEPTH           = 3;
            constexpr Score NMP_BASE_MARGIN         = 150;  // flat, dominant term
            constexpr Score NMP_MARGIN_PER_DEPTH    = 10;
            constexpr Score NMP_IMPROVING_REDUCTION = 25;
            constexpr int   R                       = 3;

            if (!pos.in_check()
                && depth >= NMP_MIN_DEPTH
                && pos.previous().move != Move::null()
                && beta > -Eval::MATE_THRESHOLD
                && ss->static_eval < Eval::MATE_THRESHOLD
                && ss->static_eval >= beta //- NMP_MARGIN_PER_DEPTH * depth - NMP_IMPROVING_REDUCTION * improving +NMP_BASE_MARGIN
                && pos.non_pawn_material(pos.side_to_move())) {

                pos.make_null_move();
                const auto null_value = static_cast<Score>(-alpha_beta<NodeType::NonPV>(-beta, -beta + 1, depth - 1 - R, ss + 1));
                pos.undo_null_move();

                if (stop.load(std::memory_order_relaxed)) return 0;

                if (null_value >= beta)
                    return null_value < Eval::MATE_THRESHOLD ? null_value : beta;
                }
        }

        Score best_score = -Eval::INF;
        Move best_move = Move::none();
        bool found_move = false;

        if constexpr (RootNode) {
            if (auto legal = pos.legal_moves(); !legal.empty())
                ss->pv.update(legal[0]);   // fallback if we're interrupted before improving alpha
        }

        MovePicker move_picker(pos, tt_move, depth, capture_history);
        int i = 0;
        for (Move move = move_picker.next_move(); move != Move::none(); move = move_picker.next_move()) {
            if (!pos.legal(move)) continue;
            ++i;
            found_move = true;
            const bool capture = pos.is_capture(move);
            Piece moved_piece = pos.square(move.from());
            tt.prefetch(pos.prefetch_key(move));
            pos.make_move(move);
            if (capture) {
                const int captured_type = static_cast<int>(Pieces::getType(pos.state().captured));
                const auto captured_value = Eval::piece_value(pos.state().captured);
                ss->stat_score = History::capture_stat_victim_value(captured_value)
                               + capture_history[moved_piece][move.to()][captured_type];
            }

            Score score;
            if constexpr (PvNode) {
                //PVS
                if (i == 1) {
                    score = static_cast<Score>(-alpha_beta<NodeType::PV>(-beta, -alpha, depth - 1, ss + 1));
                } else {
                    score = static_cast<Score>(-alpha_beta<NodeType::NonPV>(-alpha - 1, -alpha, depth - 1, ss + 1));
                    if (score > alpha && score < beta)
                        score = static_cast<Score>(-alpha_beta<NodeType::PV>(-beta, -alpha, depth - 1, ss + 1));
                }
            } else {
                score = static_cast<Score>(-alpha_beta<NodeType::NonPV>(-beta, -alpha, depth - 1, ss + 1));
            }
            pos.undo_move();

            if (stop.load(std::memory_order_relaxed)) return 0;

            if (score > best_score) {
                best_score = score;
                if (score > alpha) {
                    best_move = move;
                    alpha = score;
                    ss->pv.update(move, (ss + 1)->pv);
                }
            }
            if (capture && move != best_move && i <= SEARCHED_LIST_CAPACITY)
                captured_searched.push_back(move);

            if (alpha >= beta) break;
        }

        if constexpr (!RootNode) {
            if (!found_move) {
                if (pos.in_check()) return static_cast<Score>(-Eval::MATE_SCORE + pos.ply());
                return 0;
            }
        }

        if (!stop.load(std::memory_order_relaxed)) {
            Bound bound = (best_score <= original_alpha) ? Bound::UPPER
                         : (best_score >= beta)           ? Bound::LOWER
                                                           : Bound::EXACT;
            if constexpr (!RootNode) {
                if (best_move != Move::none() && best_score > original_alpha)
                    update_stats(best_move, tt_move, depth, captured_searched, ss,PvNode);
            }
            tt.insert(zobrist_key, TranspositionTable::value_to_tt(best_score, pos.ply()), depth, best_move, bound,PvNode);
        }

        return best_score;
    }

    Duration Search::calculate_soft_limit() const {
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

    Duration Search::calculate_hard_limit() const {
        if (limits.movetime_ms > 0) return calculate_soft_limit(); // movetime is already authoritative

        const int time_ms = (pos.side_to_move() == Color::WHITE) ? limits.wtime_ms : limits.btime_ms;
        if (time_ms == 0) return Duration::max();

        constexpr int HARD_LIMIT_MULTIPLIER = 4; // placeholder
        const auto soft = calculate_soft_limit();
        if (soft == Duration::max()) return soft;

        return Duration{ std::min<int64_t>(soft.count() * HARD_LIMIT_MULTIPLIER, time_ms - 50) };
    }

    bool Search::should_stop() const {
        if (stop.load(std::memory_order_relaxed)) {
            return true;
        }
        if (limits.nodes && nodes >= limits.nodes) {
            return true;
        }
        if (search_deadline != std::chrono::time_point<Clock>::max() && Clock::now() >= search_deadline) {
            return true;
        }
        return false;
    }
    SearchResult Search::search(const int depth, SearchStack* ss) {
        const Score score = alpha_beta<NodeType::Root>(-Eval::INF, Eval::INF, depth, ss);
        const Move move = ss->pv.empty() ? Move::none() : ss->pv[0];
        return { .move = move, .score = score };
    }
    Move Search::find_best_move() {

        nodes = 0;
        Move best_move = Move::none(),last_best_move = Move::none();

        // Fallback to first legal move to prevent returning Move::none() on instant stops
        auto root_moves = pos.legal_moves();

        if (!root_moves.empty()) {
            best_move = last_best_move = root_moves[0];
        }
        //forced move no need to think;
        if (root_moves.size() == 1 && !limits.infinite && limits.depth == 0 && limits.nodes == 0) {
            return root_moves[0];
        }

        tt.new_search();
        start_time = Clock::now();
        const auto soft_limit = calculate_soft_limit();
        search_deadline = (calculate_hard_limit() == Duration::max())
            ? std::chrono::time_point<Clock>::max()
            : Clock::now() + calculate_hard_limit();

        int stability = 0;

        constexpr std::array STABILITY_SCALE = {1.30, 1.15, 1.00, 0.90, 0.80}; // placeholder table, indexed by min(stability,4)

        for (int depth = 1; ; ++depth) {
            if (should_stop()) {
                stop_search();
                break;
            }

            SearchStack* ss = stack;
            assert(ss->stat_score == 0);
            const auto [move, score] = search(depth, ss);
            const auto elapsed = std::chrono::duration_cast<Duration>(Clock::now() - start_time);

            const auto time_ms = std::max<int64_t>(1, elapsed.count());
            const auto nps = nodes * 1000 / time_ms;

            if (stop.load(std::memory_order_relaxed)) {
                return best_move;
            }

            if (move != Move::none()) {
                if (move == last_best_move) {
                    stability = std::min(stability + 1, 4);
                }
                else {
                    stability = 0;
                    last_best_move = move;
                }
                best_move = move;
            }

            std::cout << "info depth " << depth;

            if (score > Eval::MATE_THRESHOLD) {
                std::cout << " score mate " << (Eval::MATE_SCORE - score);
            } else if (score < -Eval::MATE_THRESHOLD) {
                std::cout << " score mate " << (-Eval::MATE_SCORE - score);
            } else {
                std::cout << " score cp " << score;
            }

            std::cout << " time " << time_ms
                      << " nodes " << nodes
                      << " nps " << nps
                      << " pv ";

            for (const auto& pv_move : ss->pv) {
                std::cout << pv_move.to_string() << ' ';
            }

            std::cout << '\n';

            if (limits.depth && depth >= limits.depth) {
                return best_move;
            }
            if (soft_limit != Duration::max()) {
                const auto scaled = Duration{ int64_t(soft_limit.count() * STABILITY_SCALE[stability]) };
                if (elapsed >= scaled) return best_move;
            }
        }
#if DEBUG_STATS
        stats.nodes = nodes;
#endif
        return best_move;
    }
#if DEBUG_STATS
    void Search::print_stats() const {
    const auto pct = [](const uint64_t numerator, const uint64_t denominator) -> double {
        if (denominator == 0) {
            return 0.0;
        }
        return 100.0 * static_cast<double>(numerator)
             / static_cast<double>(denominator);
    };

    std::cerr << "\n";
    std::cerr << "========== SEARCH STATS ==========\n";

    // Nodes
    std::cerr << "Nodes\n";
    std::cerr << "  nodes:              " << stats.nodes << '\n';
    std::cerr << "  qnodes:             " << stats.qnodes << '\n';
    std::cerr << "  total:              "
              << stats.nodes + stats.qnodes << '\n';

    if (stats.nodes + stats.qnodes > 0) {
        std::cerr << "  qnode %:            "
                  << std::fixed << std::setprecision(1)
                  << pct(stats.qnodes, stats.nodes + stats.qnodes)
                  << "%\n";
    }

    // TT
    std::cerr << "\nTransposition Table\n";
    std::cerr << "  probes:             " << stats.tt_probes << '\n';
    std::cerr << "  hits:               " << stats.tt_hits
              << " (" << std::fixed << std::setprecision(1)
              << pct(stats.tt_hits, stats.tt_probes) << "%)\n";

    std::cerr << "  depth sufficient:   " << stats.tt_depth_sufficient
              << " (" << std::fixed << std::setprecision(1)
              << pct(stats.tt_depth_sufficient, stats.tt_hits) << "% of hits)\n";

    std::cerr << "  cutoffs:            " << stats.tt_cutoffs
              << " (" << std::fixed << std::setprecision(1)
              << pct(stats.tt_cutoffs, stats.tt_probes) << "% of probes)\n";

    std::cerr << "    exact:            " << stats.tt_exact_cutoffs << '\n';
    std::cerr << "    lower:            " << stats.tt_lower_cutoffs << '\n';
    std::cerr << "    upper:            " << stats.tt_upper_cutoffs << '\n';

    // Search
    std::cerr << "\nSearch\n";
    std::cerr << "  beta cutoffs:       " << stats.beta_cutoffs << '\n';

    std::cerr << "  fail high:          " << stats.fail_high << '\n';
    std::cerr << "  fail low:           " << stats.fail_low << '\n';

    // Evaluation
    std::cerr << "\nEvaluation\n";
    std::cerr << "  eval calls:         " << stats.eval_calls << '\n';

    // Move generation
    std::cerr << "\nMove Generation\n";
    std::cerr << "  calls:              " << stats.movegen_calls << '\n';
    std::cerr << "  legal moves:        " << stats.legal_moves << '\n';

    if (stats.movegen_calls > 0) {
        std::cerr << "  avg moves/call:     "
                  << std::fixed << std::setprecision(2)
                  << static_cast<double>(stats.legal_moves)
                     / stats.movegen_calls
                  << '\n';
    }

    std::cerr << "\nQMove Generation\n";
    std::cerr << "  calls:              " << stats.qmovegen_calls << '\n';
    std::cerr << "  moves generated:    " << stats.qmoves_generated << '\n';

    if (stats.qmovegen_calls > 0) {
        std::cerr << "  avg moves/call:     "
                  << std::fixed << std::setprecision(2)
                  << static_cast<double>(stats.qmoves_generated)
                     / stats.qmovegen_calls
                  << '\n';
    }

    std::cerr << "==================================\n";
}
#endif

} // Engine