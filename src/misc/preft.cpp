//
// Created by FloopDJBoy on 06/08/2026.
//

#include "preft.h"

#include <iostream>
#include <chrono>
#include <iomanip>
#include <vector>
#include <string>
#include "../ChessCore/FenHelper.h"
#include "Engine/MovePicker.h"

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
            Engine::MovePicker move_picker(pos,Move::none(),10,{},{});
            for (Move move = move_picker.next_move();move != Move::none();move = move_picker.next_move()) {
                //g_path.push_back(move.to_string());
                if (!pos.legal(move)){continue;}
                pos.make_move(move);
                nodes += run(pos, depth - 1);
                pos.undo_move();
                //g_path.pop_back();
            }

            return nodes;
        }
    }
    uint64_t divide(const Position& original, const int depth)
    {
        Position pos = original.copy_for_search();
        uint64_t total = 0;

        for (const Move move : pos.legal_moves())
        {
            pos.make_move(move);

            const uint64_t nodes = run(pos, depth - 1);

            pos.undo_move();

            std::cout << move.to_string()
                      << ": "
                      << nodes
                      << '\n';

            total += nodes;
        }

        std::cout << "Nodes searched: " << total << '\n';

        return total;
    }
    void test(const Position& original, const int max_depth)
    {
        using clock = std::chrono::steady_clock;
        Position pos = original.copy_for_search();
        g_path.reserve(50);

        for (int depth = 1; depth <= max_depth; ++depth)
        {
            const auto start = clock::now();

            const uint64_t nodes = run(pos, depth);

            const auto end = clock::now();

            const auto milliseconds =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    end - start
                ).count();

            const uint64_t nps =
                milliseconds > 0
                    ? nodes * 1000ULL / static_cast<uint64_t>(milliseconds)
                    : 0;

            std::cout
                << "info depth " << depth
                << " nodes " << nodes
                << " time " << milliseconds
                << " nps " << nps
                << '\n';
        }
    }

} // ChessCore