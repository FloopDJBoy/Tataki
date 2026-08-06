//
// Created by FloopDJBoy on 03/08/2026.
//

#ifndef CHESSENGINE_BOARD_H
#define CHESSENGINE_BOARD_H
#include <array>
#include <cassert>

#include "BitBoards.h"
#include "Pieces.h"
#include "Types.h"
namespace ChessCore {
    class Board {
        std::array<Piece, 64> pieces = []()-> std::array<Piece, 64> {
            std::array<Piece, 64> arr{};
            for (int i = 0; i < 64; i++) {
                arr[i] = Pieces::EMPTY;
            }
            return arr;
        }();
        public:
        //stores king squares {white,black}
        std::array<Square,2> king_squares{NO_SQUARE,NO_SQUARE};
        //stores per piece bitboard. order is based on the piece values aka Pieces::WHITE_PAWN give white pawns
        std::array<BitBoard,15> piece_bitboard = []() -> auto {
            std::array<BitBoard,15> arr{};
            for (int i = 0; i < 15; i++) {
                arr[i] = 0ull;
            }
            return arr;
        }();
        //stores where each colors pieces are {white,black}
        std::array<BitBoard,2> color_bitboard = {0ull,0ull};
        //where all pieces are
        BitBoard all_piece_bitboard = 0ull;


        [[nodiscard]] constexpr Piece get_piece(const Square square) const {return pieces[square];}
        constexpr void remove_piece(const Square square) {
            const Piece piece = pieces[square];

            if (piece == Pieces::EMPTY)
                return;

            const BitBoard bb = BitBoards::make_bitboard(square);

            pieces[square] = Pieces::EMPTY;

            piece_bitboard[piece] &= ~bb;

            const Color color = Pieces::getColor(piece);
            color_bitboard[color_idx(color)] &= ~bb;

            all_piece_bitboard &= ~bb;
        }
        constexpr void set_piece(const Square square, const Piece piece) {
            //assert(pieces[square] != Pieces::EMPTY);
            const BitBoard bb = BitBoards::make_bitboard(square);
            pieces[square] = piece;
            all_piece_bitboard |= bb;
            piece_bitboard[piece] |= bb;
            const Color c = Pieces::getColor(piece);
            color_bitboard[color_idx(c)] |= bb;
            if (Pieces::getType(piece) == PieceType::KING) {
                king_squares[color_idx(c)] = square;
            }
        }
        void move_piece(Square from, Square to);
        ~Board() = default;
    };

} // ChessCore

#endif //CHESSENGINE_BOARD_H
