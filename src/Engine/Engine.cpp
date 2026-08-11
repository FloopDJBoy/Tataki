//
// Created by FloopDJBoy on 07/08/2026.
//

#include "Engine.h"

namespace Engine {
    Engine::~Engine()
    {
        stop();
    }
    void Engine::go(const SearchLimits& limits)
    {
        stop();
        //auto book_move = book[position];
        //if (book_move!= ChessCore::Move::none()) {
        //    best_move_ = book_move;
        //    finish_search();
        //    return;
        //}
        searcher = std::make_unique<Search>(position, limits);

        searching.store(true);


        search_thread = std::jthread([this] {
            const auto move = searcher->find_best_move();
            {
                std::lock_guard lock(mutex);
                best_move_ = move;
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

        searching.store(false);
    }

    bool Engine::is_finished() const
    {
        return !searching.load();
    }

    void Engine::finish_search() {
        searching.store(false, std::memory_order_relaxed);
        cv_.notify_one();
    }

    ChessCore::Move Engine::best_move() const
    {
        std::lock_guard lock(mutex);
        return best_move_;
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