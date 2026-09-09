//
// Created by FloopDJBoy on 11/08/2026.
//
#include <map>

#include "UCI.h"
#include "ChessCore/FenHelper.h"
#include "Engine/Engine.h"
#include "Engine/Eval.h"
#include "Engine/History.h"
#include "Engine/MovePicker.h"
#include "misc/FitScale.h"
#include "misc/LazyStats.h"
#include "misc/preft.h"
#include "misc/pgn_extract.h"
#include "nneu/Accumulator.h"
#include "nneu/FilterData.h"
#include "nneu/Network.h"
#include "nneu/ViriFormat.h"


static bool check_move_picker(const ChessCore::Position& pos, const ChessCore::Move tt,
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
    //const std::string edp = R"(E:\lichess-big3-resolved\lichess-big3-resolved.book)";
    //Engine::LazyTuning::run_lazy_tuning(edp);
    //Engine::Eval::NNEU::FilterData::pgn_to_viriformat_mt(R"(E:\fastchess-windows-x86-64\fastchess-windows-x86-64\5k_soft_self_gen)",R"(E:\traning_data\set1.vf)",15);
    //FitScale::fit_scale(R"(E:\fastchess-windows-x86-64\fastchess-windows-x86-64\5k_soft_self_gen)",15);
    //std::cout << Engine::Eval::NNEU::ViriFormat::self_test();

    Engine::Eval::NNUE::load_net(R"(E:\bullet\target\release\checkpoints\simple-40\quantised.bin)");
    UCI::loop(argc,argv);
    return 0;
}
