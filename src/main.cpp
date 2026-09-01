//
// Created by FloopDJBoy on 11/08/2026.
//
#include <map>

#include "UCI.h"
#include "ChessCore/FenHelper.h"
#include "Engine/History.h"
#include "Engine/MovePicker.h"
#include "misc/LazyStats.h"
#include "misc/preft.h"
#include "misc/pgn_extract.h"


bool check_move_picker(const ChessCore::Position& pos, const ChessCore::Move tt,
                              const int depth, const Engine::History::CaptureHistory& ch,
                              const Engine::History::ButterflyHistory& bh,const std::array<ChessCore::Move,2>& killers) {
    using namespace ChessCore;
    using namespace ChessCore::MoveGen;
    std::map<uint32_t,int> expected, got;

    if (pos.checkers())
        for (Move m : MoveList<GenType::EVASIONS>(pos)) expected[m.raw()]++;
    else {
        for (Move m : MoveList<GenType::CAPTURES>(pos)) expected[m.raw()]++;
        if (depth > 0)
            for (Move m : MoveList<GenType::QUIETS>(pos)) expected[m.raw()]++;
    }

    const bool tt_ok = tt != Move::none() && expected.contains(tt.raw());
    Engine::MovePicker mp(pos, tt_ok ? tt : Move::none(), depth, ch,bh,killers,{});
    for (Move m = mp.next_move(); m != Move::none(); m = mp.next_move())
        got[m.raw()]++;

    if (expected == got) return true;
    for (auto& [raw, n] : expected)
        if (got[raw] != n)
            std::cerr << "  " << Move(raw).to_string()
                      << " expected " << n << " got " << got[raw] << "\n";
    for (auto& [raw, n] : got)
        if (!expected.contains(raw))
            std::cerr << "  " << Move(raw).to_string() << " EXTRA x" << n << "\n";
    return false;
}
int main(int argc, char *argv[]) {
    //QApplication app(argc, argv);
    //auto pos = ChessCore::Position(ChessCore::FenHelper::fen_to_pos("8/5R2/8/k7/2B5/5p1K/1R6/8 b - - 0 45"));
    //std::string s1 = ENGINE_NAME;
    //s1 += " v0.3.0";
    //std::string s2 = ENGINE_NAME;
    //s2 += " v0.2.7";
    //const std::string edp = R"(E:\lichess-big3-resolved\lichess-big3-resolved.book)";
    //Engine::LazyTuning::run_lazy_tuning(edp);
    UCI::loop();
    return 0;
}
