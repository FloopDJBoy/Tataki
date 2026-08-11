//
// Created by FloopDJBoy on 04/08/2026.
//

#include "MoveGen.h"
#include "Position.h"

namespace ChessCore::MoveGen {
    namespace {
        [[nodiscard]] inline Move* add_moves(Move* moveList, Square from,BitBoard moves) {
            while (moves!=0) {
                const Square to = BitBoards::pop_lsb(moves);
                *moveList++ = Move(from,to);
            }
            return moveList;
        }
    }
    namespace {
        template<GenType type,bool capture>
        [[nodiscard]] inline Move* add_promotion_moves(Move* moveList, const Square from, const Square to) {
            constexpr bool all  = type == GenType::EVASIONS || type == GenType::NON_EVASIONS;
            if constexpr (type == GenType::CAPTURES || all) {
                *moveList++ = Move::make(from,to,PieceType::QUEEN);
            }
            if ((type == GenType::CAPTURES && capture) || (type == GenType::QUIETS && !capture) || all ) {
                *moveList++ = Move::make(from,to,PieceType::ROOK);
                *moveList++ = Move::make(from,to,PieceType::BISHOP);
                *moveList++ = Move::make(from,to,PieceType::KNIGHT);
            }
            return moveList;

        }
    }
    namespace {
        template<GenType type,Color Side, BitBoards::Direction Dir,bool promo = false,bool capture=false>
        [[nodiscard]] inline Move* add_pawn_move(Move* moveList, Square to)
        {
            constexpr int offset =
                    Side == Color::WHITE ? 8 : -8;

            Square from;

            if constexpr (Dir == BitBoards::Direction::UP)
            {
                from = to - offset;
            }
            else if constexpr (Dir == BitBoards::Direction::LEFT)
            {
                if constexpr (Side == Color::WHITE)
                    from = to - 7;
                else
                    from = to + 9;
            }
            else // RIGHT
            {
                if constexpr (Side == Color::WHITE)
                    from = to - 9;
                else
                    from = to + 7;
            }
            if constexpr(promo) {
                return add_promotion_moves<type,capture>(moveList,from,to);
            }
            *moveList++ = Move(from, to);
            return moveList;
        }
    }
    namespace {
        template<GenType type,Color Side>
        [[nodiscard]] Move* generate_pawn_moves(const Position& pos,Move* moveList, BitBoard target) {
            const BitBoard pawns  = pos.piece_bb(PieceType::PAWN,Side);
            const BitBoard empty = ~pos.all_bb();
            const BitBoard enemies = pos.color_bb(~Side);
            const Square ep_square = pos.ep_square();
            constexpr int offset =Side == Color::WHITE ? 8 : -8;

            constexpr BitBoard promotion_rank = Side == Color::WHITE ? BitBoards::RANK_8 : BitBoards::RANK_1;
            constexpr BitBoard double_push_rank = Side == Color::WHITE ? BitBoards::RANK_3 : BitBoards::RANK_6;
            constexpr BitBoard promo_start_rank = Side == Color::WHITE ? BitBoards::RANK_7 : BitBoards::RANK_2;
            const BitBoard promo_pawns = pawns & promo_start_rank;
            //Pawn push
            if constexpr (type != GenType::CAPTURES) {
                BitBoard pushed = BitBoards::shift_pawns<Side,BitBoards::Direction::UP>(pawns) & empty;
                BitBoard dbl = BitBoards::shift_pawns<Side,BitBoards::Direction::UP>(pushed & double_push_rank) & empty;

                const BitBoard promotions = pushed & promotion_rank;
                pushed &= ~promotions;

                if constexpr (type == GenType::EVASIONS) {
                    pushed &= target;
                    dbl &= target;
                }
                while (pushed) {
                    const Square to = BitBoards::pop_lsb(pushed);
                    *moveList++ = Move(to-offset,to);
                }
                while (dbl) {
                    const Square to = BitBoards::pop_lsb(dbl);
                    *moveList++ = Move(to-offset*2,to);
                }
            }
            if (promo_pawns) {
                BitBoard right =  BitBoards::shift_pawns<Side,BitBoards::Direction::RIGHT>(promo_pawns) & enemies;
                BitBoard left = BitBoards::shift_pawns<Side,BitBoards::Direction::LEFT>(promo_pawns) & enemies;
                BitBoard up = BitBoards::shift_pawns<Side,BitBoards::Direction::UP>(promo_pawns) & empty;
                if constexpr (type == GenType::EVASIONS) {
                    right &= target;
                    left &= target;
                    up &= target;
                }
                while (left) {
                    const Square to = BitBoards::pop_lsb(left);
                    moveList = add_pawn_move<type,Side,BitBoards::Direction::LEFT,true,true>(moveList,to);
                }
                while (right) {
                    const Square to = BitBoards::pop_lsb(right);
                    moveList = add_pawn_move<type,Side,BitBoards::Direction::RIGHT,true,true>(moveList,to);
                }
                while (up) {
                    const Square to = BitBoards::pop_lsb(up);
                    moveList =  add_promotion_moves<type,false>(moveList,to-offset,to);
                }
            }

            //pawn attacks
            if constexpr (type!=GenType::QUIETS) {

                BitBoard attacks_left = BitBoards::shift_pawns<Side,BitBoards::Direction::LEFT>(pawns) & enemies;
                BitBoard attacks_right = BitBoards::shift_pawns<Side,BitBoards::Direction::RIGHT>(pawns) & enemies;

                const BitBoard promo_left = attacks_left & promotion_rank;
                const BitBoard promo_right = attacks_right & promotion_rank;

                attacks_left &= ~promo_left;
                attacks_right &= ~promo_right;

                if constexpr (type == GenType::EVASIONS) {
                    attacks_left &= target;
                    attacks_right &=target;
                }

                while (attacks_left) {
                    const Square to = BitBoards::pop_lsb(attacks_left);
                    moveList = add_pawn_move<type,Side,BitBoards::Direction::LEFT>(moveList,to);
                }
                while (attacks_right) {
                    const Square to = BitBoards::pop_lsb(attacks_right);
                    moveList = add_pawn_move<type,Side,BitBoards::Direction::RIGHT>(moveList,to);
                }

                if (ep_square != NO_SQUARE ) {
                    const BitBoard ep_bb = BitBoards::make_bitboard(ep_square);
                    BitBoard attackers =(BitBoards::shift_pawns<~Side,BitBoards::Direction::LEFT>(ep_bb) |BitBoards::shift_pawns<~Side,BitBoards::Direction::RIGHT>(ep_bb)) & pawns;

                    //if en passant capture makes a discovered check remove it
                    if (type == GenType::EVASIONS && target & BitBoards::make_bitboard(ep_square + offset)) {
                        return moveList;
                    }

                    while (attackers) {
                        const Square from = BitBoards::pop_lsb(attackers);
                        *moveList++ = Move::make<MoveType::EN_PASSANT>(from,ep_square);
                    }
                }
            }
            return moveList;
        }
    }
    namespace {
        template<Color Side>
        Move* generate_castling(Move* moveList,const Position& pos) {
            const Square ksq = pos.king_square(Side);
            constexpr Square king_side = Side == Color::WHITE ? g1 : g8;
            constexpr Square queen_side = Side == Color::WHITE ? c1 : c8;
            if (pos.can_castle_kingside<Side>()) {
                BitBoard castling_blockers = pos.all_bb() & BitBoards::king_side_castle_mask[color_idx(Side)];
                if (castling_blockers == 0) {
                    *moveList++ = Move::make<MoveType::CASTLING>(ksq,king_side);
                }
            }
            if (pos.can_castle_queenside<Side>()) {
                BitBoard castling_blockers = pos.all_bb() & BitBoards::queen_side_castle_mask[color_idx(Side)];
                if (castling_blockers == 0) {
                    *moveList++ = Move::make<MoveType::CASTLING>(ksq,queen_side);
                }
            }
            return moveList;
        }
    }
    namespace {
        template<PieceType Pt, Color Side>
        Move* generate_moves(const Position& pos,Move* moveList, const BitBoard target) {
            static_assert(Pt != PieceType::PAWN,"PAWN not supported in generate_moves");
            BitBoard pieces = pos.piece_bb(Pt,Side);
            while (pieces != 0) {
                Square from = BitBoards::pop_lsb(pieces);
                BitBoard moves;
                if constexpr (Pt == PieceType::KNIGHT || Pt == PieceType::KING) {
                    moves = BitBoards::get_attacks_bb<Pt>(from);
                }else {
                    moves = BitBoards::get_attacks_bb<Pt>(from,pos.all_bb());
                }
                moves &= target;
                moveList = add_moves(moveList,from,moves);
            }
            return moveList;
        }
    }

    template Move* generate<GenType::CAPTURES>(const Position&, Move*);
    template Move* generate<GenType::QUIETS>(const Position&, Move*);
    template Move* generate<GenType::EVASIONS>(const Position&, Move*);
    template Move* generate<GenType::NON_EVASIONS>(const Position&, Move*);

    namespace {
        template<GenType type,Color Side>
        Move* generate_all_pseudo_moves(const Position& pos,Move* moveList) {
            static_assert(type != GenType::LEGAL,"LEGAL not supported in generate_all_psudo_moves");
            BitBoard target;
            //only king moves in double check
            if (type != GenType::EVASIONS || !BitBoards::more_than_one(pos.checkers())) {
                if constexpr (type == GenType::EVASIONS) {
                    target = BitBoards::line_between[pos.king_square(Side)][BitBoards::lsb(pos.checkers())] | BitBoards::make_bitboard(BitBoards::lsb(pos.checkers()));
                }
                else if constexpr (type == GenType::NON_EVASIONS) {target = ~pos.color_bb(Side);}
                else if constexpr (type == GenType::CAPTURES) {target = pos.color_bb(~Side);}
                else { target = ~pos.all_bb();} //QUIETS
                moveList = generate_moves<PieceType::BISHOP,Side>(pos,moveList,target);
                moveList = generate_moves<PieceType::ROOK,Side>(pos,moveList,target);
                moveList = generate_moves<PieceType::QUEEN,Side>(pos,moveList,target);
                moveList = generate_moves<PieceType::KNIGHT,Side>(pos,moveList,target);
                moveList = generate_pawn_moves<type,Side>(pos,moveList,target);
            }
            moveList = generate_moves<PieceType::KING,Side>(pos,moveList,type == GenType::EVASIONS? ~pos.color_bb(Side):target);
            if constexpr (type == GenType::QUIETS || type == GenType::NON_EVASIONS) {
                moveList = generate_castling<Side>(moveList,pos);
            }
            return moveList;
        }
    }

    template<GenType type>
    Move* generate(const Position& pos,Move* moveList) {
        static_assert(type != GenType::LEGAL,"LEGAL not supported in generate");
        assert((type == GenType::EVASIONS) == static_cast<bool>(pos.checkers()));
        const Color side = pos.side_to_move();

        return side == Color::WHITE ? generate_all_pseudo_moves<type,Color::WHITE>(pos,moveList)
                                    : generate_all_pseudo_moves<type,Color::BLACK>(pos,moveList);
    }

    template<>
    Move* generate<GenType::LEGAL>(const Position& pos,Move* moveList) {
        const Color side = pos.side_to_move();
        const BitBoard pinned = pos.blockers_for_king(side) & pos.color_bb(side);
        Move* cur = moveList;

        moveList = pos.checkers()? generate<GenType::EVASIONS>(pos,moveList): generate<GenType::NON_EVASIONS>(pos,moveList);
        while (cur != moveList) {
            if ((pinned & BitBoards::make_bitboard(cur->from())) || (cur->from() == pos.king_square(side)) || (cur->get_type() == MoveType::EN_PASSANT)) {
                if (!pos.legal(*cur)) {
                    *cur = *(--moveList);
                    continue;
                }
            }
            ++cur;
        }
        return moveList;
    }

} // ChessCore