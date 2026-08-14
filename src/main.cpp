//
// Created by FloopDJBoy on 11/08/2026.
//
#include "UCI.h"
#include "ChessCore/FenHelper.h"
#include "misc/LazyStats.h"
#include "misc/preft.h"
#include "misc/pgn_extract.h"

int main(int argc, char *argv[]) {
    //QApplication app(argc, argv);
    auto pos = ChessCore::Position(ChessCore::FenHelper::fen_to_pos("8/5R2/8/k7/2B5/5p1K/1R6/8 b - - 0 45"));
    std::string s1 = ENGINE_NAME;
    s1 += " v0.3.0";
    std::string s2 = ENGINE_NAME;
    s2 += " v0.2.7";
    const std::string edp = R"(E:\ChessEngine\assets\quiet-labeled.epd)";
    Engine::LazyTuning::run_lazy_tuning(edp);
    UCI::loop();
    return 0;
}