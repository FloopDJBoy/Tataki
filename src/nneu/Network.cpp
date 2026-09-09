//
// Created by FloopDjBoy on 06/09/2026.
//

#include "Network.h"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "Engine/Eval.h"

namespace Engine::Eval::NNUE {
    Score Network::evaluate(const Accumulator& acc, const Color stm) const {
        const int16_t* us   = acc[stm];
        const int16_t* them = acc[~stm];

        int32_t sum = 0;
        for (int i = 0; i < HL_SIZE; ++i) {
            sum += screlu(us[i])   * output_layer.weights[i][0];
            sum += screlu(them[i]) * output_layer.weights[i + HL_SIZE][0];
        }

        const int32_t out = sum / QA + output_layer.bias[0];
        return static_cast<Score>(std::clamp(out * SCALE / (QA * QB), -MATE_THRESHOLD + 1, MATE_THRESHOLD - 1));
    }

    void Network::load(const std::filesystem::path& path) {
        constexpr size_t NET_BYTES =
        sizeof(int16_t) * INPUT_SIZE * HL_SIZE   // l0w
        + sizeof(int16_t) * HL_SIZE                // l0b
        + sizeof(int16_t) * HL_SIZE * 2            // l1w
        + sizeof(int16_t);                         // l1b
        std::ifstream f(path, std::ios::binary);
        if (!f) throw std::runtime_error("cannot open net: " + path.string());

        const auto on_disk = std::filesystem::file_size(path);
        if (on_disk < NET_BYTES)
            throw std::runtime_error("net too small: got " + std::to_string(on_disk)
                                   + ", need " + std::to_string(NET_BYTES));
        auto read = [&](void* dst, const size_t n) { f.read(static_cast<char*>(dst), n); };
        read(input_layer.weights.data(),  sizeof(int16_t) * INPUT_SIZE * HL_SIZE);
        read(input_layer.bias.data(),     sizeof(int16_t) * HL_SIZE);
        read(output_layer.weights.data(), sizeof(int16_t) * HL_SIZE * 2);
        read(output_layer.bias.data(),    sizeof(int16_t));

        if (!f) throw std::runtime_error("truncated net file");
    }
}
