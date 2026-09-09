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
    ChessCore::Position pos(ChessCore::FenHelper::STARTING_POSITION_FEN);
    Engine::Eval::NNEU::Network net{};
    net.load(R"(E:\bullet\target\release\checkpoints\simple-40\quantised.bin)");
    constexpr std::array benchmark_positions = {
        #include "bench.csv"
    };
    Engine::Eval::NNEU::Accumulator acc{};
    std::vector<std::tuple<std::string,Score,Score,Score>> res;
    Engine::Engine engine;
    engine.enable_book(false);
    constexpr Engine::SearchLimits limits={.depth=15};

    for (auto& fen : benchmark_positions) {
        pos = ChessCore::Position(fen);
        acc.refresh(pos,net.input_layer);
        engine.set_position(pos);
        engine.go(limits);
        engine.wait_until_search_finished();
        res.emplace_back(fen,net.evaluate(acc,pos.side_to_move()),Engine::Eval::evaluate(pos),engine.search_score());
    }
    for (auto& [fen,nn_score,hce_score,d15_score] : res) {
        std::cout << fen << " nneu: " << nn_score << " hce: " << hce_score << " depth 15 hce: " << d15_score << std::endl;
    }


    //UCI::loop(argc,argv);
    return 0;
}
