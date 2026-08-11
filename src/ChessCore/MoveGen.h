//
// Created by FloopDJBoy on 04/08/2026.
//

#pragma once
#include "Types.h"
#include "BitBoards.h"
#include "Move.h"
#include "Pieces.h"
#include <algorithm>

namespace ChessCore {
    class Position;
}

namespace ChessCore::MoveGen {
    enum class GenType {
        CAPTURES,
        //no captures
        QUIETS,
        //Generate only moves that get the king out of check.
        EVASIONS,
        //Generate normal moves assuming the king is not currently in check.
        NON_EVASIONS,
        LEGAL
    };
    template<GenType type>
    Move* generate(const Position& pos,Move* moveList);
    template<GenType genType>
    struct MoveList {
        private:
        constexpr static int MAX_MOVES = 256;
        Move moveList[MAX_MOVES];
        Move* last;
        public:
        [[nodiscard]] const Move* begin() const {return moveList;}
        [[nodiscard]] const Move* end() const {return last;}
        [[nodiscard]] Move* begin() {return moveList;}
        [[nodiscard]] Move* end() {return last;}
        Move& operator[](const uint16_t index) { return moveList[index]; }
        const Move& operator[](const uint16_t index) const { return moveList[index]; }
        [[nodiscard]] uint16_t size() const {return last-moveList;}
        [[nodiscard]] bool contains(const Move m) const {return std::find(begin(),end(),m) != end();}
        explicit MoveList(const Position& pos) : last(generate<genType>(pos,moveList)) {};
        [[nodiscard]] bool empty() const {return last == moveList;}
        MoveList(const MoveList&) = delete;
        MoveList& operator=(const MoveList&) = delete;

    };


} // ChessCore
