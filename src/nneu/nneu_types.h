//
// Created by FloopDjBoy on 06/09/2026.
//

#ifndef TATAKI_NNEU_TYPES_H
#define TATAKI_NNEU_TYPES_H
#include "Types.h"
#include "ChessCore/Pieces.h"

namespace Engine::Eval::NNEU::types {
    constexpr int INPUT_SIZE = 768;
    constexpr int HL_SIZE    = 128;
    constexpr int QA = 255, QB = 64, SCALE = 218;
    constexpr int CACHE_LINE_SIZE = 64;
    // 768 = 2 colors x 6 piece types x 64 squares
    inline int feature_index(const Color perspective, const Piece p, const Square sq) {
        const int color_off = (ChessCore::Pieces::getColor(p) == perspective) ? 0 : 384;
        const int pt_off    = (static_cast<int>(ChessCore::Pieces::getType(p)) - 1) * 64;  // PAWN=1 -> 0
        const int rel_sq    = (perspective == Color::WHITE) ? sq : (sq ^ 56);
        return color_off + pt_off + rel_sq;
    }
    template<int In, int Out, typename W = int16_t, typename B = int16_t>
    struct alignas(64) LinearLayer {
        constexpr static int input_size = In;
        constexpr static int output_size = Out;
        std::array<std::array<W, Out>, In> weights;   // [input][output]
        std::array<B, Out>                 bias;
        constexpr static LinearLayer zeroed_out(){return  LinearLayer();};
    };
    using InputLayer = LinearLayer<INPUT_SIZE,HL_SIZE>;
    using OutputLayer = LinearLayer<HL_SIZE * 2, 1>;
}

#endif //TATAKI_NNEU_TYPES_H
