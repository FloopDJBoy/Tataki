
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
        int repetition;  //3 fold repetition counter
        int half_clock;

        //scoring
        std::array<Value, 2> non_pawn_material;
        std::array<ScorePair,2> material_score;
        uint8_t phase;
        Key pawn_key;
    };
    class Position {
        static constexpr int16_t MAX_PLY = 512;
        Board board;
        BitBoard check_bb = 0ull;
        std::array<BitBoard, COLOR_NUMBER> blockers_for_king_ = {0ull,0ull};
        StateInfo current_state_;
        std::array<StateInfo, MAX_PLY> history;
        Color side_to_move_ = Color::WHITE;
        int16_t ply_;
        int16_t root_ply_ = 0; // ply_ value when this copy was handed to Search
        int32_t fullmove_number_;
        std::array<BitBoard,COLOR_NUMBER> pinners_ = {0,0};
        std::array<BitBoard,PT_NUMBER> check_squares_{};
        void update_slider_blockers(Color c) ;
        [[nodiscard]] Key zobrist_key (bool from_scratch) const;
        public:
        explicit Position(
            Board board,
            CastlingRight cr,
            Square ep,
            Color side,
            int half_clock,
            int full_move);
        [[nodiscard]] Position copy_for_search() const
        {
            Position p(*this);
            p.root_ply_ = p.ply_;
            return p;
        }
        explicit Position(std::string_view fen);
        [[nodiscard]] std::string_view fen() const;
        void make_move(Move move);
        void make_null_move();
        void undo_null_move();
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
        [[nodiscard]] BitBoard  piece_bb(const PieceType p) const { return piece_bb(Pieces::makePiece(p,Color::WHITE)) | piece_bb(Pieces::makePiece(p,Color::BLACK)); }
        [[nodiscard]] BitBoard check_squares(const PieceType p) const {return check_squares_[static_cast<int>(p)];}

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
        void update_check_info();
        [[nodiscard]] Move parse_move(const std::string & move_string) const;
        [[nodiscard]] Move parse_move(Square from, Square to, PieceType promo) const;
        [[nodiscard]] BitBoard attackers_to(Square s , BitBoard occupancy) const;
        [[nodiscard]] bool see_ge(Move move,int threshold = 0) const;
        [[nodiscard]] Key polyglot_hash() const;
        [[nodiscard]] auto legal_moves() const {return MoveGen::MoveList<MoveGen::GenType::LEGAL>(*this);}
        [[nodiscard]] auto quiescence_moves() const {return MoveGen::MoveList<MoveGen::GenType::CAPTURES>(*this);} //moves are pseudo_legal
        [[nodiscard]] auto evasion_moves() const {return MoveGen::MoveList<MoveGen::GenType::EVASIONS>(*this);}
        template<MoveGen::GenType type>
        [[nodiscard]] auto generate_moves() const {return MoveGen::MoveList<type>(*this);}
        [[nodiscard]] bool in_check() const {return checkers();}
        [[nodiscard]] Key zobrist_key() const {return zobrist_key(false);}
        [[nodiscard]] Key pawn_key() const {return state().pawn_key;}
        [[nodiscard]] bool is_3fold() const {return state().repetition >= 1;}
        [[nodiscard]] bool is_insufficient_material() const;
        [[nodiscard]] BitBoard pinners(const Color c) const {return pinners_[color_idx(c)];}
        [[nodiscard]] Key prefetch_key(Move move) const;
        [[nodiscard]] bool is_draw() const {return is_3fold() || state().half_clock>=100 || is_insufficient_material();}
        [[nodiscard]] bool is_capture(const Move move) const {
            const MoveType mt = move.get_type();
            if (mt == MoveType::NORMAL) {
                return square(move.to()) != Pieces::EMPTY ;
            }
            //all queen promo is a capture
            if (mt == MoveType::PROMOTION) {
                return square(move.to()) != Pieces::EMPTY || move.promotion_type() == PieceType::QUEEN;
            }
            return mt == MoveType::EN_PASSANT;
        }
        [[nodiscard]] Value non_pawn_material(const Color c) const {return state().non_pawn_material[color_idx(c)];}
        [[nodiscard]] Value non_pawn_material() const {return non_pawn_material(Color::WHITE) + non_pawn_material(Color::BLACK);}
        [[nodiscard]] bool gives_check(Move move) const;
        [[nodiscard]] const StateInfo& previous() const {assert(ply_!=0);return history[ply_-1];}
#ifndef NDEBUG
        void verify_zobrist() const {
            assert(current_state_.zobrist_key == zobrist_key(true));
        }
#endif
        [[nodiscard]] int ply() const {return ply_ - root_ply_;}
        [[nodiscard]] int abs_ply () const {return ply_;}


    };
} // ChessCore
