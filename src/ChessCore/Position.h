
#pragma once
#include <set>

#include "Board.h"
#include "Move.h"
#include "MoveGen.h"
#include "Types.h"
#include "Engine/Zobrist.h"

namespace ChessCore {
    struct StateInfo {
        Key zobrist_key;

        //for undo
        Move move;
        Piece captured;
        Square ep_square;
        CastlingRight castling_rights;
        int half_clock;

        //scoring
        std::array<Value,2> pst_score;
        std::array<Value,2> material_score;
    };
    class Position {
        static constexpr int16_t MAX_PLY = 512;
        Board board;
        BitBoard check_bb = 0ull;
        std::array<BitBoard, 2> blockers_for_king_ = {0ull,0ull};
        StateInfo current_state_;
        std::array<StateInfo, MAX_PLY> history;
        Color side_to_move_ = Color::WHITE;
        int16_t ply_;
        int repetition = 0; //3 fold repetition counter
        void update_slider_blockers(Color c) ;
        [[nodiscard]] Key zobrist_key (bool from_scratch) const;
        public:
        explicit Position(
            Board board,
            CastlingRight cr,
            Square ep,
            Color side,
            int half_clock);
        [[nodiscard]] Position copy_for_search() const
        {
            Position p(*this);
            p.history.fill({});
            p.ply_ = 0;
            return p;
        }
        void make_move(Move move);
        [[nodiscard]] constexpr Piece square(const Square s) const { return board.get_piece(s); }
        [[nodiscard]] Color side_to_move() const {return side_to_move_;}
        void swap_side() {side_to_move_ = ~side_to_move_;}
        [[nodiscard]] const StateInfo& state() const {return current_state_;}
        [[nodiscard]] Square king_square(const Color side) const {return board.king_squares[color_idx(side)];}
        [[nodiscard]] constexpr BitBoard piece_bb(const Piece p) const {assert(Pieces::getType(p) != PieceType::EMPTY); return board.piece_bitboard[p];}
        [[nodiscard]] constexpr BitBoard piece_bb(const PieceType p,const Color color) const {assert(p != PieceType::EMPTY); return board.piece_bitboard[Pieces::makePiece(p,color)];}
        [[nodiscard]] constexpr BitBoard color_bb(const Color c) const { return board.color_bitboard[color_idx(c)]; }
        [[nodiscard]] constexpr BitBoard all_bb() const { return board.all_piece_bitboard; }
        [[nodiscard]] constexpr BitBoard ep_square() const { return current_state_.ep_square; }
        [[nodiscard]] BitBoard  piece_bb(PieceType p) const { return piece_bb(Pieces::makePiece(p,Color::WHITE)) | piece_bb(Pieces::makePiece(p,Color::BLACK)); }
        bool try_make_move(Move move);
        [[nodiscard]] CastlingRight castling_rights() const { return current_state_.castling_rights; }
        template<Color Side>
        [[nodiscard]] bool can_castle_kingside() const {
            constexpr auto mask = Side == Color::WHITE ? CastlingRight::WhiteKingSide : CastlingRight::BlackKingSide;
            return (state().castling_rights & mask) != CastlingRight::None;
        }
        [[nodiscard]] BitBoard checkers() const {return check_bb;}
        template<Color Side>
        [[nodiscard]] bool can_castle_queenside() const {
            constexpr auto mask = Side == Color::WHITE ? CastlingRight::WhiteQueenSide : CastlingRight::BlackQueenSide;
            return (state().castling_rights & mask) != CastlingRight::None;
        }
        inline const StateInfo& push_state(Move move, Piece captured);
        void handle_castling_rights(Move move);
        void undo_move();
        [[nodiscard]] bool legal(Move move) const;
        [[nodiscard]] std::set<Square> get_moves_squares(Square s) const;
        [[nodiscard]] BitBoard blockers_for_king(const Color c) const {return blockers_for_king_[color_idx(c)];}
        [[nodiscard]] Move parse_move(const std::string & move_string) const;
        [[nodiscard]] Move parse_move(Square from, Square to, PieceType promo) const;
        [[nodiscard]] BitBoard attackers_to(Square s , BitBoard occupancy) const;
        [[nodiscard]] Key polyglot_hash() const;
        [[nodiscard]] auto legal_moves() const {return MoveGen::MoveList<MoveGen::GenType::LEGAL>(*this);}
        [[nodiscard]] auto quiescence_moves() const {return MoveGen::MoveList<MoveGen::GenType::CAPTURES>(*this);} //moves are pseudo_legal
        [[nodiscard]] auto evasion_moves() const {return MoveGen::MoveList<MoveGen::GenType::EVASIONS>(*this);}
        template<MoveGen::GenType type>
        [[nodiscard]] auto generate_moves() const {return MoveGen::MoveList<type>(*this);}
        [[nodiscard]] bool in_check() const {return checkers();}
        [[nodiscard]] Key zobrist_key() const {return zobrist_key(false);}
        [[nodiscard]] bool is_3fold() const {return repetition >= 2;}
        void verify_zobrist() const {assert(current_state_.zobrist_key == zobrist_key(true));}
        [[nodiscard]] int ply() const {return ply_;}


    };
} // ChessCore
