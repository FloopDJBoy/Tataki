//
// Created by FloopDJBoy on 16/08/2026.
//

#include "MovePicker.h"
#include <algorithm>

#include "Eval.h"

namespace Engine {
    using namespace ChessCore;
    using ChessCore::Move;
    using ChessCore::MoveGen::MoveList;
    using ChessCore::MoveGen::GenType;
    static inline PickStage& operator++(PickStage& s) {
        s = static_cast<PickStage>(static_cast<int>(s) + 1);
        return s;
    }
    MovePicker::MovePicker(const Position& p, const Move tt, const int depth ,
        const History::CaptureHistory& ch,
        const History::ButterflyHistory& bh,
        const std::array<Move,2>& killer,
        const History::PieceToHistory* const* cont_hist)
        : pos(p), tt_move(tt),capture_history(ch),butterfly_history(bh),killers(killer),depth(depth),continuation_history(cont_hist) {
        if (pos.checkers()) {
            stage = EVASION_TT;
        }else {
            stage = depth <= 0 ? QSEARCH_TT: MAIN_TT;
            if (tt_move == Move::none()) {
                ++stage;
            }
        }
    }



    // --- Core State Machine ---



    Move MovePicker::next_move() {
        while (true) {
            switch (stage) {
                //all tt is just return tt
                case QSEARCH_TT:
                case EVASION_TT:
                case MAIN_TT:
                    ++stage;
                    if (tt_move != Move::none()) {
                        return tt_move;
                    }
                    break;
                case GEN_QCAPTURE:
                case GEN_CAPTURES: {
                    MoveList<GenType::CAPTURES> caps(pos);
                    cur = moves;
                    end_bad_captures = moves;
                    end = score<GenType::CAPTURES>(caps);
                    partial_insertion_sort(cur,end,std::numeric_limits<int>::min());
                    ++stage;
                    break;
                }
                case GOOD_CAPTURES: {
                    auto filter = [&] {
                        if (pos.see_ge(static_cast<Move>(*cur), -cur->score / 18)) return true;
                        std::swap(*end_bad_captures++, *cur);
                        return false;
                    };
                    if (select(filter) != Move::none()) return static_cast<Move>(*(cur - 1));
                    ++stage;
                    break;
                }
                case GEN_QUIETS: {
                    if (!skip_quiets_) {
                        MoveList<GenType::QUIETS> movelist(pos);
                        end_captures = cur;                        // quiets begin here
                        end = end_generated = score<GenType::QUIETS>(movelist);
                        partial_insertion_sort(cur, end, quiet_sort_limit(depth));
                    }
                    ++stage;
                    break;
                }
                case GOOD_QUIETS: {
                    if (!skip_quiets_ && select([&] { return cur->score > GOOD_QUIET_THRESHOLD; }) != Move::none())
                        return static_cast<Move>(*(cur - 1));
                    cur = moves;
                    end = end_bad_captures;
                    ++stage;
                    break;
                }
                case BAD_CAPTURES: {
                    if (select([] { return true; }) != Move::none())
                        return static_cast<Move>(*(cur - 1));
                    cur = end_captures;                        // re-scan the quiets
                    end = end_generated;
                    ++stage;
                    break;
                }
                case BAD_QUIETS:
                    if (!skip_quiets_) {
                        return select([&] { return cur->score <= GOOD_QUIET_THRESHOLD; });
                    }
                    return Move::none();
                case GEN_EVASION: {
                    MoveList<GenType::EVASIONS> movelist(pos);
                    cur = moves;
                    end = score<GenType::EVASIONS>(movelist);
                    partial_insertion_sort(cur, end, std::numeric_limits<int>::min());
                    ++stage;
                    break;
                }
                case QCAPTURE:
                case EVASION: {
                    return select([]() { return true; });
                }
            }
        }
    }

    // --- Scoring and Sorting Implementations ---
    void MovePicker::partial_insertion_sort(ScoredMove* begin, const ScoredMove* end, const int limit) {
        if (begin == end)
            return;

        for (auto p = begin + 1; p != end; ++p) {
            if (p->score < limit)
                continue;

            const ScoredMove tmp = *p;
            auto q = p;

            while (q != begin && (q - 1)->score < tmp.score) {
                *q = *(q - 1);
                --q;
            }

            *q = tmp;
        }
    }

    template<GenType Type>
    ScoredMove* MovePicker::score(const MoveList<Type>& movelist) {
        static_assert(Type == GenType::CAPTURES || Type == GenType::QUIETS || Type == GenType::EVASIONS);
        constexpr int KILLER_BONUS = 1 << 16;   // 65536
        const Color us = pos.side_to_move(),them [[maybe_unused]] = ~us;
        ScoredMove* it = cur;
        for (const Move move : movelist) {
            ScoredMove& m = *it++;
            m = move;
            const Square from = m.from();
            const Square to = m.to();
            const Piece moved = pos.square(from);
            //const PieceType type = Pieces::getType(moved);
            const PieceType captured_pt =m.get_type() == MoveType::EN_PASSANT? PieceType::PAWN: Pieces::getType(pos.square(m.to()));
            if constexpr (Type == GenType::CAPTURES) {
                //const PieceType attacker_pt = Pieces::getType(moved);
                m.score = capture_history[moved][to][static_cast<int>(captured_pt)]
                 + 7 * Eval::piece_value(captured_pt);
            }else if constexpr (Type == GenType::QUIETS) {
                m.score = butterfly_history[color_idx(us)][from][to]
                    + (*continuation_history[0])[moved][to]
                       + (*continuation_history[1])[moved][to];
                if (m.get_type() == MoveType::PROMOTION)
                    m.score += 16 * Eval::piece_value(m.promotion_type());
                if (m == killers[0]) m.score += KILLER_BONUS;
                else if (m == killers[1]) m.score += KILLER_BONUS - 1;
            }else if constexpr (Type == GenType::EVASIONS) {
                //const PieceType attacker_pt = Pieces::getType(moved);
                if (pos.is_capture(move)) {
                    m.score = Eval::piece_value(captured_pt) + (1<<28);
                } else {
                    m.score = butterfly_history[color_idx(us)][from][to] + (*continuation_history[0])[moved][to];
                }
            }
        }
        return it;
    }

    template<typename Filter>
    requires std::convertible_to<std::invoke_result_t<Filter &>, bool>
    ChessCore::Move MovePicker::select(Filter &&filter) {
        for (;cur<end; ++cur) {
            if (*cur != tt_move && filter()) {
                return static_cast<Move>(*cur++); //score is no longer needed NOLINT
            }
        }
        return Move::none();
    }
} // namespace Engine