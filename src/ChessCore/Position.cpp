//
// Created by FloopDJBoy on 03/08/2026.
//

#include "Position.h"

#include "FenHelper.h"
#include "magic_enum.hpp"
#include "Types.h"
#include "MoveGen.h"
#include "Engine/Zobrist.h"
#include "Engine/Eval.h"
#include "Engine/OpeningBook.h"

namespace ChessCore {
    using enum PieceType;
    using Pieces::makePiece;
    using namespace Engine;
    Position::Position(const std::string_view fen) : Position(FenHelper::fen_to_pos(fen)) {

    }
    std::string_view Position::fen() const {
        static thread_local std::string result;
        result.clear();

        // Board
        for (int rank = 7; rank >= 0; --rank) {
            int empty = 0;

            for (int file = 0; file < 8; ++file) {
                const auto s = static_cast<Square>(rank * 8 + file);
                const Piece p = square(s);

                if (p == Pieces::EMPTY) {
                    ++empty;
                    continue;
                }

                if (empty) {
                    result += static_cast<char>('0' + empty);
                    empty = 0;
                }

                result += Pieces::piece_to_symbol(p);
            }

            if (empty)
                result += static_cast<char>('0' + empty);

            if (rank != 0)
                result += '/';
        }

        // Side to move
        result += ' ';
        result += side_to_move() == Color::WHITE ? 'w' : 'b';

        // Castling rights
        result += ' ';

        bool has_castling = false;

        if (can_castle_kingside<Color::WHITE>()) {
            result += 'K';
            has_castling = true;
        }

        if (can_castle_queenside<Color::WHITE>()) {
            result += 'Q';
            has_castling = true;
        }

        if (can_castle_kingside<Color::BLACK>()) {
            result += 'k';
            has_castling = true;
        }

        if (can_castle_queenside<Color::BLACK>()) {
            result += 'q';
            has_castling = true;
        }

        if (!has_castling)
            result += '-';

        // En passant
        result += ' ';

        if (ep_square() == NO_SQUARE) {
            result += '-';
        } else {
            const int file = BitBoards::file_of(ep_square());
            const int rank = BitBoards::rank_of(ep_square());

            result += static_cast<char>('a' + file);
            result += static_cast<char>('1' + rank);
        }

        // Halfmove clock
        result += ' ';
        result += std::to_string(current_state_.half_clock);

        // Fullmove number
        result += ' ';
        result+=  std::to_string(fullmove_number_);

        return result;
    }
    //move is assume legal
    Position::Position(const Board board,const CastlingRight cr,const Square ep,const Color side,const int half_clock,const int full_move)
    : board(board),side_to_move_(side),ply_(0), fullmove_number_(full_move) {
        current_state_.castling_rights = cr;
        current_state_.ep_square = ep;
        current_state_.half_clock = half_clock;
        update_slider_blockers(Color::WHITE);
        update_slider_blockers(Color::BLACK);
        check_bb = attackers_to(king_square(side_to_move()), all_bb()) & color_bb(~side_to_move());
        current_state_.zobrist_key = zobrist_key(true);

        for (const Color c : {Color::WHITE, Color::BLACK}) {
            BitBoard bb = color_bb(c);
            while (bb) {
                const Square s = BitBoards::pop_lsb(bb);
                const Piece p = square(s);
                const auto ci = color_idx(c);

                current_state_.material_score[ci] +=Eval::evaluate_piece(p,s);
            }
        }
    }
    void Position::make_move(const Move move) {

        const Square from = move.from();
        const Square to = move.to();
        const Color us = side_to_move();
        const Color them = ~us;
        const MoveType type = move.get_type();
        const int offset = (us == Color::WHITE) ? 8 : -8;
        const auto moving = square(from);
        const Square captured_square = type == MoveType::EN_PASSANT ? to - offset : to;
        const auto captured = type == MoveType::EN_PASSANT? Pieces::makePiece(PAWN,them)  : square(to);

        //ref to old state
        const auto& st = push_state(move,captured);
        Key& key = current_state_.zobrist_key;

        bool half_clock_move = captured != Pieces::EMPTY;

        if (st.ep_square != NO_SQUARE)
            key ^= Engine::Zobrist::ep_key(st.ep_square);
        if (st.castling_rights != CastlingRight::None) {
            key ^= Engine::Zobrist::castling_key(st.castling_rights);
        }
        handle_castling_rights(move);

        if (current_state_.castling_rights != CastlingRight::None)
            key ^= Engine::Zobrist::castling_key(current_state_.castling_rights);

        board.move_piece(from, to);

        switch (type) {
            case MoveType::NORMAL:
                break;
            case MoveType::PROMOTION: {

                half_clock_move = true;
                const auto promo =  makePiece(move.promotion_type(), us);
                key ^= Engine::Zobrist::piece_key(moving, from);
                key ^= Engine::Zobrist::piece_key(promo, to);

                board.remove_piece(to);
                board.set_piece(to,promo);
                current_state_.material_score[color_idx(us)] +=  Eval::evaluate_piece(promo,to) - Eval::evaluate_piece(moving,from);
                current_state_.phase += Eval::phase_value(move.promotion_type());
                break;
            }
            case MoveType::CASTLING: {
                assert(from == e1 ||from == e8);
                assert(to == g1 ||to == c1 ||to == g8 ||to == c8);
                const Square rook_start =
                    from == e1 ? (to == g1 ? h1 : a1)
                               : (to == g8 ? h8 : a8);

                const Square rook_end =
                    us == Color::WHITE
                        ? (rook_start == a1 ? d1 : f1)
                        : (rook_start == a8 ? d8 : f8);

                assert(Pieces::getType(square(rook_start)) == PieceType::ROOK);


                board.remove_piece(rook_start);
                board.set_piece(rook_end, Pieces::makePiece(PieceType::ROOK, us));
                const Piece rook = Pieces::makePiece(PieceType::ROOK, us);
                current_state_.material_score[color_idx(us)] +=Eval::evaluate_piece(rook, rook_end) -Eval::evaluate_piece(rook, rook_start);
                key ^= Engine::Zobrist::piece_key(rook, rook_start);
                key ^= Engine::Zobrist::piece_key(rook, rook_end);
                break;
            }
            case MoveType::EN_PASSANT: {
                half_clock_move = true;
                board.remove_piece(to - offset);
                break;
            }
        }

        if (Pieces::getType(square(to)) == PAWN) {
            half_clock_move = true;
            current_state_.ep_square = (std::abs(to - from) == 16) ? to - offset : NO_SQUARE;
        } else {
            current_state_.ep_square = NO_SQUARE;
        }
        if (current_state_.ep_square != NO_SQUARE)
            key ^= Engine::Zobrist::ep_key(current_state_.ep_square);

        current_state_.half_clock = half_clock_move ? 0 : current_state_.half_clock + 1;
        if (captured != Pieces::EMPTY) {
            key ^= Zobrist::piece_key(captured, captured_square);
            current_state_.material_score[color_idx(them)] -= Eval::evaluate_piece(captured, captured_square);
            current_state_.phase -= Eval::phase_value(Pieces::getType(captured));
        }
        if (type != MoveType::PROMOTION) {
            current_state_.material_score[color_idx(us)] += Eval::evaluate_piece(moving,to) - Eval::evaluate_piece(moving,from);
            key ^= Zobrist::piece_key(moving, from);
            key ^= Zobrist::piece_key(moving, to);
        }
        if (us == Color::BLACK) {
            ++fullmove_number_;
        }
        swap_side();
        key ^= Engine::Zobrist::tables.side_to_move;
        update_slider_blockers(side_to_move());
        check_bb = attackers_to(king_square(side_to_move()), all_bb()) &
                   color_bb(~side_to_move());

        repetition = 0;
        if (current_state_.half_clock >= 4) {
            for (int p = ply() - 2, i = 4;i <= current_state_.half_clock;p -= 2, i += 2) {
                if (history[p].zobrist_key == key) {
                    ++repetition;

                    if (repetition >= 2)
                        break;
                }
            }
        }
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
        //auto cb =  color_bb(~side_to_move());

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
    inline const StateInfo& Position::push_state(Move move, Piece captured)
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
    Move Position::parse_move(const std::string& move_string) const {
        auto from_name = magic_enum::enum_cast<SquareName>(move_string.substr(0,2));
        auto to_name   = magic_enum::enum_cast<SquareName>(move_string.substr(2,2));

        if (!from_name || !to_name)
            return Move::none();

        Square from = *from_name;
        Square to   = *to_name;

        PieceType promo = EMPTY;

        if (move_string.size() == 5)
            promo = Pieces::getType(Pieces::symbol_to_piece(move_string[4]));

        return parse_move(from, to, promo);
    }
    Move Position::parse_move(const Square from, const Square to,const PieceType promo) const {
        if (promo != EMPTY) {
            return Move::make(from, to, promo);
        }

        // Castling
        if (Pieces::getType(square(from)) == KING) {
            if (abs(to - from) == 2) {
                return Move::make<MoveType::CASTLING>(from, to);
            }

            if (from == e1 && to == a1) {
                return Move::make<MoveType::CASTLING>(from, c1);
            }

            if (from == e1 && to == h1) {
                return Move::make<MoveType::CASTLING>(from, g1);
            }

            if (from == e8 && to == a8) {
                return Move::make<MoveType::CASTLING>(from, c8);
            }

            if (from == e8 && to == h8) {
                return Move::make<MoveType::CASTLING>(from, g8);
            }
        }

        // En passant
        if (Pieces::getType(square(from)) == PAWN) {
            if (to == ep_square())
                return Move::make<MoveType::EN_PASSANT>(from, to);
        }

        return Move(from, to);
    }
    Key Position::polyglot_hash() const {
        Key key = 0;

        // Pieces
        for (Square s = 0; s < 64; ++s) {
            Piece p = square(s);

            if (p != Pieces::EMPTY) {
                const int index = Pieces::polyglot_index(p);
                key ^= OpeningBook::OpeningBook::POLYGLOT_RANDOM[64 * index + s];
            }
        }

        // Castling rights
        if (can_castle_kingside<Color::WHITE>())
            key ^= OpeningBook::POLYGLOT_RANDOM[768];

        if (can_castle_queenside<Color::WHITE>())
            key ^= OpeningBook::POLYGLOT_RANDOM[769];

        if (can_castle_kingside<Color::BLACK>())
            key ^= OpeningBook::POLYGLOT_RANDOM[770];

        if (can_castle_queenside<Color::BLACK>())
            key ^= OpeningBook::POLYGLOT_RANDOM[771];

        // En passant
        // En passant
        if (ep_square() != NO_SQUARE) {
            const Square ep = ep_square();

            if (side_to_move() == Color::WHITE) {
                const auto pawns =
                    BitBoards::get_single_pawn_attacks<Color::BLACK>(ep)
                    & piece_bb(Pieces::WHITE_PAWN);

                if (pawns)
                    key ^= OpeningBook::POLYGLOT_RANDOM[772 + BitBoards::file_of(ep)];
            } else {
                const auto pawns =
                    BitBoards::get_single_pawn_attacks<Color::WHITE>(ep)
                    & piece_bb(Pieces::BLACK_PAWN);

                if (pawns)
                    key ^= OpeningBook::POLYGLOT_RANDOM[772 + BitBoards::file_of(ep)];
            }
        }

        // Side to move
        if (side_to_move() == Color::WHITE)
            key ^= OpeningBook::POLYGLOT_RANDOM[780];

        return key;
    }

    Key Position::zobrist_key(const bool from_scratch) const {
        if (!from_scratch) {
            return state().zobrist_key;
        }
        Key key = 0;

        // Pieces
        for (Square s = 0; s < 64; ++s) {
            const Piece p = square(s);

            if (p != Pieces::EMPTY) {
                const int index = Pieces::polyglot_index(p);
                key ^= Zobrist::tables.piece[64 * index + s];
            }
        }


        // Castling rights
        if (can_castle_kingside<Color::WHITE>())
            key ^= Zobrist::tables.castling[0];

        if (can_castle_queenside<Color::WHITE>())
            key ^= Zobrist::tables.castling[1];

        if (can_castle_kingside<Color::BLACK>())
            key ^= Zobrist::tables.castling[2];

        if (can_castle_queenside<Color::BLACK>())
            key ^= Zobrist::tables.castling[3];

        // En passant
        if (ep_square() != NO_SQUARE) {
            key ^= Zobrist::tables.en_passant[BitBoards::file_of(ep_square())];
        }

        // Side to move
        if (side_to_move() == Color::WHITE)
            key ^= Zobrist::tables.side_to_move;

        return key;
    }
} // ChessCore