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
#include "Engine/PawnHash.h"

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

    Position::Position(const Board board,const CastlingRight cr,const Square ep,const Color side,const int half_clock,const int full_move)
    : board(board),side_to_move_(side),ply_(0), fullmove_number_(full_move), current_state_{} {
        current_state_.castling_rights = cr;
        current_state_.ep_square = ep;
        current_state_.half_clock = half_clock;
        update_check_info();
        check_bb = attackers_to(king_square(side_to_move()), all_bb()) & color_bb(~side_to_move());
        current_state_.zobrist_key = zobrist_key(true);
        current_state_.pawn_key = 0;
        current_state_.non_pawn_material = {0,0};
        current_state_.phase = 0;

        for (const Color c : {Color::WHITE, Color::BLACK}) {
            BitBoard bb = color_bb(c);
            while (bb) {
                const Square s = BitBoards::pop_lsb(bb);
                const Piece p = square(s);
                const auto ci = color_idx(c);

                current_state_.material_score[ci] +=Eval::evaluate_piece(p,s);
                current_state_.phase += Eval::phase_value(Pieces::getType(p));
                if (Pieces::getType(p) == PAWN) {
                    current_state_.pawn_key ^= PawnHash::hash(c,s);
                }else if (Pieces::getType(p) != KING) {
                    current_state_.non_pawn_material[ci] += Eval::piece_value(p);
                }
            }
        }

    }
    //move is assume legal
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
        current_state_.captured = captured;
        Key& zobrist_key = current_state_.zobrist_key;
        Key& pawn_key = current_state_.pawn_key;

        bool half_clock_move = captured != Pieces::EMPTY;

        if (st.ep_square != NO_SQUARE)
            zobrist_key ^= Engine::Zobrist::ep_key(st.ep_square);
        if (st.castling_rights != CastlingRight::None) {
            zobrist_key ^= Engine::Zobrist::castling_key(st.castling_rights);
        }
        handle_castling_rights(move);

        if (current_state_.castling_rights != CastlingRight::None)
            zobrist_key ^= Engine::Zobrist::castling_key(current_state_.castling_rights);

        board.move_piece(from, to);

        switch (type) {
            case MoveType::NORMAL:
                break;
            case MoveType::PROMOTION: {

                half_clock_move = true;
                const auto promo =  makePiece(move.promotion_type(), us);
                zobrist_key ^= Engine::Zobrist::piece_key(moving, from);
                zobrist_key ^= Engine::Zobrist::piece_key(promo, to);

                current_state_.non_pawn_material[color_idx(us)] += Eval::piece_value(promo);

                pawn_key ^= Engine::PawnHash::hash(us,from);

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
                zobrist_key ^= Engine::Zobrist::piece_key(rook, rook_start);
                zobrist_key ^= Engine::Zobrist::piece_key(rook, rook_end);
                break;
            }
            case MoveType::EN_PASSANT: {
                half_clock_move = true;
                board.remove_piece(to - offset);
                break;
            }
        }

        if (Pieces::getType(moving) == PAWN) {
            half_clock_move = true;
            current_state_.ep_square = (std::abs(to - from) == 16) ? to - offset : NO_SQUARE;
        } else {
            current_state_.ep_square = NO_SQUARE;
        }
        if (current_state_.ep_square != NO_SQUARE)
            zobrist_key ^= Engine::Zobrist::ep_key(current_state_.ep_square);

        current_state_.half_clock = half_clock_move ? 0 : current_state_.half_clock + 1;
        if (captured != Pieces::EMPTY) {
            zobrist_key ^= Zobrist::piece_key(captured, captured_square);
            current_state_.material_score[color_idx(them)] -= Eval::evaluate_piece(captured, captured_square);
            current_state_.phase -= Eval::phase_value(Pieces::getType(captured));

            pawn_key ^= PawnHash::hash(them,captured_square);

            if (Pieces::getType(captured) == PAWN) {
                current_state_.non_pawn_material[color_idx(them)] -= Eval::piece_value(captured);
            }
        }
        if (type != MoveType::PROMOTION) {
            current_state_.material_score[color_idx(us)] += Eval::evaluate_piece(moving,to) - Eval::evaluate_piece(moving,from);
            zobrist_key ^= Zobrist::piece_key(moving, from);
            zobrist_key ^= Zobrist::piece_key(moving, to);
            if(Pieces::getType(moving) == PAWN) {
                pawn_key ^= PawnHash::hash(us,from);
                pawn_key ^= PawnHash::hash(us,to);
            }
        }
        if (us == Color::BLACK) {
            ++fullmove_number_;
        }
        swap_side();
        zobrist_key ^= Engine::Zobrist::tables.side_to_move;
        update_check_info();
        check_bb = attackers_to(king_square(side_to_move()), all_bb()) &
                   color_bb(~side_to_move());

        current_state_.repetition = 0;
        if (current_state_.half_clock >= 4) {
            for (int p = ply_ - 4, i = 4; i <= current_state_.half_clock && p >= 0; p -= 2, i += 2) {
                if (history[p].zobrist_key == zobrist_key) {
                    ++current_state_.repetition;
                    if (current_state_.repetition >= 1)
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

        update_check_info();
        check_bb = attackers_to(
            king_square(side_to_move()),
            all_bb()
        ) & color_bb(~side_to_move());
    }

    void Position::make_null_move() {
        assert(!in_check());
        const auto& st = push_state(Move::null(),Pieces::EMPTY);
        Key& zobrist_key = current_state_.zobrist_key;
        Key& pawn_key = current_state_.pawn_key;

        if (st.ep_square != NO_SQUARE) {
            current_state_.ep_square = NO_SQUARE;
            zobrist_key ^= Zobrist::ep_key(st.ep_square);
        }
        zobrist_key ^= Zobrist::tables.side_to_move;
        swap_side();
        update_check_info();
        check_bb = attackers_to(king_square(side_to_move()), all_bb()) &
                   color_bb(~side_to_move());
        current_state_.repetition = 0;
    }

    void Position::undo_null_move() {
        --ply_;
        const StateInfo& st = history[ply_];
        current_state_ = st;
        swap_side();
        update_check_info();
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


    bool Position::legal(const Move move) const {
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


    inline const StateInfo& Position::push_state(const Move move, const Piece captured)
    {
        StateInfo& st = history[ply_++];
        st = current_state_;
        st.move = move;
        st.captured = captured;
        return st;
    }
    void Position::update_slider_blockers(const Color c)
    {
        blockers_for_king_[color_idx(c)] = 0;
        pinners_[color_idx(~c)] = 0;

        const Square ksq = king_square(c);

        BitBoard sliders =
            (
                BitBoards::get_attacks_bb<PieceType::ROOK>(ksq, 0) &
                (piece_bb(PieceType::ROOK) | piece_bb(PieceType::QUEEN))
            |
                BitBoards::get_attacks_bb<PieceType::BISHOP>(ksq, 0) &
                (piece_bb(PieceType::BISHOP) | piece_bb(PieceType::QUEEN))
            ) & color_bb(~c);

        const BitBoard occupancy = all_bb() & ~sliders;

        while (sliders)
        {
            const Square s = BitBoards::pop_lsb(sliders);

            const BitBoard blockers =
                BitBoards::line_between[ksq][s] & occupancy;

            if (blockers && !BitBoards::more_than_one(blockers)) {
                blockers_for_king_[color_idx(c)] |= blockers;
                if (blockers & color_bb(c)) {
                    pinners_[color_idx(~c)] |= s;
                }
            }
        }
    }

    void Position::update_check_info() {
        using namespace BitBoards;
        update_slider_blockers(Color::WHITE);
        update_slider_blockers(Color::BLACK);

        const Square ksq = king_square(side_to_move());
        const BitBoard bishop = get_attacks_bb<BISHOP>(ksq,all_bb());
        const BitBoard rook = get_attacks_bb<ROOK>(ksq,all_bb());


        check_squares_[static_cast<int>(PAWN)] = side_to_move() == Color::WHITE ? get_single_pawn_attacks<Color::BLACK>(ksq)
                                                                            : get_single_pawn_attacks<Color::WHITE>(ksq);
        check_squares_[static_cast<int>(KNIGHT)] = get_attacks_bb<KNIGHT>(ksq);
        check_squares_[static_cast<int>(BISHOP)] = bishop;
        check_squares_[static_cast<int>(ROOK)] = rook;
        check_squares_[static_cast<int>(QUEEN)] = rook | bishop;
        check_squares_[static_cast<int>(KING)] = 0;


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

    bool Position::see_ge(const Move move, const int threshold ) const {
        using BitBoards::clear_square;
        using BitBoards::toggle_square;
        using BitBoards::get_attacks_bb;

        //assume promo capture, ep passes
        if (move.get_type() != MoveType::NORMAL) {
            return 0>= threshold;
        }

        const Square from=move.from(),to = move.to();

        int max_value = Eval::piece_value(square(move.to())) - threshold;
        if (max_value < 0) {
            return false;
        }
        max_value = Eval::piece_value(square(from)) - max_value;
        //if the value of our piece is lower then theirs we always win. ie pawn takes queen
        if (max_value <= 0) {
            return true;
        }

        BitBoard occupancy = all_bb();
        //do first capture
        occupancy = clear_square(occupancy,from);
        occupancy = clear_square(occupancy,to);

        Color stm = side_to_move();
        BitBoard attackers = attackers_to(to, occupancy);
        int res = 1;

        BitBoard dig = piece_bb(BISHOP) | piece_bb(QUEEN);
        BitBoard ortho = piece_bb(ROOK) | piece_bb(QUEEN);


        while (true) {
            stm = ~stm;
            attackers = attackers & occupancy;

            BitBoard stm_attackers = attackers & color_bb(stm);
            if (!stm_attackers) {
                break;
            }
            if (pinners(~stm) & occupancy) {
                stm_attackers &= ~blockers_for_king(stm); //remove all pinned
                if (!stm_attackers) {
                    break;
                }
            }
            res ^= 1;
            BitBoard bb;

            // Pawn
        if ((bb = stm_attackers & piece_bb(PAWN))) {
            max_value = Eval::piece_value(PAWN) - max_value;

            if (max_value < res)
                break;

            occupancy = clear_square(
                occupancy,
                BitBoards::lsb(bb)
            );

            // A pawn was removed, which can open a bishop/queen x-ray.
            attackers |= get_attacks_bb<BISHOP>(to, occupancy) & dig;
        }

        // Knight
        else if ((bb = stm_attackers & piece_bb(KNIGHT))) {
            max_value = Eval::piece_value(KNIGHT) - max_value;

            if (max_value < res)
                break;

            occupancy = clear_square(
                occupancy,
                BitBoards::lsb(bb)
            );
        }

        // Bishop
        else if ((bb = stm_attackers & piece_bb(BISHOP))) {
            max_value = Eval::piece_value(BISHOP) - max_value;

            if (max_value < res)
                break;

            occupancy = clear_square(
                occupancy,
                BitBoards::lsb(bb)
            );

            // Removing a bishop can reveal another bishop/queen.
            attackers |= get_attacks_bb<BISHOP>(to, occupancy) & dig;
        }

        // Rook
        else if ((bb = stm_attackers & piece_bb(ROOK))) {
            max_value = Eval::piece_value(ROOK) - max_value;

            if (max_value < res)
                break;

            occupancy = clear_square(occupancy,BitBoards::lsb(bb)
            );

            // Removing a rook can reveal another rook/queen.
            attackers |= get_attacks_bb<ROOK>(to, occupancy) & ortho;
        }

        // Queen
        else if ((bb = stm_attackers & piece_bb(QUEEN))) {
            max_value = Eval::piece_value(QUEEN) - max_value;
            if (max_value < res)
                break;
            occupancy = clear_square(occupancy,BitBoards::lsb(bb));

            // Removing a queen can reveal either diagonal or orthogonal
            // x-ray attackers.
            attackers |=
                (get_attacks_bb<BISHOP>(to, occupancy) & dig) |
                (get_attacks_bb<ROOK>(to, occupancy) & ortho);
        }

        // King
        else {
            // If we "capture" with the king but the opponent still
            // has attackers, reverse the result.
            return (attackers & ~color_bb(stm)) ? static_cast<bool>(res ^ 1)
                                                : static_cast<bool>(res);
        }

        }
        return static_cast<bool>(res);
    }

    bool Position::is_insufficient_material() const {
        if (std::popcount(all_bb()) > 4) {
            return false;
        }
        BitBoard major_and_pawns = piece_bb(PieceType::PAWN) | piece_bb(PieceType::ROOK) | piece_bb(PieceType::QUEEN);

        if (major_and_pawns != 0) {
            return false; // Material is sufficient to mate
        }
        int knight_count = std::popcount(piece_bb(PieceType::KNIGHT));
        int bishop_count = std::popcount(piece_bb(PieceType::BISHOP));
        int total_minors = knight_count + bishop_count;

        if (total_minors == 0) return true;

        // K+N vs K or K+B vs K
        if (total_minors == 1) return true;

        // K+B vs K+B (Check if they are on the same color squares)
        if (knight_count == 0 && bishop_count == 2) {
            BitBoard bishops = piece_bb(PieceType::BISHOP);

            // LIGHT_SQUARES is a precomputed constant bitboard (0x55AA55AA55AA55AA)
            bool both_on_light = (bishops & BitBoards::LIGHT_SQUARES) == bishops;
            bool both_on_dark  = (bishops & BitBoards::DARK_SQUARES) == bishops;

            if (both_on_light || both_on_dark) {
                return true; // Draw by same-color bishops
            }
        }
        // Cases like KNN vs K, or KB vs KN, or opposite color bishops.
        // FIDE rules technically allow a mate (helpmate) in these scenarios.
        //good for play vs humans
        return false;
    }

    //this is called before make_move
    bool Position::gives_check(const Move move) const {
        const Square from =move.from() ,to = move.to();
        const Piece moving = square(move.from());
        const Square ksq = king_square(~side_to_move());

        if (check_squares(Pieces::getType(moving)) & to) {
            return true;
        }

        //discovered check
        if (blockers_for_king(~side_to_move()) & from) {
            return !(BitBoards::line_bb[from][to] & piece_bb(KING,~side_to_move())) || move.get_type() == MoveType::CASTLING;
        }
        if (move.get_type() == MoveType::NORMAL) {
            return false;
        }
        if (move.get_type() == MoveType::PROMOTION) {
            return BitBoards::get_attacks_bb(move.promotion_type(),to,all_bb() & ~BitBoards::make_bitboard(from)) & piece_bb(KING,~side_to_move());
        }
        if (move.get_type() == MoveType::EN_PASSANT) {
            const int offset = (side_to_move() == Color::WHITE) ? 8 : -8;
            const Square capture_square = to - offset;
            BitBoard after_bb = (all_bb() & ~BitBoards::make_bitboard(from) & ~BitBoards::make_bitboard(capture_square)) | BitBoards::make_bitboard(to);

            const BitBoard bishop = BitBoards::get_attacks_bb<BISHOP>(ksq,after_bb);
            const BitBoard rook = BitBoards::get_attacks_bb<ROOK>(ksq,after_bb);

            return rook & (piece_bb(ROOK, side_to_move()) | piece_bb(QUEEN, side_to_move()))
            || bishop & (piece_bb(BISHOP, side_to_move()) | piece_bb(QUEEN, side_to_move()));
        }
        //CASTLING
        const Square rook_start =
                   from == e1 ? (to == g1 ? h1 : a1)
                              : (to == g8 ? h8 : a8);
        const Square rook_end =
                   side_to_move() == Color::WHITE
                       ? (rook_start == a1 ? d1 : f1)
                       : (rook_start == a8 ? d8 : f8);
        return check_squares(ROOK) & rook_end;
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

        return {from, to};
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
    //used to warm up the cache line
    Key Position::prefetch_key(const Move move) const {
        const Square from = move.from();
        const Square to   = move.to();
        const Piece moving   = square(from);
        const Piece captured = square(to);

        Key k = zobrist_key() ^ Engine::Zobrist::tables.side_to_move;
        if (captured != Pieces::EMPTY)
            k ^= Engine::Zobrist::piece_key(captured, to);
        k ^= Engine::Zobrist::piece_key(moving, to);
        k ^= Engine::Zobrist::piece_key(moving, from);
        return k;
    }

} // ChessCore