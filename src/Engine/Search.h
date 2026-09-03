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
       // Score stat_score;
        PV pv;
        bool ttPv = false;
        Score static_eval = Eval::NO_SCORE;
        std::array<ChessCore::Move,2> killers = {ChessCore::Move::none(), ChessCore::Move::none()};
        ChessCore::Move current_move  = ChessCore::Move::none();
        ChessCore::Move excluded = ChessCore::Move::none();
        Piece           moved_piece  = ChessCore::Pieces::EMPTY;
        History::PieceToHistory* continuation_history = nullptr;
        void update_killer(const ChessCore::Move m) {
            if (killers[0] != m) {
                killers[1] = killers[0];
                killers[0] = m;
            }
        }
    };
    class Search {
        //not tuned
        constexpr static int   RFP_MAX_DEPTH           = 8;
        constexpr static Score RFP_MARGIN_PER_DEPTH    = 80;
        constexpr static Score RFP_IMPROVING_REDUCTION = 60;

        //not tuned
        constexpr static int IIR_MIN_DEPTH = 4;

        //not tuned
        constexpr static int FP_DEPTH = 8;
        constexpr static Score FP_MARGIN  = 100; //per ply

        //not tuned
        constexpr static int LMP_MAX_DEPTH = 8;



        constexpr static int SEARCHED_LIST_CAPACITY = 32;

        //not tuned
        constexpr static int   ASP_MIN_DEPTH  = 4;    // full window below this
        constexpr static Score ASP_INIT_DELTA = 25;   // ~0.25 pawns
        constexpr static Score ASP_MAX_DELTA  = 500;  // stop widening, go full width

        //not tuned
        constexpr static int QS_SEE_MARGIN = -74;

        //not tuned
        constexpr static int SEE_CAPTURE_MARGIN = 177;
        constexpr static int SEE_QUIET_MARGIN = 23;

        //not tuned
        constexpr static int HP_MAX_DEPTH = 6; //not used
        constexpr static int HP_MARGIN    = 3000;

        ChessCore::Position pos;
        uint64_t nodes = 0;
        const SearchLimits limits;
        constexpr static int search_time_margin=100;
        std::atomic_bool stop{false};
        SearchStack stack[PV::MAX_PLY + 10]{};
        TranspositionTable& tt;
        History::CaptureHistory& capture_history;
        History::ButterflyHistory& butterfly_history;
        History::ContinuationHistory& continuation_history;
        PawnTT& pawn_tt;
        template<NodeType node_type>
        Score alpha_beta(Score alpha, Score beta,int depth,SearchStack* ss);
        [[nodiscard]] bool should_stop() const;
        template <bool in_check,NodeType node_type>
        [[nodiscard]] Score quiesce(Score alpha, Score beta,SearchStack* ss);
        [[nodiscard]] SearchResult search(int depth, SearchStack* ss, Score prev_score);
        [[nodiscard]] std::chrono::milliseconds calculate_soft_limit() const;
        [[nodiscard]] std::chrono::milliseconds calculate_hard_limit() const;

        std::chrono::time_point<std::chrono::steady_clock> search_deadline;

        void update_stats(const SearchStack* ss,ChessCore::Move best_move, ChessCore::Move tt_move, int depth,
                          const DumbVector<ChessCore::Move, SEARCHED_LIST_CAPACITY>& quiets_searched,
                          const DumbVector<ChessCore::Move, SEARCHED_LIST_CAPACITY>& captures_searched) const;
        static void update_continuation_histories(const SearchStack* ss,Piece moved,Square to,int bonus);
        static int reduction(int depth,int move_count);

    public:
        void stop_search() {stop.store(true,std::memory_order_relaxed);}
        [[nodiscard]] uint64_t node_count() const { return nodes; }
        Search(const ChessCore::Position &p,
            const SearchLimits& search_limits,
            TranspositionTable& t,
            PawnTT& pawn_t,
            History::CaptureHistory& capture_history,
            History::ButterflyHistory& butterfly_history,
            History::ContinuationHistory& continuation_history)
        :pos(p.copy_for_search())
        ,limits(search_limits),tt(t),capture_history(capture_history),
        butterfly_history(butterfly_history),
        continuation_history(continuation_history),
        pawn_tt(pawn_t)
        {};
        ChessCore::Move find_best_move();

    };
} // Engine

#endif //CHESSENGINE_SEARCH_H
