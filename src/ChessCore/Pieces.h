#pragma once
#include <cstdint>
#include <stdexcept>
#include <utility>

#include "../Types.h"
namespace ChessCore::Pieces {
    //Piece representation
    //last 3 bits represents the type
    //first bit  represents the color
    //so 0bCTTT
    //C = color
    //T = type
    constexpr uint8_t pieceMask = 0b0111;
    constexpr uint8_t colorMask = 0b1000;


    constexpr Piece makePiece(const PieceType piece, const Color color) {
        return static_cast<Piece>(static_cast<uint8_t>(piece) | static_cast<uint8_t>(color));
    }
    
    constexpr Piece makePiece(const PieceType piece, const bool isWhiteToMove) {
        return makePiece(piece,isWhiteToMove ? Color::WHITE : Color::BLACK);
    }
    constexpr PieceType getType(const Piece piece) {
        return static_cast<PieceType>(piece & pieceMask);
    }
    constexpr Color getColor(const Piece piece) {
        return static_cast<Color>(piece & colorMask);
    }
    constexpr Piece EMPTY = makePiece(PieceType::EMPTY, Color::WHITE);
    constexpr Piece WHITE_PAWN   = makePiece(PieceType::PAWN, Color::WHITE);
    constexpr Piece WHITE_ROOK   = makePiece(PieceType::ROOK, Color::WHITE);
    constexpr Piece WHITE_KNIGHT = makePiece(PieceType::KNIGHT, Color::WHITE);
    constexpr Piece WHITE_BISHOP = makePiece(PieceType::BISHOP, Color::WHITE);
    constexpr Piece WHITE_QUEEN  = makePiece(PieceType::QUEEN, Color::WHITE);
    constexpr Piece WHITE_KING   = makePiece(PieceType::KING, Color::WHITE);

    constexpr Piece BLACK_PAWN   = makePiece(PieceType::PAWN, Color::BLACK);
    constexpr Piece BLACK_ROOK   = makePiece(PieceType::ROOK, Color::BLACK);
    constexpr Piece BLACK_KNIGHT = makePiece(PieceType::KNIGHT, Color::BLACK);
    constexpr Piece BLACK_BISHOP = makePiece(PieceType::BISHOP, Color::BLACK);
    constexpr Piece BLACK_QUEEN  = makePiece(PieceType::QUEEN, Color::BLACK);
    constexpr Piece BLACK_KING   = makePiece(PieceType::KING, Color::BLACK);
    constexpr char piece_to_symbol(const Piece p)
    {
        switch (p)
        {
            case WHITE_PAWN:   return 'P';
            case WHITE_ROOK:   return 'R';
            case WHITE_KNIGHT: return 'N';
            case WHITE_BISHOP: return 'B';
            case WHITE_QUEEN:  return 'Q';
            case WHITE_KING:   return 'K';

            case BLACK_PAWN:   return 'p';
            case BLACK_ROOK:   return 'r';
            case BLACK_KNIGHT: return 'n';
            case BLACK_BISHOP: return 'b';
            case BLACK_QUEEN:  return 'q';
            case BLACK_KING:   return 'k';

            case EMPTY:        return '.';
            default:           throw std::invalid_argument("invalid piece");
        }
    }
    constexpr Piece symbol_to_piece(const char c)
    {
        switch (c)
        {
            case 'P': return WHITE_PAWN;
            case 'R': return WHITE_ROOK;
            case 'N': return WHITE_KNIGHT;
            case 'B': return WHITE_BISHOP;
            case 'Q': return WHITE_QUEEN;
            case 'K': return WHITE_KING;

            case 'p': return BLACK_PAWN;
            case 'r': return BLACK_ROOK;
            case 'n': return BLACK_KNIGHT;
            case 'b': return BLACK_BISHOP;
            case 'q': return BLACK_QUEEN;
            case 'k': return BLACK_KING;

            case '.': return EMPTY;

            default:
                throw std::invalid_argument("invalid piece symbol");
        }
    }
    constexpr int polyglot_index(const Piece p) {
        switch (p) {
            case BLACK_PAWN:   return 0;
            case WHITE_PAWN:   return 1;

            case BLACK_KNIGHT: return 2;
            case WHITE_KNIGHT: return 3;

            case BLACK_BISHOP: return 4;
            case WHITE_BISHOP: return 5;

            case BLACK_ROOK:   return 6;
            case WHITE_ROOK:   return 7;

            case BLACK_QUEEN:  return 8;
            case WHITE_QUEEN:  return 9;

            case BLACK_KING:   return 10;
            case WHITE_KING:   return 11;

            default: return -1;
        }
    }
    constexpr Piece from_polyglot_index(const int index) {
        switch (index) {
            case 0:  return WHITE_PAWN;
            case 1:  return WHITE_KNIGHT;
            case 2:  return WHITE_BISHOP;
            case 3:  return WHITE_ROOK;
            case 4:  return WHITE_QUEEN;
            case 5:  return WHITE_KING;

            case 6:  return BLACK_PAWN;
            case 7:  return BLACK_KNIGHT;
            case 8:  return BLACK_BISHOP;
            case 9:  return BLACK_ROOK;
            case 10: return BLACK_QUEEN;
            case 11: return BLACK_KING;

            default: return EMPTY;
        }
    }
    constexpr Piece zobrist_index(const Piece p) {
        return polyglot_index(p);
    }




}
