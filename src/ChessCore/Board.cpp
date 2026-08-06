//
// Created by FloopDJBoy on 03/08/2026.
//

#include "Board.h"
namespace ChessCore {
     void Board::move_piece(const Square from, const Square to) {
        const Piece moving = pieces[from];
        const Piece captured = pieces[to];
         assert(moving != Pieces::EMPTY);

        const Color color = Pieces::getColor(moving);

        const BitBoard fromBB = BitBoards::make_bitboard(from);
        const BitBoard toBB   = BitBoards::make_bitboard(to);

        if (captured != Pieces::EMPTY) {
            piece_bitboard[captured] &= ~toBB;
            color_bitboard[color_idx(Pieces::getColor(captured))] &= ~toBB;
        }

        pieces[from] = Pieces::EMPTY;
        pieces[to] = moving;

        const BitBoard moveBB = fromBB | toBB;

        piece_bitboard[moving] ^= moveBB;
        color_bitboard[color_idx(color)] ^= moveBB;

        all_piece_bitboard ^= fromBB;
        all_piece_bitboard |= toBB;

        if (Pieces::getType(moving) == PieceType::KING)
            king_squares[color_idx(color)] = to;
    }
} // ChessCore