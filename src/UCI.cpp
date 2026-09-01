//
// Created by FloopDJBoy on 07/08/2026.
//

#include <iostream>
#include <sstream>
#include <string>

#include "UCI.h"

#include <fstream>
#include <map>

#include "misc/preft.h"
#include "ChessCore/FenHelper.h"
#include "ChessCore/Move.h"
#include "ChessCore/Position.h"
#include "Engine/Engine.h"
#include "Engine/MovePicker.h"

namespace UCI {
    using std::string;
    namespace {
        std::array options = {
            "option name OwnBook type check default true",
            "option name BookFile type string default Perfect2023.bin",
            "option name Hash type spin default 128 min 1 max 4096"
        };
        std::array bench_pos = {
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
            "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 10",
            "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 11",
            "4rrk1/pp1n3p/3q2pQ/2p1pb2/2PP4/2P3N1/P2B2PP/4RRK1 b - - 7 19",
            "rq3rk1/ppp2ppp/1bnpN3/3N2B1/4P3/7P/PPPQ1PP1/2KR3R b - - 0 14",
            "r1bq1r1k/1pp1n1pp/1p1p4/4p2Q/4PpP1/1BNP4/PPP2P1P/3R1RK1 b - g3 0 14",
            "r3r1k1/2p2ppp/p1p1bn2/8/1q2P3/2NPQN2/PPP3PP/R4RK1 b - - 2 15",
            "r1bbk1nr/pp3p1p/2n5/1N4p1/2Np1B2/8/PPP2PPP/2KR1B1R w kq - 0 13",
            "r1bq1rk1/ppp1nppp/4n3/3p3Q/3P4/1BP1B3/PP1N2PP/R4RK1 w - - 1 16",
            "4r1k1/3n1ppp/4p3/3p4/3P4/3BPN2/1p3PPP/1R4K1 b - - 1 24"
        };
        bool check_move_picker(const ChessCore::Position& pos, const ChessCore::Move tt,
                       const int depth, const Engine::History::CaptureHistory& ch,
                       const Engine::History::ButterflyHistory& bh,const std::array<ChessCore::Move,2>& killers) {
            using namespace ChessCore;
            using namespace ChessCore::MoveGen;

            std::map<uint32_t,int>  expected, got;
            std::map<uint32_t,Move> label;                 // raw -> Move, for printing

            auto add = [&](const Move m) { expected[m.raw()]++; label[m.raw()] = m; };

            if (pos.checkers())
                for (Move m : MoveList<GenType::EVASIONS>(pos)) add(m);
            else {
                for (Move m : MoveList<GenType::CAPTURES>(pos)) add(m);
                if (depth > 0)
                    for (Move m : MoveList<GenType::QUIETS>(pos)) add(m);
            }

            const bool tt_ok = tt != Move::none() && expected.contains(tt.raw());
            Engine::MovePicker mp(pos, tt_ok ? tt : Move::none(), depth, ch,bh,killers,{});
            for (Move m = mp.next_move(); m != Move::none(); m = mp.next_move()) {
                got[m.raw()]++;
                label[m.raw()] = m;
            }

            if (expected == got) return true;

            std::cerr << "MISMATCH  depth=" << depth
                      << "  tt=" << (tt_ok ? tt.to_string() : "none")
                      << "  fen=" << pos.fen() << "\n";
            for (auto& [raw, n] : expected)
                if (got[raw] != n)
                    std::cerr << "    " << label[raw].to_string()
                              << "  expected " << n << "  got " << got[raw] << "\n";
            for (auto& [raw, n] : got)
                if (!expected.contains(raw))
                    std::cerr << "    " << label[raw].to_string() << "  EXTRA x" << n << "\n";
            return false;
        }
        void run_move_picker_suite(const std::string& path) {
            using namespace ChessCore;
            using namespace ChessCore::MoveGen;

            static Engine::History::CaptureHistory ch{};   // 13 KB, zero-initialised
            static Engine::History::ButterflyHistory bh{};   //zero-initialised

            std::ifstream in(path);
            if (!in) { std::cerr << "cannot open " << path << "\n"; return; }

            int positions = 0, checks = 0, failures = 0;
            std::string fen;
            while (std::getline(in, fen)) {
                if (fen.empty() || fen[0] == '#') continue;
                Position pos(fen);
                ++positions;

                // every pseudo-legal move is a candidate tt_move
                std::vector<Move> candidates{Move::none()};
                if (pos.checkers())
                    for (Move m : MoveList<GenType::EVASIONS>(pos)) candidates.push_back(m);
                else {
                    for (Move m : MoveList<GenType::CAPTURES>(pos)) candidates.push_back(m);
                    for (Move m : MoveList<GenType::QUIETS>(pos))   candidates.push_back(m);
                }

                for (const int depth : {0, 1, 4})
                    for (const Move tt : candidates) {
                        ++checks;
                        if (!check_move_picker(pos, tt, depth, ch,bh,{})) ++failures;
                    }
            }
            std::cout << "mpcheck: " << positions << " positions, "
                      << checks << " checks, " << failures << " failures\n";
        }
        void run_benchmark(Engine::Engine& engine) {
            engine.enable_book(false);
            Engine::SearchLimits limits = {.depth = 12,.nodes = 5000000};
            for (const auto& fen : bench_pos) {
                auto start_time = std::chrono::high_resolution_clock::now();
                std::cerr<<fen <<std::endl;
                ChessCore::Position pos(fen);
                engine.clear();
                engine.set_position(pos);
                engine.go(limits);
                engine.wait_until_search_finished();
#if DEBUG_STATS
                engine.print_stats();
#endif

            }
        }
        ChessCore::Position parse_position(const std::string& command)
        {
            ChessCore::Position pos = ChessCore::FenHelper::STARTING_POSITION;
            std::istringstream ss(command);
            std::string token;

            ss >> token; // "position"
            ss >> token;

            if (token == "startpos") {
                pos = ChessCore::FenHelper::STARTING_POSITION;
            }
            else if (token == "kiwipete") {
                pos = ChessCore::FenHelper::KIWIPETE;
            }
            else if (token == "fen") {
                std::string fen;
                std::string field;

                for (int i = 0; i < 6 && ss >> field; ++i) {
                    if (!fen.empty())
                        fen += ' ';

                    fen += field;
                }

                pos = ChessCore::FenHelper::fen_to_pos(fen);
            }

            if (ss >> token && token == "moves") {
                while (ss >> token) {
                    const ChessCore::Move move = pos.parse_move(token);

                    if (move == ChessCore::Move::none())
                        break;

                    pos.make_move(move);
                }
            }
            return pos;
        }
        Engine::SearchLimits parse_go(const std::string& command) {
            Engine::SearchLimits limits;
            std::istringstream ss(command);
            std::string token;
            ss >> token; //the word "go"
            while (ss >> token) {
                if (token == "wtime") {
                    ss >> limits.wtime_ms;
                }
                else if (token == "btime") {
                    ss >> limits.btime_ms;
                }
                else if (token == "winc") {
                    ss >> limits.winc_ms;
                }
                else if (token == "binc") {
                    ss >> limits.binc_ms;
                }
                else if (token == "movestogo") {
                    ss >> limits.moves_to_go;
                }
                else if (token == "depth") {
                    ss >> limits.depth;
                }
                else if (token == "nodes") {
                    ss >> limits.nodes;
                }
                else if (token == "movetime") {
                    ss >> limits.movetime_ms;
                }
                else if (token == "mate") {
                    ss >> limits.mate;
                }else if (token == "nodessoft") {
                    ss >> limits.nodes_soft;
                }
                else if (token == "infinite") {
                    limits.infinite = true;
                }
                else if (token == "ponder") {
                    limits.ponder = true;
                }
            }
            return limits;
        }
        void parse_set_option(const std::string& command,Engine::Engine& engine) {
            std::istringstream ss(command);
            std::string token;
            ss >> token; //setoption
            ss >> token; // name

            ss >>  token;
            if (token == "OwnBook") {
                ss >> token; // value
                bool enabled;
                ss >> std::boolalpha >> enabled;
                engine.enable_book(enabled);
            }
            else if (token == "BookFile") {
                ss >> token; // value

                std::string path;
                std::getline(ss >> std::ws, path);

                engine.set_book(path);
            }else if (token == "Hash") {
                ss>>token; //value
                size_t tt_size;
                ss>>tt_size;
                engine.set_tt_size(tt_size);
            }
        }
    }
    void loop() {
        string command;
        Engine::Engine engine("assets/opening_books/Perfect2023.bin");
        ChessCore::Position pos(ChessCore::FenHelper::STARTING_POSITION_FEN);
        //std::cerr << pos.fen() << std::endl;
        assert(pos.fen() == ChessCore::FenHelper::STARTING_POSITION_FEN);
        engine.set_position(pos);
        engine.enable_book(false);
        engine.on_search_finished([](const ChessCore::Move move) {
                   std::cout << "bestmove "
                             << move.to_string()
                             << std::endl;
               });
        while (std::getline(std::cin, command)) {
            if (command == "uci") {
                std::cout << "id name "<< ENGINE_NAME << " " << ENGINE_VERSION << std::endl;
                std::cout << "id author FloopDJBoy" << std::endl;
                for (const auto& option : options) {
                    std::cout << option << std::endl;
                }
                std::cout << "uciok" << std::endl;
            }
            else if (command == "isready") {
                std::cout << "readyok" << std::endl;
            }else if (command.starts_with("go")) {
                std::string token;
                std::istringstream ss(command);
                ss >> token; //"go"
                ss >> token;
                if (token == "preft") {
                    int depth;
                    ss >> depth;
                    ChessCore::preft::test(pos, depth);
                    continue;
                }
                auto limits = parse_go(command);
                engine.go(limits);
            }
            else if (command.starts_with("position")) {
                pos = parse_position(command);
                engine.set_position(pos);
            }else if (command == "quit") {
                break;
            }else if (command == "stop") {
                engine.stop();
            }else if (command == "getfen") {
                std::cerr << "fen " << pos.fen() << std::endl;
            }else if (command == "ucinewgame") {
                engine.stop();
                engine.clear();
                pos = ChessCore::FenHelper::STARTING_POSITION;
                engine.set_position(pos);
            }else if (command.starts_with("setoption")) {
                parse_set_option(command, engine);
            }else if (command=="bench") {
                run_benchmark(engine);
            }else if (command.starts_with("mpcheck")) {
                std::istringstream ss(command);
                std::string token, path;
                ss >> token;
                if (!(ss >> path)) path = "E:/positions.txt";
                run_move_picker_suite(path);
            }
        }
    }
}

