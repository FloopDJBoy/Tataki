//
// Created by FloopDjBoy on 06/09/2026.
//

#include "Accumulator.h"

#include <immintrin.h>

#include "ChessCore/BitBoards.h"
#include <ChessCore/Position.h>

namespace Engine::Eval::NNUE {
    namespace {
        constexpr int CHUNKS = HL_SIZE / 16;   // 16 int16 per ymm -> 8 registers at HL_SIZE=128
        static_assert(HL_SIZE % 16 == 0);
        static_assert(CHUNKS <= 14, "refresh would spill; tile the loop instead");

        inline __m256i ld(const int16_t* p) {
            return _mm256_load_si256(reinterpret_cast<const __m256i*>(p));
        }
        inline void st(int16_t* p, const __m256i v) {
            _mm256_store_si256(reinterpret_cast<__m256i*>(p), v);
        }
    }

    void Accumulator::refresh(const ChessCore::Position& pos, const InputLayer& il, const Color c) {
        using namespace ChessCore;
        int16_t* const acc = (*this)[c];

        __m256i regs[CHUNKS];
        for (int k = 0; k < CHUNKS; ++k) regs[k] = ld(il.bias.data() + k * 16);

        auto occ = pos.all_bb();
        while (occ) {
            const Square sq = BitBoards::pop_lsb(occ);
            const int16_t* const w = il.weights[feature_index(c, pos.square(sq), sq)].data();
            for (int k = 0; k < CHUNKS; ++k)
                regs[k] = _mm256_add_epi16(regs[k], ld(w + k * 16));
        }
        for (int k = 0; k < CHUNKS; ++k) st(acc + k * 16, regs[k]);
    }

    void Accumulator::add_feature(const InputLayer& il, const Square sq, const Piece p, const Color c) {
        int16_t* __restrict acc       = (*this)[c];
        const int16_t* __restrict w   = il.weights[feature_index(c, p, sq)].data();
        for (int i = 0; i < HL_SIZE; i += 16)
            st(acc + i, _mm256_add_epi16(ld(acc + i), ld(w + i)));
    }

    void Accumulator::remove_feature(const InputLayer& il, const Square sq, const Piece p, const Color c) {
        int16_t* __restrict acc       = (*this)[c];
        const int16_t* __restrict w   = il.weights[feature_index(c, p, sq)].data();
        for (int i = 0; i < HL_SIZE; i += 16)
            st(acc + i, _mm256_sub_epi16(ld(acc + i), ld(w + i)));
    }

    // quiet move
    void Accumulator::add_sub(const Accumulator& parent, const InputLayer& il,
                              const int add, const int sub, const Color c) {
        int16_t* __restrict dst       = (*this)[c];
        const int16_t* __restrict src = parent[c];
        const int16_t* __restrict wa  = il.weights[add].data();
        const int16_t* __restrict ws  = il.weights[sub].data();
        for (int i = 0; i < HL_SIZE; i += 16) {
            __m256i v = ld(src + i);
            v = _mm256_add_epi16(v, ld(wa + i));
            v = _mm256_sub_epi16(v, ld(ws + i));
            st(dst + i, v);
        }
    }

    // capture, and promotion-capture
    void Accumulator::add_sub_sub(const Accumulator& parent, const InputLayer& il,
                                  const int add, const int sub1, const int sub2, const Color c) {
        int16_t* __restrict dst       = (*this)[c];
        const int16_t* __restrict src = parent[c];
        const int16_t* __restrict wa  = il.weights[add].data();
        const int16_t* __restrict w1  = il.weights[sub1].data();
        const int16_t* __restrict w2  = il.weights[sub2].data();
        for (int i = 0; i < HL_SIZE; i += 16) {
            __m256i v = ld(src + i);
            v = _mm256_add_epi16(v, ld(wa + i));
            v = _mm256_sub_epi16(v, ld(w1 + i));
            v = _mm256_sub_epi16(v, ld(w2 + i));
            st(dst + i, v);
        }
    }

    // castling: king and rook both move
    void Accumulator::add_add_sub_sub(const Accumulator& parent, const InputLayer& il,
                                      const int a1, const int a2,
                                      const int s1, const int s2, const Color c) {
        int16_t* __restrict dst       = (*this)[c];
        const int16_t* __restrict src = parent[c];
        const int16_t* __restrict wa1 = il.weights[a1].data();
        const int16_t* __restrict wa2 = il.weights[a2].data();
        const int16_t* __restrict ws1 = il.weights[s1].data();
        const int16_t* __restrict ws2 = il.weights[s2].data();
        for (int i = 0; i < HL_SIZE; i += 16) {
            __m256i v = ld(src + i);
            v = _mm256_add_epi16(v, ld(wa1 + i));
            v = _mm256_add_epi16(v, ld(wa2 + i));
            v = _mm256_sub_epi16(v, ld(ws1 + i));
            v = _mm256_sub_epi16(v, ld(ws2 + i));
            st(dst + i, v);
        }
    }
    void AccumulatorStack::push_move(const InputLayer& il, const ChessCore::Position& pos, const ChessCore::Move m) {
        using namespace ChessCore;
        if (m == Move::null()) {
            push_null();
            return;
        }
        const Square from = m.from(), to = m.to();
        const Color  us   = pos.side_to_move();
        const Piece  moved = pos.square(from);
        const Accumulator& parent = stack_[index_];
        Accumulator& child = stack_[index_ + 1];

        switch (m.get_type()) {
            case MoveType::NORMAL: {
                const Piece cap = pos.square(to);
                if (cap == Pieces::EMPTY) child.move_piece(parent, il, from, to, moved,moved);
                else                      child.capture_piece(parent, il, from, to, moved, moved, to, cap);
                break;
            }
            case MoveType::PROMOTION: {
                const Piece promo = Pieces::makePiece(m.promotion_type(), us);
                const Piece cap   = pos.square(to);
                if (cap == Pieces::EMPTY) child.move_piece(parent, il, from, to, moved, promo);
                else                      child.capture_piece(parent, il, from, to, moved, promo, to, cap);
                break;
            }
            case MoveType::EN_PASSANT: {
                const int offset = (us == Color::WHITE) ? 8 : -8;
                const Square cap_sq = to - offset;
                child.capture_piece(parent, il, from, to, moved, moved,
                                    cap_sq, Pieces::makePiece(PieceType::PAWN, ~us));
                break;
            }
            case MoveType::CASTLING: {
                const Square rook_from = from == e1 ? (to == g1 ? h1 : a1) : (to == g8 ? h8 : a8);
                const Square rook_to   = us == Color::WHITE ? (rook_from == a1 ? d1 : f1)
                                                            : (rook_from == a8 ? d8 : f8);
                child.do_castling(parent, il, from, to, rook_from, rook_to, us);
                break;
            }
        }
        ++index_;
    }
    AccumulatorStack::AccumulatorStack() :stack_(std::make_unique<Accumulator[]>(ChessCore::Position::SEARCH_STACK_SIZE)) {}
}
