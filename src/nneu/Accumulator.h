//
// Created by FloopDjBoy on 06/09/2026.
//

#ifndef TATAKI_ACCUMULATOR_H
#define TATAKI_ACCUMULATOR_H
#include <array>
#include <cstdint>
#include <memory>
#include <immintrin.h>
#include "nneu_types.h"
#include "Types.h"
#include "ChessCore/Move.h"

namespace ChessCore {
    class Position;
}

namespace Engine::Eval::NNUE {
    struct alignas(types::CACHE_LINE_SIZE) Accumulator {
        std::array<std::array<int16_t, types::HL_SIZE>,COLOR_NUMBER> vals;
        int16_t* operator[](const Color c){return vals[color_idx(c)].data();}
        const int16_t* operator[](const Color c) const {return vals[color_idx(c)].data();}
        int16_t operator[](const Color c,const int i) const {return vals[color_idx(c)][i];}
        void refresh(const ChessCore::Position& pos, const types::InputLayer& input_layer,Color c);
        void refresh(const ChessCore::Position& pos, const types::InputLayer& input_layer){refresh(pos,input_layer,Color::WHITE);refresh(pos,input_layer,Color::BLACK);};
        void add_piece(const types::InputLayer& l, const Square sq, const Piece p) {
            add_feature(l, sq, p, Color::WHITE);
            add_feature(l, sq, p, Color::BLACK);
        }
        void remove_piece(const types::InputLayer& l, const Square sq, const Piece p) {
            remove_feature(l, sq, p, Color::WHITE);
            remove_feature(l, sq, p, Color::BLACK);
        }


        void move_piece(const Accumulator& parent, const types::InputLayer& il,const Square from, const Square to,const Piece moved, const Piece placed) {
            using namespace types;
            for (const Color c : {Color::WHITE, Color::BLACK})
                add_sub(parent, il, feature_index(c, placed, to),
                                    feature_index(c, moved, from), c);
        }

        void capture_piece(const Accumulator& parent, const types::InputLayer& il,
                                        const Square from, const Square to,
                                        const Piece moved, const Piece placed,
                                        const Square cap_sq, const Piece captured) {
            using namespace types;
            for (const Color c : {Color::WHITE, Color::BLACK})
                add_sub_sub(parent, il, feature_index(c, placed, to),
                                        feature_index(c, moved, from),
                                        feature_index(c, captured, cap_sq), c);
        }

        void do_castling(const Accumulator& parent, const types::InputLayer& il,
                                      const Square king_from, const Square king_to,
                                      const Square rook_from, const Square rook_to, const Color us) {
            using namespace types;
            const Piece k = ChessCore::Pieces::makePiece(PieceType::KING, us);
            const Piece r = ChessCore::Pieces::makePiece(PieceType::ROOK, us);
            for (const Color c : {Color::WHITE, Color::BLACK})
                add_add_sub_sub(parent, il, feature_index(c, k, king_to),
                                            feature_index(c, r, rook_to),
                                            feature_index(c, k, king_from),
                                            feature_index(c, r, rook_from), c);
        }
    private:
        void add_feature(const types::InputLayer &input_layer,  Square square, Piece p,  Color c);
        void remove_feature(const types::InputLayer &input_layer,  Square square, Piece p,  Color c);
        void add_sub(const Accumulator& parent, const types::InputLayer& il,int add, int sub, Color c);
        void add_sub_sub(const Accumulator& parent, const types::InputLayer& il,int add,  int sub1,  int sub2,  Color c);
        void add_add_sub_sub(const Accumulator& parent, const types::InputLayer& il,int a1,  int a2,int s1,  int s2,  Color c);
    };
    class AccumulatorStack {
        std::unique_ptr<Accumulator[]> stack_;
        int index_ = 0;
    public:
        AccumulatorStack();
        Accumulator& top() {return stack_[index_];}
        [[nodiscard]] const Accumulator& top() const {return stack_[index_];}
        void push() {stack_[index_ + 1] = stack_[index_]; ++index_;}
        void reset() {index_ = 0;}
        void pop() {--index_;}
        void refresh(const ChessCore::Position& pos, const types::InputLayer& input_layer, const Color c){top().refresh(pos,input_layer,c);};
        void refresh(const ChessCore::Position& pos, const types::InputLayer& input_layer){top().refresh(pos,input_layer);};
        void push_null() {
            stack_[index_ + 1] = stack_[index_];   // accumulator is unchanged by a null move
            ++index_;
        }
        void push_move(const types::InputLayer& il, const ChessCore::Position& pos, ChessCore::Move m);

        AccumulatorStack(const AccumulatorStack& o) : AccumulatorStack() { stack_[0] = o.stack_[o.index_]; }
        AccumulatorStack& operator=(const AccumulatorStack& o) { stack_[0] = o.stack_[o.index_]; index_ = 0; return *this; }
        AccumulatorStack(AccumulatorStack&&) noexcept = default;
        AccumulatorStack& operator=(AccumulatorStack&&) noexcept = default;

    };
}

#endif //TATAKI_ACCUMULATOR_H
