//
// Created by FloopDJBoy on 07/08/2026.
//

#include <iostream>
#include <sstream>
#include <string>

#include "UCI.h"
#include "misc/preft.h"
#include "ChessCore/FenHelper.h"
#include "ChessCore/Move.h"
#include "ChessCore/Position.h"
#include "Engine/Engine.h"

namespace UCI {
    using std::string;
    namespace {
        std::array options = {
            "option name OwnBook type check default true",
            "option name BookFile type string default Perfect2023.bin",
            "option name Hash type spin default 128 min 1 max 4096"
        };
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

            }
        }
    }
}

