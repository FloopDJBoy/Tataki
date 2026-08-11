#pragma once
#include <memory>
#include <ranges>
#include <string_view>
#include "BitBoards.h"
#include <vector>

#include "Position.h"
#include "Types.h"
namespace ChessCore::FenHelper {
    using std::string_view;
    using namespace std::string_view_literals;
    constexpr bool is_piece_symbol(const char c)
    {
        switch (c)
        {
            case 'P': case 'N': case 'B':
            case 'R': case 'Q': case 'K':
            case 'p': case 'n': case 'b':
            case 'r': case 'q': case 'k':
                return true;
            default:
                return false;
        }
    }
    constexpr std::string_view STARTING_POSITION_FEN =  "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    constexpr string_view KIWIPETE_FEN = "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1";
    static constexpr string_view get_x_split(const int index,const string_view& str,const char delimiter = ' ') {
        auto segments = str | std::views::split(delimiter);
        auto it = segments.begin();
        for (int i=0;i<index;++i) {
            ++it;
        }
        return {(*it).begin(),(*it).end()};
    }

    constexpr Board get_board(const string_view& fen) {
        Board board;
        Square index = 56;
        auto rows_view = get_x_split(0,fen) |
            std::views::split('/') |
            std::views::transform([](auto&& r){return string_view(&*r.begin(),std::ranges::distance(r));});
        std::vector<string_view> rows;
        for (auto&& r : rows_view) rows.push_back(r);
        for (auto& row : rows) {
            for (auto& c : row) {
                if (is_piece_symbol(c)) {
                    const Piece p = Pieces::symbol_to_piece(c);
                    board.set_piece(index, p);
                    ++index;
                }else{
                    index += c - '0';
                }
            }
            index -= 2* 8;//move down 2 ranks
        }
        return board;
    }
    constexpr Color get_side_to_move(const string_view& fen) {
         return  get_x_split(1,fen)[0] == 'w' ? Color::WHITE : Color::BLACK;
    }
    constexpr CastlingRight get_castling_rights(const string_view fen) {
        const auto sv = get_x_split(2,fen);
        auto rights = CastlingRight::None;
        if (sv.contains('K')) {
            rights |=  CastlingRight::WhiteKingSide;
        }
        if (sv.contains('Q')) {
            rights |=  CastlingRight::WhiteQueenSide;
        }
        if (sv.contains('k')) {
            rights |=  CastlingRight::BlackKingSide;
        }
        if (sv.contains('q')) {
            rights |= CastlingRight::BlackQueenSide;
        }
        return rights;
    }
    constexpr Square get_en_square(const string_view fen)
    {
        const auto sv = get_x_split(3,fen);

        if (sv == "-")
            return NO_SQUARE;

        const int file = sv[0] - 'a';
        const int rank = sv[1] - '1';

        return rank * 8 + file;
    }
    constexpr Position fen_to_pos(const string_view fen) {
        const auto board = get_board(fen);
        const auto side = get_side_to_move(fen);
        const auto rights = get_castling_rights(fen);
        const auto en_square = get_en_square(fen);
        const auto pos = Position(board,rights,en_square,side,0);
        return Position(pos);
    }
    inline const Position STARTING_POSITION = fen_to_pos(STARTING_POSITION_FEN);
    inline const Position KIWIPETE = fen_to_pos(KIWIPETE_FEN);
} // ChessCore