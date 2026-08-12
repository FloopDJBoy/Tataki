//
// Created by FloopDJBoy on 11/08/2026.
//
#include "UCI.h"
#include "ChessCore/FenHelper.h"
#include "ChessCore/preft.h"

int main(int argc, char *argv[]) {
    //QApplication app(argc, argv);
    auto pos = ChessCore::Position(ChessCore::FenHelper::fen_to_pos("8/5R2/8/k7/2B5/5p1K/1R6/8 b - - 0 45"));
    //assert(t == 20);
    UCI::loop();
    return 0;
}