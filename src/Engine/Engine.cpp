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
        std::cerr<<std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start_time).count()<< " ms\n" << std::endl;
    }
    void Engine::go(const SearchLimits& limits)
    {
        start_time = Clock::now();
        stop();
        if constexpr (enable_book) {
            auto book_move = book[position];
            if (book_move!= ChessCore::Move::none()) {
                best_move_ = book_move;
                finish_search();
                return;
            }
        }
        searcher = std::make_unique<Search>(position, limits,tt,pawn_tt);

        searching.store(true);


        search_thread = std::jthread([this] {
            std::cerr << "A: search thread started\n";
            print_time();
            const auto move = searcher->find_best_move();
            std::cerr << "B: find_best_move returned\n";
            print_time();
            {
                std::lock_guard lock(mutex);
                best_move_ = move;
            }
            std::cerr << "C: best_move stored\n";
            print_time();
            finish_search();
            std::cerr << "D: finish_search returned\n";
            print_time();
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
        std::cerr << "E: finish_search entered\n";
        print_time();
        searching.store(false, std::memory_order_relaxed);
        const auto move = best_move();
        std::cerr << "F: best_move obtained\n";
        print_time();
        if (on_search_finished_) {
            std::cerr << "G: callback\n";
            print_time();
            on_search_finished_(move);
        }
        std::cerr << "H: callback returned\n";
        print_time();
        cv_.notify_all();
        std::cerr << "I: finish_search done\n";
        print_time();
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