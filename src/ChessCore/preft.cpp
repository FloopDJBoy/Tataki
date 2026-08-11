//
// Created by FloopDJBoy on 06/08/2026.
//

#include "preft.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <string>
#include "FenHelper.h"

namespace ChessCore::preft {
    namespace {
        std::vector<std::string> g_path;
    }
    namespace {
        uint64_t run(Position& pos, const int depth)
        {
            if (depth == 0)
                return 1;

            uint64_t nodes = 0;


            for (const Move move : pos.legal_moves()) {
                g_path.push_back(move.to_string());
                const Key parent_key = pos.zobrist_key();
                pos.make_move(move);
                pos.verify_zobrist();
                nodes += run(pos, depth - 1);
                pos.undo_move();
                assert(pos.zobrist_key() == parent_key);
                pos.verify_zobrist();
                g_path.pop_back();
            }

            return nodes;
        }
    }
    uint64_t divide(const Position& original, const int depth){
        Position pos = original.copy_for_search();
        uint64_t total = 0;
        for (const Move move : pos.legal_moves())
        {
            pos.make_move(move);

            const uint64_t nodes = run(pos, depth - 1);

            pos.undo_move();

            std::cout << move.to_string() << ": " << nodes << std::endl;

            total += nodes;
        }

        std::cout << "Total: " << total << std::endl;

        return total;
    }
    void test(const Position& original, const int max_depth)
    {
        using clock = std::chrono::steady_clock;
        Position pos = original.copy_for_search();
        for (int depth = 1; depth <= max_depth; ++depth)
        {
            const auto start = clock::now();

            const uint64_t nodes = run(pos, depth);

            const auto end = clock::now();
            const double seconds =
                std::chrono::duration<double>(end - start).count();

            const uint64_t nps =
                seconds > 0.0 ? static_cast<uint64_t>(nodes / seconds) : 0;

            std::cout
                << "Depth " << depth
                << ": " << nodes << " nodes"
                << " | Time: " << std::fixed << std::setprecision(3)
                << seconds << " s"
                << " | NPS: " << nps
                << '\n';
        }
    }

} // ChessCore