//
// Created by FloopDJBoy on 02/08/2026.
//

#pragma once
#include <array>
#include <cstdint>
#include <tuple>

enum class PieceType : uint8_t {
    EMPTY = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6
};
enum class Color : uint8_t {
    WHITE = 0,
    BLACK = 8
};
enum class CastlingRight : uint8_t {
    None = 0,
   WhiteKingSide  = 1 << 0,
   WhiteQueenSide = 1 << 1,
   BlackKingSide  = 1 << 2,
   BlackQueenSide = 1 << 3,
};
constexpr CastlingRight operator|(CastlingRight a, CastlingRight b)
{
    return static_cast<CastlingRight>(
        static_cast<uint8_t>(a) |
        static_cast<uint8_t>(b)
    );
}
constexpr CastlingRight& operator|=(CastlingRight& a, CastlingRight b)
{
    a = static_cast<CastlingRight>(
        static_cast<uint8_t>(a) |
        static_cast<uint8_t>(b)
    );

    return a;
}
constexpr CastlingRight& operator&=(CastlingRight& a, CastlingRight b)
{
    a = static_cast<CastlingRight>(
        static_cast<uint8_t>(a) &
        static_cast<uint8_t>(b)
    );

    return a;
}
constexpr CastlingRight operator~(CastlingRight a) {
    return static_cast<CastlingRight>(~static_cast<uint8_t>(a));
}
constexpr CastlingRight operator&(CastlingRight a, CastlingRight b)
{
    return static_cast<CastlingRight>(
        static_cast<uint8_t>(a) &
        static_cast<uint8_t>(b)
    );
}
constexpr CastlingRight WHITE_CASTLING_MASK = CastlingRight::WhiteKingSide | CastlingRight::WhiteQueenSide;
constexpr CastlingRight BLACK_CASTLING_MASK = CastlingRight::BlackKingSide | CastlingRight::BlackQueenSide;
constexpr std::array CASTLING_MASK = {WHITE_CASTLING_MASK, BLACK_CASTLING_MASK};
constexpr Color operator~(const Color c)
{
    return c == Color::WHITE ? Color::BLACK : Color::WHITE;
}
constexpr int color_idx(const Color c) {
    return  c == Color::WHITE ? 0 : 1;
}
enum class MoveType : uint16_t {
    NORMAL,
    PROMOTION  = 1 << 14,
    EN_PASSANT = 2 << 14,
    CASTLING   = 3 << 14
};
using Piece = uint8_t;
using Square = uint8_t;
using BitBoard = uint64_t;


constexpr Square NO_SQUARE = 255;
enum SquareName : Square {
    a1,b1,c1,d1,e1,f1,g1,h1,
    a2,b2,c2,d2,e2,f2,g2,h2,
    a3,b3,c3,d3,e3,f3,g3,h3,
    a4,b4,c4,d4,e4,f4,g4,h4,
    a5,b5,c5,d5,e5,f5,g5,h5,
    a6,b6,c6,d6,e6,f6,g6,h6,
    a7,b7,c7,d7,e7,f7,g7,h7,
    a8,b8,c8,d8,e8,f8,g8,h8
};


constexpr bool is_valid_square(const Square s) {
    return s<64;
}
constexpr bool is_valid_cord(const int x, const int y) {
    return x >= 0 && x < 8 && y >= 0 && y < 8;
}
constexpr Square cord_to_square(const int x, const int y) {
    return static_cast<Square>(y * 8 + x);
}
constexpr std::pair<int,int> square_to_cord(const Square square) {
    return  {square%8,square/8};
}