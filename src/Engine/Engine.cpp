//
// Created by FloopDJBoy on 07/08/2026.
//

#include "Engine.h"

#include <iostream>

namespace Engine {
    using Clock = std::chrono::steady_clock;
    static auto start_time = Clock::now();
    Engine::~Engine()
    {
        stop();
    }
    inline static void print_time() {
        ////std::cerr<<std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start_time).count()<< " ms\n" << std::endl;
    }
    void Engine::go(const SearchLimits& limits)
    {
        start_time = Clock::now();
        stop();
        if (enable_book_) {
            auto book_move = (*book)[position];
            if (book_move!= ChessCore::Move::none()) {
                best_move_ = book_move;
                finish_search();
                return;
            }
        }
        searcher = std::make_unique<Search>(position, limits,tt,pawn_tt,capture_history,butterfly_history,*continuation_history);

        searching.store(true);


        search_thread = std::jthread([this] {
            const auto [move,score] = searcher->find_best_move();
            {
                std::lock_guard lock(mutex);
                total_nodes.fetch_add(searcher->node_count(), std::memory_order_relaxed);
                best_move_ = move;
                score_ = score;
            }
            finish_search();
        });
    }

    void Engine::stop()
    {
        if (searcher)
            searcher->stop_search();

        if (search_thread.joinable())
            search_thread.join();

        searcher.reset();

        searching.store(false);
    }
    bool Engine::is_finished() const
    {
        return !searching.load();
    }

    void Engine::finish_search() {
        //std::cerr << "E: finish_search entered\n";
        //print_time();
        searching.store(false, std::memory_order_relaxed);
        const auto move = best_move();
        const auto score = search_score();
        if (on_search_finished_) {
            on_search_finished_(move,score);
        }
        cv_.notify_all();
    }

    ChessCore::Move Engine::best_move() const
    {
        std::lock_guard lock(mutex);
        return best_move_;
    }

    Score Engine::search_score() const {
        std::lock_guard lock(mutex);
        return score_;
    }

    void Engine::set_position(const ChessCore::Position& pos)
    {
        stop();
        position = pos;
    }

    void Engine::wait_until_search_finished() const{
        std::unique_lock lock(mutex);
        cv_.wait(lock, [this] {
            return is_finished();
        });
    }
} // Engine