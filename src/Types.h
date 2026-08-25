//
// Created by FloopDJBoy on 02/08/2026.
//

#pragma once
#include <array>
#include <cassert>
#include <cstdint>

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
constexpr Color operator~(const Color c) {
    return static_cast<Color>(static_cast<uint8_t>(c) ^ 0b1000u);
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
class SplitMix64 {
    uint64_t state;

public:
    explicit constexpr SplitMix64(const uint64_t seed) : state(seed) {}

    constexpr  uint64_t next() {
        uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }
};

using Piece = uint8_t;
using Square = uint8_t;
using BitBoard = uint64_t;
using Key = uint64_t;
using Score = int16_t;
using Value = int32_t;

constexpr int COLOR_NUMBER = 2;
constexpr int SQUARE_NUMBER = 64;
constexpr int PIECE_NUMBER = 15; //such that you can do arr[piece]
constexpr int PT_NUMBER = 7; //including empty



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
enum class Bound : uint8_t{
    EXACT,
    LOWER,
    UPPER
};

struct ScorePair {
    Score mg{0};
    Score eg{0};
    constexpr ScorePair() = default;
    constexpr ScorePair(const Score mg, const Score eg) : mg(mg), eg(eg) {}
    constexpr ScorePair operator+(ScorePair o) const { return {static_cast<Score>(mg + o.mg), static_cast<Score>(eg + o.eg)}; }
    constexpr ScorePair operator-(ScorePair o) const { return {static_cast<Score>(mg - o.mg), static_cast<Score>(eg - o.eg)}; }
    ScorePair& operator+=(const ScorePair o) { mg += o.mg; eg += o.eg; return *this; }
    ScorePair& operator-=(const ScorePair o) { mg -= o.mg; eg -= o.eg; return *this; }

    ScorePair operator*(const Score mult) const {return { static_cast<Score>(mg * mult), static_cast<Score>(eg * mult)};};
};
template<typename T,size_t MaxSize>
class DumbVector {
    T     values_[MaxSize];
    size_t size_ = 0;
    public:
    [[nodiscard]] size_t size() const { return size_; }
    [[nodiscard]] int   ssize() const { return static_cast<int>(size_); }
    void  push_back(const T& value) {
        assert(size_ < MaxSize);
        values_[size_++] = value;
    }
    const T* begin() const { return values_; }
    const T* end() const { return values_ + size_; }
    const T& operator[](int index) const { return values_[index]; }
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

