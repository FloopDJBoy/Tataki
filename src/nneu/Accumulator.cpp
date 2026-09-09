//
// Created by FloopDjBoy on 06/09/2026.
//

#include "Accumulator.h"

namespace Engine::Eval::NNEU {
    void Accumulator::refresh(const ChessCore::Position &pos, const types::InputLayer &input_layer, const Color c) {
        using namespace ChessCore;
        using namespace types;
        const auto acc = (*this)[c];
        std::memcpy(acc, input_layer.bias.data(), sizeof(input_layer.bias));
        auto occ = pos.all_bb();
        while (occ) {
            const Square sq = BitBoards::pop_lsb(occ);
            const auto& w = input_layer.weights[types::feature_index(c, pos.square(sq), sq)];
            for (int i = 0; i < InputLayer::output_size; ++i)
                acc[i] += w[i];
        }
    }
    void Accumulator::add_feature(const types::InputLayer &input_layer, const Square square,const Piece p, const Color c) {
        using namespace types;
        const auto acc = (*this)[c];
        const auto& w = input_layer.weights[feature_index(c, p, square)];
        for (int i = 0; i < InputLayer::output_size; ++i)
            acc[i] += w[i];
    }

    void Accumulator::remove_feature(const types::InputLayer &input_layer, const Square square,const Piece p, const Color c) {
        using namespace types;
        const auto acc = (*this)[c];
        const auto& w = input_layer.weights[feature_index(c, p, square)];
        for (int i = 0; i < InputLayer::output_size; ++i)
            acc[i] -= w[i];
    }
}
