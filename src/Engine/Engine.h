//
// Created by FloopDJBoy on 07/08/2026.
//

#ifndef CHESSENGINE_ENGINE_H
#define CHESSENGINE_ENGINE_H
#include <condition_variable>
#include <thread>

#include "OpeningBook.h"
#include "Search.h"
#include "ChessCore/FenHelper.h"

namespace Engine {
    class Engine {
        OpeningBook book;
        ChessCore::Position position;

        std::unique_ptr<Search> searcher;
        std::jthread search_thread;

        ChessCore::Move best_move_;
        std::atomic_bool searching = false;
        mutable std::mutex mutex;
        mutable std::condition_variable cv_;

        void finish_search();

    public:
        explicit Engine(
            const std::string& book_path,
            const ChessCore::Position* pos = nullptr
        )
            : book(book_path),
              position(pos ? *pos : ChessCore::FenHelper::STARTING_POSITION)
        {}

        ~Engine();

        void set_position(const ChessCore::Position& pos);

        void go(const SearchLimits& limits);
        void stop();
        void wait_until_search_finished() const;
        bool is_finished() const;
        ChessCore::Move best_move() const;
    };
} // Engine

#endif //CHESSENGINE_ENGINE_H
