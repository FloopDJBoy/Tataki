//
// Created by FloopDjBoy on 06/09/2026.
//

#ifndef TATAKI_ACCUMULATOR_H
#define TATAKI_ACCUMULATOR_H
#include <array>
#include <cstdint>
#include <memory>

#include "nneu_types.h"
#include "Types.h"
#include "ChessCore/Position.h"

namespace Engine::Eval::NNEU {
    struct alignas(types::CACHE_LINE_SIZE) Accumulator {
        std::array<std::array<int16_t, types::HL_SIZE>,COLOR_NUMBER> vals;
        int16_t* operator[](const Color c){return vals[color_idx(c)].data();}
        const int16_t* operator[](const Color c) const {return vals[color_idx(c)].data();}
        int16_t operator[](const Color c,const int i) const {return vals[color_idx(c)][i];}
        void refresh(const ChessCore::Position& pos, const types::InputLayer& input_layer,Color c);
        void refresh(const ChessCore::Position& pos, const types::InputLayer& input_layer){refresh(pos,input_layer,Color::WHITE);refresh(pos,input_layer,Color::BLACK);};
        void remove_feature(const types::InputLayer &input_layer,  Square square, Piece p,  Color c);
        void add_piece(const types::InputLayer& l, const Square sq, const Piece p) {
            add_feature(l, sq, p, Color::WHITE);
            add_feature(l, sq, p, Color::BLACK);
        }
    private:
        void add_feature(const types::InputLayer &input_layer,  Square square, Piece p,  Color c);
    };
    class AccumulatorStack {
        std::unique_ptr<Accumulator[]> stack_;
        int index_ = 0;
        public:
        AccumulatorStack() : stack_(std::make_unique<Accumulator[]>(ChessCore::Position::SEARCH_STACK_SIZE)) {}
        Accumulator& top() {return stack_[index_];}
        [[nodiscard]] const Accumulator& top() const {return stack_[index_];}
        void push() {stack_[index_ + 1] = stack_[index_]; ++index_;}
        void reset() {index_ = 0;}
        void pop() {--index_;}
        void refresh(const ChessCore::Position& pos, const types::InputLayer& input_layer, const Color c){top().refresh(pos,input_layer,c);};
    };

}

#endif //TATAKI_ACCUMULATOR_H
