//
// Created by FloopDJBoy on 13/08/2026.
//

#ifndef CHESSENGINE_PGN_EXTRACT_H
#define CHESSENGINE_PGN_EXTRACT_H


#include <fstream>
#include <iostream>
#include <string>
namespace ChessCore::pgn_extract{
    struct Stats {
        int wins = 0;
        int losses = 0;
        int draws = 0;

        int games() const {
            return wins + losses + draws;
        }

        double score() const {
            return games()
                ? (wins + 0.5 * draws) / games() * 100.0
                : 0.0;
        }
    };

    void run(const std::string& file_path,const std::string &name_first,const std::string& name_second) {
        std::ifstream file((file_path));

        Stats v1;
        Stats v2;

        std::string line;
        std::string white;
        std::string black;
        std::string result;

        while (std::getline(file, line)) {
            if (line.starts_with("[White ")) {
                white = line;
            }
            else if (line.starts_with("[Black ")) {
                black = line;
            }
            else if (line.starts_with("[Result ")) {
                result = line;
            }
            else if (line.empty() && !result.empty()) {
                // Process game
                const bool v1White = white.find(name_first) != std::string::npos;

                if (result.find("\"1/2-1/2\"") != std::string::npos) {
                    ++v1.draws;
                    ++v2.draws;
                }
                else if (result.find("\"1-0\"") != std::string::npos) {
                    if (v1White) {
                        ++v1.wins;
                        ++v2.losses;
                    } else {
                        ++v2.wins;
                        ++v1.losses;
                    }
                }
                else if (result.find("\"0-1\"") != std::string::npos) {
                    if (v1White) {
                        ++v1.losses;
                        ++v2.wins;
                    } else {
                        ++v2.losses;
                        ++v1.wins;
                    }
                }

                white.clear();
                black.clear();
                result.clear();
            }
        }

        std::cout << name_first << std::endl;
        std::cout << "  Wins:   " << v1.wins << '\n';
        std::cout << "  Losses: " << v1.losses << '\n';
        std::cout << "  Draws:   " << v1.draws << '\n';
        std::cout << "  Score:   " << v1.score() << "%\n\n";

        std::cout << name_second << std::endl;
        std::cout << "  Wins:   " << v2.wins << '\n';
        std::cout << "  Losses: " << v2.losses << '\n';
        std::cout << "  Draws:   " << v2.draws << '\n';
        std::cout << "  Score:   " << v2.score() << "%\n";
    }
}


#endif //CHESSENGINE_PGN_EXTRACT_H
