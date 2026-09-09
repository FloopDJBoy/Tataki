//
// Created by FloopDjBoy on 06/09/2026.
//

#ifndef TATAKI_NETWORK_H
#define TATAKI_NETWORK_H
#include <filesystem>

#include "Accumulator.h"
#include "nneu_types.h"

namespace Engine::Eval::NNUE {
    using namespace types;
    struct alignas(64) Network {
        InputLayer input_layer;
        OutputLayer output_layer;
        void load(const std::filesystem::path& path);
        [[nodiscard]] Score evaluate(const Accumulator& acc, Color stm) const;
    };
    constexpr int32_t screlu(const int16_t x) {
        const int32_t y = std::clamp<int32_t>(x, 0, QA);
        return y * y;
    }
    namespace Details {inline Network g_net{}; }          // ~200 KB, zero-init in .bss
    inline const Network& net() { return Details::g_net; }
    inline void load_net(const std::filesystem::path& p) { Details::g_net.load(p); }
}

#endif //TATAKI_NETWORK_H
