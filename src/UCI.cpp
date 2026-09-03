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
            "option name OwnBook type check default false",
            "option name BookFile type string default Perfect2023.bin",
            "option name Hash type spin default 128 min 1 max 4096",
            "option name SoftNodes type check default false",
            "option name HardNodes type spin default 0 min 0 max 1000000000"
        };
        //benchmark positions from Igel which got them from Ethereal
        constexpr std::array benchmark_positions = {
            #include "bench.csv"
        };
        bool     soft_nodes = false;
        uint64_t hard_nodes = 0;
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
            // SoftNodes: reinterpret `go nodes N` as a soft limit.
            if (soft_nodes && limits.nodes) {
                limits.nodes_soft = limits.nodes;
                limits.nodes      = 0;
            }

            // HardNodes: hard ceiling, never loosens an explicit `go nodes`.
            if (hard_nodes)
                limits.nodes = limits.nodes ? std::min(limits.nodes, hard_nodes)
                                            : hard_nodes;
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
            }else if (token == "SoftNodes") {
                ss >> token;                    // "value"
                std::string v;
                ss >> v;
                soft_nodes = (v == "true" || v == "1");
            }else if (token == "HardNodes") {
                ss >> token;                    // "value"
                ss >> hard_nodes;
            }
        }
    }
    void do_ucinewgame(Engine::Engine& engine) {
        engine.stop();
        engine.clear();
        auto pos = ChessCore::FenHelper::STARTING_POSITION;
        engine.set_position(pos);
    }
    void run_benchmark(Engine::Engine& engine,std::optional<int> depth = std::nullopt) {
        using Clock = std::chrono::steady_clock;
        using ChessCore::Position;
        engine.on_search_finished(nullptr);   // no bestmove spam
        engine.set_tt_size(16);

        if (!depth) {
            depth = 12;
        }
        std::cout << "Running benchmark" << std::endl;
        engine.enable_book(false);
        const Engine::SearchLimits limits = {.depth = *depth};
        const auto start = Clock::now();
        for (auto& pos : benchmark_positions) {
            do_ucinewgame(engine);
            engine.set_position(Position(pos));
            engine.go(limits);
            engine.wait_until_search_finished();
        }
        const auto ms = std::max<int64_t>(1,
        std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now() - start).count());

        const uint64_t n = engine.nodes();
        std::cout << n << " nodes " << n * 1000 / ms << " nps" << std::endl;

    }
    static bool handle(Engine::Engine& engine,const string& command) {

        ChessCore::Position pos(ChessCore::FenHelper::STARTING_POSITION_FEN);
        //std::cerr << pos.fen() << std::endl;
        engine.on_search_finished([](const ChessCore::Move move) {
                   std::cout << "bestmove "
                             << move.to_string()
                             << std::endl;
               });
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
                return true;
            }
            auto limits = parse_go(command);
            engine.go(limits);
        }
        else if (command.starts_with("position")) {
            pos = parse_position(command);
            engine.set_position(pos);
        }else if (command == "quit") {
            return false;
        }else if (command == "stop") {
            engine.stop();
        }else if (command == "getfen") {
            std::cerr << "fen " << pos.fen() << std::endl;
        }else if (command == "ucinewgame") {
            do_ucinewgame(engine);
        }else if (command.starts_with("setoption")) {
            parse_set_option(command, engine);
        }else if (command.starts_with("mpcheck")) {
            std::istringstream ss(command);
            std::string token, path;
            ss >> token;
            if (!(ss >> path)) path = "E:/positions.txt";
            run_move_picker_suite(path);
        }else if (command == "bench") {
            std::istringstream ss(command);
            std::string token;
            int depth;
            ss >> token; //bench
            ss>>depth;
            run_benchmark(engine,depth);
        }
        return true;
    }
    void loop(const int argc, char** argv) {
        Engine::Engine engine;
        ChessCore::Position pos(ChessCore::FenHelper::STARTING_POSITION_FEN);
        engine.set_position(pos);
        engine.enable_book(false);
        const std::vector<std::string> args(argv,argv+argc);
        if (args.size() > 1) {
            for (int i = 1; i < args.size(); ++i) {
                if (!handle(engine,args[i])) return;
            }
        }
        std::string command;
        while (std::getline(std::cin,command)) {
            if (!handle(engine,command)) break;
        }

    }
}