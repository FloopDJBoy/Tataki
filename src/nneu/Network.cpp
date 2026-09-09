//
// Created by FloopDjBoy on 06/09/2026.
//

#include "Network.h"

#include <immintrin.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mmintrin.h>

#include "Engine/Eval.h"
#if !(defined(_MSC_VER) || defined(__SCE__)) || __has_feature(modules) ||      \
    defined(__SSE2__)
#include <emmintrin.h>
#endif
namespace Engine::Eval::NNUE {
    namespace {
        int32_t hsum_epi32(__m256i v) {
            v = _mm256_hadd_epi32(v, v);
            v = _mm256_hadd_epi32(v, v);
            return _mm256_extract_epi32(v, 0) + _mm256_extract_epi32(v, 4);
        }
    }

    Score Network::evaluate(const Accumulator& acc, const Color stm) const {
        static_assert(HL_SIZE % 16 == 0);
        const int16_t* us   = acc[stm];
        const int16_t* them = acc[~stm];
        // OutputLayer is <256,1>, so weights is 256 contiguous int16.
        const auto* const w = reinterpret_cast<const int16_t*>(output_layer.weights.data());

        const __m256i zero = _mm256_setzero_si256();
        const __m256i qa   = _mm256_set1_epi16(QA);
        __m256i s_us = _mm256_setzero_si256();
        __m256i s_th = _mm256_setzero_si256();

        for (int i = 0; i < HL_SIZE; i += 16) {
            __m256i a = _mm256_load_si256(reinterpret_cast<const __m256i*>(us + i));
            a = _mm256_min_epi16(_mm256_max_epi16(a, zero), qa);            // clamp(x, 0, QA)
            const __m256i wa = _mm256_load_si256(reinterpret_cast<const __m256i*>(w + i));
            // (a*w) as i16, then madd against a again -> a*a*w in i32. One pass, no widening.
            s_us = _mm256_add_epi32(s_us, _mm256_madd_epi16(_mm256_mullo_epi16(a, wa), a));

            __m256i b = _mm256_load_si256(reinterpret_cast<const __m256i*>(them + i));
            b = _mm256_min_epi16(_mm256_max_epi16(b, zero), qa);
            const __m256i wb = _mm256_load_si256(reinterpret_cast<const __m256i*>(w + HL_SIZE + i));
            s_th = _mm256_add_epi32(s_th, _mm256_madd_epi16(_mm256_mullo_epi16(b, wb), b));
        }

        const int32_t sum = hsum_epi32(_mm256_add_epi32(s_us, s_th));
        const int32_t out = sum / QA + output_layer.bias[0];
        return static_cast<Score>(std::clamp(out * SCALE / (QA * QB),
                                             -MATE_THRESHOLD + 1, MATE_THRESHOLD - 1));
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
        for (const auto& row : output_layer.weights)
            if (std::abs(row[0]) > 32767 / QA)
                throw std::runtime_error("output weight " + std::to_string(row[0])
                                         + " overflows the i16 SCReLU path");
        if (!f) throw std::runtime_error("truncated net file");
    }
}
