//
// Created by FloopDJBoy on 10/08/2026.
//

#ifndef CHESSENGINE_ZOBRIST_H
#define CHESSENGINE_ZOBRIST_H
#include <array>
#include "ChessCore/Pieces.h"
#include "Types.h"
#include "ChessCore/BitBoards.h"

namespace Engine::Zobrist  {
    constexpr size_t PIECE_KEYS = 12 * 64;

    struct Tables {
        std::array<Key, PIECE_KEYS> piece;
        std::array<Key, 16> castling;
        std::array<Key, 8> en_passant;
        Key side_to_move;
    };

    consteval Tables make_tables() {
        SplitMix64 rng(0x123456789ABCDEF0ULL);

        Tables tables{};

        for (auto& key : tables.piece)
            key = rng.next();

        for (auto& key : tables.castling)
            key = rng.next();

        for (auto& key : tables.en_passant)
            key = rng.next();

        tables.side_to_move = rng.next();

        return tables;
    }


    inline constexpr Tables tables = make_tables();

    constexpr Key piece_key(const Piece piece, const Square square) {
        return tables.piece[static_cast<size_t>(ChessCore::Pieces::zobrist_index(piece)) * 64 + static_cast<size_t>(square)];
    }

    constexpr Key castling_key(const CastlingRight rights) {
        Key key = 0;
        if ((rights & CastlingRight::WhiteKingSide)  != CastlingRight::None) key ^= tables.castling[0];
        if ((rights & CastlingRight::WhiteQueenSide) != CastlingRight::None) key ^= tables.castling[1];
        if ((rights & CastlingRight::BlackKingSide)  != CastlingRight::None) key ^= tables.castling[2];
        if ((rights & CastlingRight::BlackQueenSide) != CastlingRight::None) key ^= tables.castling[3];
        return key;
    }

    constexpr Key ep_key(const Square square) {
        return tables.en_passant[static_cast<size_t>(ChessCore::BitBoards::file_of(square))];
    }


}
#endif //CHESSENGINE_ZOBRIST_H
