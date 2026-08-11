//
// Created by FloopDJBoy on 11/08/2026.
//
#include "UCI.h"
#include "ChessCore/FenHelper.h"
#include "ChessCore/preft.h"

int main(int argc, char *argv[]) {
    //QApplication app(argc, argv);
    auto pos = ChessCore::Position(ChessCore::FenHelper::STARTING_POSITION);
    //assert(t == 20);
    UCI::loop();
    return 0;
}