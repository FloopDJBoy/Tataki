#include "LazyStats.h"

#include <chrono>
#include <fstream>
#include <iostream>
#include <string>

#include "ChessCore/Position.h"
#include "Engine/Eval.h"
//
// Created by FloopDJBoy on 14/08/2026.
//
namespace Engine::LazyTuning {
    void run_lazy_tuning(const std::string& epd_path) {
    std::ifstream file(epd_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open " << epd_path << std::endl;
        return;
    }

    LazyStats stats;
    std::string line;

    auto start_time = std::chrono::high_resolution_clock::now();

    while (std::getline(file, line)) {
        if (line.empty()) continue;

        // EPD lines usually end with extra opcodes/comments separated by ';'
        std::string fen = line.substr(0, line.find_first_of(";"));

        // Setup position from FEN
        ChessCore::Position pos(fen);

        // 1. Calculate Lazy Evaluation (Material + Phase Interpolation)
        const int mg_weight = std::clamp(static_cast<int>(pos.state().phase), 0, Engine::Eval::MAX_PHASE);
        const int eg_weight = Engine::Eval::MAX_PHASE - mg_weight;

        const auto& mat = pos.state().material_score;
        const auto& mat_w = mat[color_idx(Color::WHITE)];
        const auto& mat_b = mat[color_idx(Color::BLACK)];

        const int tempo = Eval::interpolate(Eval::TEMPO_BONUS,mg_weight);
        const int w_tempo = pos.side_to_move() == Color::WHITE? tempo : -tempo;

        const int lazy_w = (mat_w.mg * mg_weight + mat_w.eg * eg_weight) / Engine::Eval::MAX_PHASE;
        const int lazy_b = (mat_b.mg * mg_weight + mat_b.eg * eg_weight) / Engine::Eval::MAX_PHASE;
        const int lazy_diff = lazy_w - lazy_b + w_tempo;
        const int lazy_eval = (pos.side_to_move() == Color::WHITE) ? lazy_diff : -lazy_diff;

        // 2. Force Full Evaluation (Pass wide window to bypass lazy cutoff)
        const int full_eval = Eval::evaluate(pos);

        // 3. Compute error delta
        int delta = std::abs(full_eval - lazy_eval);
        stats.add(delta);
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();

    stats.print_report();
    std::cout << "Time elapsed: " << duration << " ms\n";
}
}