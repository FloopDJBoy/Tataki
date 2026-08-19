//
// Created by FloopDJBoy on 07/08/2026.
//

#ifndef CHESSENGINE_SEARCH_H
#define CHESSENGINE_SEARCH_H
#include <atomic>
#include <cstring>

#include "History.h"
#include "OpeningBook.h"
#include "PawnTT.h"
#include "SearchLimits.h"
#include "TranspositionTable.h"
#include "misc/SearchStats.h"

#define DEBUG_STATS 0


namespace Engine {
    enum class NodeType {
        PV,
        NonPV,
        Root
    };
    struct SearchResult {
        ChessCore::Move move;
        Score score;
    };
    struct PV {
        static constexpr uint32_t MAX_PLY = 256;
        ChessCore::Move moves[MAX_PLY];
        uint32_t length = 0;

        void clear() {
            length = 0;
        }

        void update(const ChessCore::Move move, const PV& child) {
            assert(child.length < MAX_PLY);

            moves[0] = move;

            std::memcpy(
                moves + 1,
                child.moves,
                child.length * sizeof(ChessCore::Move)
            );

            length = child.length + 1;
        }

        void update(const ChessCore::Move move) {
            moves[0] = move;
            length = 1;
        }

        ChessCore::Move* begin() { return moves; }
        ChessCore::Move* end() { return moves + length; }

        [[nodiscard]] const ChessCore::Move* begin() const { return moves; }
        [[nodiscard]] const ChessCore::Move* end() const { return moves + length; }

        [[nodiscard]] uint32_t size() const { return length; }
        [[nodiscard]] bool empty() const { return length == 0; }

        ChessCore::Move& operator[](const uint32_t i) {
            assert(i < length);
            return moves[i];
        }

        const ChessCore::Move& operator[](const uint32_t i) const {
            assert(i < length);
            return moves[i];
        }
    };
    struct SearchStack {
        Score stat_score; //ranks the move played based on history
        PV pv;
    };
    class Search {
        constexpr static int SEARCHED_LIST_CAPACITY = 32;
#if DEBUG_STATS
        SearchStats stats;
#endif
        ChessCore::Position pos;
        uint64_t nodes = 0;
        const SearchLimits limits;
        constexpr static int search_time_margin=100;
        std::atomic_bool stop{false};
        SearchStack stack[PV::MAX_PLY + 10]{};
        TranspositionTable& tt;
        History::CaptureHistory& capture_history;
        PawnTT& pawn_tt;
        template<NodeType node_type>
        Score alpha_beta(Score alpha, Score beta,int depth,SearchStack* ss);
        [[nodiscard]] bool should_stop() const;
        template <bool in_check>
        [[nodiscard]] Score quiesce(Score alpha, Score beta,SearchStack* ss);
        [[nodiscard]]SearchResult search(int depth,SearchStack* ss);
        [[nodiscard]] std::chrono::milliseconds calculate_time_limit() const;
        std::chrono::time_point<std::chrono::steady_clock> search_deadline;

        void update_stats(ChessCore::Move best_move, ChessCore::Move tt_move,int depth,const DumbVector<ChessCore::Move,SEARCHED_LIST_CAPACITY> &captures_searched,const SearchStack* ss);

    public:
        void stop_search() {stop.store(true,std::memory_order_relaxed);}
        Search(const ChessCore::Position &p,const SearchLimits& search_limits,TranspositionTable& t,PawnTT& pawn_t,History::CaptureHistory& capture_history)
        :pos(p.copy_for_search())
        ,limits(search_limits),tt(t),pawn_tt(pawn_t),
        capture_history(capture_history)
        {};
        ChessCore::Move find_best_move();
#if DEBUG_STATS
        void print_stats() const;
#endif
    };
} // Engine

#endif //CHESSENGINE_SEARCH_H
