//
// Created by FloopDJBoy on 03/08/2026.
//

#include "Position.h"
#include "Types.h"
#include "MoveGen.h"
namespace ChessCore {
    using enum PieceType;
    using Pieces::makePiece;
    //move is assume legal
    void Position::make_move(const Move move) {

        const Square from = move.from();
        const Square to = move.to();
        const Color stm = side_to_move();
        const MoveType type = move.get_type();
        const int offset = (stm == Color::WHITE) ? 8 : -8;
        const auto captured = square(to);

        auto& st = push_state(move,captured);

        bool half_clock_move = captured != Pieces::EMPTY;
        handle_castling_rights(move);
        board.move_piece(from, to);

        switch (type) {
            case MoveType::NORMAL:
                break;

            case MoveType::PROMOTION:
                half_clock_move = true;
                board.remove_piece(to);
                board.set_piece(to, makePiece(move.promotion_type(), stm));
                break;

            case MoveType::CASTLING: {
                assert(from == e1 ||from == e8);
                assert(
                    to == g1 ||
                    to == c1 ||
                    to == g8 ||
                    to == c8
                );
                const Square rook_start =
                    from == e1 ? (to == g1 ? h1 : a1)
                               : (to == g8 ? h8 : a8);

                const Square rook_end =
                    stm == Color::WHITE
                        ? (rook_start == a1 ? d1 : f1)
                        : (rook_start == a8 ? d8 : f8);

                assert(Pieces::getType(square(rook_start)) == PieceType::ROOK);

                board.remove_piece(rook_start);
                board.set_piece(rook_end, Pieces::makePiece(PieceType::ROOK, stm));
                break;
            }

            case MoveType::EN_PASSANT:
                half_clock_move = true;
                board.remove_piece(to - offset);
                st.captured = makePiece(PAWN,~side_to_move());

                break;
        }

        if (Pieces::getType(square(to)) == PAWN) {
            half_clock_move = true;
            current_state_.ep_square = (std::abs(to - from) == 16) ? to - offset : NO_SQUARE;
        } else {
            current_state_.ep_square = NO_SQUARE;
        }


        current_state_.half_clock = half_clock_move ? 0 : current_state_.half_clock + 1;

        swap_side();
        update_slider_blockers(side_to_move());
        check_bb = attackers_to(king_square(side_to_move()), all_bb()) &
                   color_bb(~side_to_move());
    }

    void Position::undo_move() {
        --ply_;

        const StateInfo& st = history[ply_];

        const Move move = st.move;
        const Square from = move.from();
        const Square to = move.to();

        // Current side is the side that is about to move.
        // The side that made the move is the opposite.
        swap_side();
        const Color stm = side_to_move();

        const MoveType type = move.get_type();
        const int offset = (stm == Color::WHITE) ? 8 : -8;

        // Restore non-board state
        current_state_ = st;

        // Undo special moves first
        switch (type) {
            case MoveType::NORMAL:
                break;

            case MoveType::PROMOTION:
                // Remove promoted piece and restore pawn
                board.remove_piece(to);
                board.set_piece(to, makePiece(PieceType::PAWN, stm));
                break;

            case MoveType::CASTLING: {
                const Square rook_start =
                     from == e1 ? (to == g1 ? h1 : a1)
                   : (to == g8 ? h8 : a8);

                const Square rook_end =
                    stm == Color::WHITE
                        ? (rook_start == a1 ? d1 : f1)
                        : (rook_start == a8 ? d8 : f8);

                board.move_piece(rook_end, rook_start);
                break;
            }

            case MoveType::EN_PASSANT:
                // Restore captured pawn
                board.set_piece(
                    to - offset,
                    makePiece(PieceType::PAWN, ~stm)
                );
                break;
        }

        // Undo the actual piece movement
        board.move_piece(to, from);

        // Restore normal captured piece
        if (st.captured != Pieces::EMPTY &&
            type != MoveType::EN_PASSANT) {
            board.set_piece(to, st.captured);
            }

        update_slider_blockers(side_to_move());

        check_bb = attackers_to(
            king_square(side_to_move()),
            all_bb()
        ) & color_bb(~side_to_move());
    }

    void Position::handle_castling_rights(const Move move) {
        const Square from = move.from();
        const Square to = move.to();

        if (Pieces::getType(square(from)) == PieceType::KING)
            current_state_.castling_rights &= ~CASTLING_MASK[color_idx(side_to_move())];

        if (from == a1 || to == a1)
            current_state_.castling_rights &= ~CastlingRight::WhiteQueenSide;

        if (from == h1 || to == h1)
            current_state_.castling_rights &= ~CastlingRight::WhiteKingSide;

        if (from == a8 || to == a8)
            current_state_.castling_rights &= ~CastlingRight::BlackQueenSide;

        if (from == h8 || to == h8)
            current_state_.castling_rights &= ~CastlingRight::BlackKingSide;
    }
    //ui func do not call in engine
    std::set<Square> Position::get_moves_squares(const Square s) const {
        auto set = std::set<Square>();
        MoveGen::MoveList<MoveGen::GenType::LEGAL> moveList(*this);
        for (const auto m : moveList) {
            if (m.from() == s) {
                set.insert(m.to());
            }
        }
        return set;
    }
    //ui func do not call in engine
    bool Position::try_make_move(const Move move) {
        MoveGen::MoveList<MoveGen::GenType::LEGAL> moveList(*this);
        for (const auto s : moveList) {
            if (move.from() == s.from() && move.to() == s.to()) {
                make_move(s);
                return true;
            }
        }
        return false;
    }
    bool Position::legal(Move move) const {
        const Square from = move.from();
        const Square to = move.to();
        auto cb =  color_bb(~side_to_move());

        if (move.get_type() == MoveType::CASTLING) {
            const auto dir = (to - from)<0 ? -1 : 1;
            for(Square s = from; ; s+=dir)
            {
                if (attackers_to(s,all_bb()) & color_bb(~side_to_move()))
                    return false;

                if (s == to)
                    break;
            }
            return true;
        }
        if (Pieces::getType(square(from)) == PieceType::KING) {
            return (attackers_to(to,(all_bb() ^ BitBoards::make_bitboard(from))) & color_bb(~side_to_move()))==0;
        }
        return !(blockers_for_king(side_to_move()) & BitBoards::make_bitboard(from)) || (BitBoards::line_bb[from][to]) & piece_bb(PieceType::KING,side_to_move());
    }
    inline StateInfo& Position::push_state(Move move, Piece captured)
    {
        StateInfo& st = history[ply_++];
        st = current_state_;
        st.move = move;
        st.captured = captured;
        return st;
    }
    void Position::update_slider_blockers(Color c)
    {
        blockers_for_king_[color_idx(c)] = 0;

        Square ksq = king_square(c);

        BitBoard sliders =
            (
                BitBoards::get_attacks_bb<PieceType::ROOK>(ksq, 0) &
                (piece_bb(PieceType::ROOK) | piece_bb(PieceType::QUEEN))
            |
                BitBoards::get_attacks_bb<PieceType::BISHOP>(ksq, 0) &
                (piece_bb(PieceType::BISHOP) | piece_bb(PieceType::QUEEN))
            ) & color_bb(~c);

        BitBoard occupancy = all_bb() & ~sliders;

        while (sliders)
        {
            Square s = BitBoards::pop_lsb(sliders);

            BitBoard blockers =
                BitBoards::line_between[ksq][s] & occupancy;

            if (blockers && !BitBoards::more_than_one(blockers))
                blockers_for_king_[color_idx(c)] |= blockers;
        }
    }

    BitBoard Position::attackers_to(Square s, BitBoard occupancy) const
    {
        const auto rook = BitBoards::get_attacks_bb<PieceType::ROOK>(s, occupancy);
        const auto bishop = BitBoards::get_attacks_bb<PieceType::BISHOP>(s, occupancy);
        //const auto p1 = BitBoards::get_single_pawn_attacks<Color::BLACK>(s);
        //const auto p2 = BitBoards::get_single_pawn_attacks<Color::WHITE>(s);
        //const auto kh = BitBoards::get_attacks_bb<PieceType::KNIGHT>(s) & piece_bb(PieceType::KNIGHT);
        //const auto k = BitBoards::get_attacks_bb<PieceType::KING>(s) & piece_bb(PieceType::KING);
        return
            (rook & (piece_bb(PieceType::ROOK) | piece_bb(PieceType::QUEEN)))
            |
            (bishop & (piece_bb(PieceType::BISHOP) | piece_bb(PieceType::QUEEN)))
            |
            (BitBoards::get_single_pawn_attacks<Color::BLACK>(s) & piece_bb(Pieces::WHITE_PAWN))
            |
            (BitBoards::get_single_pawn_attacks<Color::WHITE>(s) & piece_bb(Pieces::BLACK_PAWN))
            |
            (BitBoards::get_attacks_bb<PieceType::KNIGHT>(s) & piece_bb(PieceType::KNIGHT))
            |
            (BitBoards::get_attacks_bb<PieceType::KING>(s) & piece_bb(PieceType::KING));
    }
} // ChessCore