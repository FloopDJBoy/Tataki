
#pragma once
#include <algorithm>
#include <array>
#include <cassert>
#include <bit>
#include "PrecomputedMagics.h"
#include "Types.h"
namespace ChessCore::BitBoards {
    constexpr BitBoard FILE_A = 0x0101010101010101ULL;
    constexpr BitBoard FILE_B = FILE_A << 1;
    constexpr BitBoard FILE_C = FILE_A << 2;
    constexpr BitBoard FILE_D = FILE_A << 3;
    constexpr BitBoard FILE_E = FILE_A << 4;
    constexpr BitBoard FILE_F = FILE_A << 5;
    constexpr BitBoard FILE_G = FILE_A << 6;
    constexpr BitBoard FILE_H = FILE_A << 7;

    constexpr BitBoard RANK_1 = 0b11111111ull;
    constexpr BitBoard RANK_2 = RANK_1<<8;
    constexpr BitBoard RANK_3 = RANK_2<<8;
    constexpr BitBoard RANK_4 = RANK_3<<8;
    constexpr BitBoard RANK_5 = RANK_4<<8;
    constexpr BitBoard RANK_6 = RANK_5<<8;
    constexpr BitBoard RANK_7 = RANK_6<<8;
    constexpr BitBoard RANK_8 = RANK_7<<8;

    constexpr std::array<BitBoard, 2> queen_side_castle_mask = {
        (1ULL << b1) | (1ULL << c1) | (1ULL << d1), // white queenside
        (1ULL << b8) | (1ULL << c8) | (1ULL << d8)  // black queenside
    };

    constexpr std::array<BitBoard, 2> king_side_castle_mask = {
        (1ULL << f1) | (1ULL << g1), // white kingside
        (1ULL << f8) | (1ULL << g8)  // black kingside
    };

    constexpr std::array<BitBoard, 2> king_side_path = {
        (1ULL << e1) | (1ULL << f1) | (1ULL << g1), // white kingside path
        (1ULL << e8) | (1ULL << f8) | (1ULL << g8)  // black kingside path
    };

    constexpr std::array<BitBoard, 2> queen_side_path = {
        (1ULL << e1) | (1ULL << d1) | (1ULL << c1), // white queenside path
        (1ULL << e8) | (1ULL << d8) | (1ULL << c8)  // black queenside path
    };


    enum class Direction {
        UP,
        RIGHT,
        LEFT
    };
    constexpr Square pop_lsb(BitBoard& bb) {
        const int index = std::countr_zero(bb);
        bb = bb & (bb - 1);
        return index;
    }
    constexpr BitBoard set_square(const BitBoard b, const Square s) {
         return  b | (1ull<<s);
    }
    constexpr BitBoard clear_square(const BitBoard b, const Square s) {
        return  b &~(1ull<<s);
    }
    constexpr std::array<BitBoard,64> SquareBB = []() -> std::array<BitBoard,64>{
        std::array<BitBoard, 64> bb{};
        for (int i = 0; i < 64; ++i)
            bb[i] = 1ULL << i;
        return bb;
    }();
    constexpr BitBoard make_bitboard(const Square s) {
        return SquareBB[s];
    }
    constexpr BitBoard toggle_square(const BitBoard b, const Square s) {
        return  b ^(1ull<<s);
    }
    constexpr bool more_than_one(const BitBoard bb) {
        return (bb & (bb-1));
    }
    constexpr Square lsb(const BitBoard bb) {
        return std::countr_zero(bb);
    }
    constexpr int file_of(const Square s) {
        return s%8;
    }
    constexpr int rank_of(const Square s) {
        return s/8;
    }


    constexpr std::array<std::pair<int, int>, 8> knight_jumps = {{
        {2, 1}, {1, 2}, {-1, 2}, {-2, 1},
        {-2, -1}, {-1, -2}, {1, -2}, {2, -1}
    }};

    constexpr std::array<std::pair<int, int>, 8> king_moves = {
        {
            {0,1},{0,-1},
            {-1,0},{1,0},
            {1,1},{1,-1},
            {-1,1},{-1,-1}
        }
    };
    constexpr std::array<std::pair<int, int>, 4> rook_moves = {
        {{1,0},{-1,0},{0,1},{0,-1}}
    };
    constexpr std::array<std::pair<int, int>, 4> bishop_moves = {
        {{1,1},{-1,1}, {1,-1},{-1,-1}}
    };
    constexpr std::array<BitBoard, 64> make_knight_attacks() {
         std::array<BitBoard, 64> knight_attacks{};
        for (int i = 0; i < 64; i++) {
            BitBoard bb = 0;
            auto [x,y] = square_to_cord(i);
            for (auto [dx,dy] : knight_jumps) {
                   if (is_valid_cord(x+dx,y+dy)) {
                       bb |= make_bitboard(cord_to_square(x+dx,y+dy));
                   }
            }
            knight_attacks[i] = bb;
        }
        return knight_attacks;
    }
    constexpr std::array<BitBoard, 64> make_king_attacks() {
        std::array<BitBoard, 64> king_attacks{};
        for (int i = 0; i < 64; i++) {
            BitBoard bb = 0;
            auto [x,y] = square_to_cord(i);
            for (auto [dx,dy] : king_moves) {
                if (is_valid_cord(x+dx,y+dy)) {
                    bb |= make_bitboard(cord_to_square(x+dx,y+dy));
                }
            }
            king_attacks[i] = bb;
        }
        return king_attacks;
    }
    template<Color C, Direction D>
    constexpr BitBoard shift_pawns(const BitBoard pawns)
    {
        if constexpr (C == Color::WHITE)
        {
            if constexpr (D == Direction::UP)
                return (pawns & ~RANK_8) << 8;

            else if constexpr (D == Direction::LEFT)
                return (pawns & ~FILE_A & ~RANK_8) << 7;

            else // RIGHT
                return (pawns & ~FILE_H & ~RANK_8) << 9;
        }
        else
        {
            if constexpr (D == Direction::UP)
                return (pawns & ~RANK_1) >> 8;

            else if constexpr (D == Direction::LEFT)
                return (pawns & ~FILE_A & ~RANK_1) >> 9;

            else // RIGHT
                return (pawns & ~FILE_H & ~RANK_1) >> 7;
        }
    }
    constexpr std::array<BitBoard, 64> generate_occupancy(const std::array<std::pair<int, int>, 4> &moves) {
        std::array<BitBoard, 64> arr{};
        for (Square s = 0; s < 64; ++s) {
            BitBoard occupancy = 0ull;
            auto [startX, startY] = square_to_cord(s);
            for (auto [dx,dy] : moves) {
                int x = startX + dx;
                int y = startY + dy;
                while (is_valid_cord(x,y)) {
                    if (!is_valid_cord(x+dx,y+dy)) {break;}
                    occupancy |= make_bitboard(cord_to_square(x,y));
                    x += dx;
                    y += dy;
                }
            }
            arr[s] = occupancy;
        }
        return arr;
    }
    constexpr std::array<BitBoard, 64> rook_occupancy = generate_occupancy(rook_moves);
    constexpr std::array<BitBoard, 64> bishop_occupancy = generate_occupancy(bishop_moves);
    static_assert(std::ranges::all_of(rook_occupancy,[](const BitBoard oc){return __builtin_popcountll(oc)<=12;}));
    static_assert(std::ranges::all_of(bishop_occupancy,[](const BitBoard oc){return __builtin_popcountll(oc)<=9;}));
    template<PieceType Pt>
    constexpr auto generate_bit_positions(const BitBoard occupancy) {
        static_assert(Pt == PieceType::BISHOP || Pt == PieceType::ROOK,"magic can only be bishops or rooks");
        constexpr int size = Pt == PieceType::BISHOP ? 9 : 12;
        std::array<Square,size> bit_positions{};
        int index = 0;
        for (Square s = 0; s < 64; s++) {
            if ((occupancy & (1ull<<s))!=0) {
                bit_positions[index++] = s;
            }
        }
        return bit_positions;
    }
    template<PieceType Pt>
    constexpr auto generate_occupancy_variations(const BitBoard occupancy) {
        static_assert(Pt == PieceType::BISHOP || Pt == PieceType::ROOK,"magic can only be bishops or rooks");
        constexpr int size = Pt == PieceType::BISHOP ? 512 : 4096;
        std::array<BitBoard,size> occupancy_variations{};
        const auto bit_positions = generate_bit_positions<Pt>(occupancy);
        const int count = __builtin_popcountll(occupancy);
        const int n = 1<<count;
        for (int i=0;i<n;++i) {
            BitBoard variation = 0ull;
            for (int j = 0; j <count; ++j) {
                if ((i & (1 << j)) != 0) {
                    variation |= make_bitboard(bit_positions[j]);
                }
            }
            occupancy_variations[i] = variation;
        }
        return occupancy_variations;
    }
    constexpr BitBoard generate_slider_attack(const BitBoard variation, const Square square,const std::array<std::pair<int, int>, 4> &moves) {
        BitBoard attack = 0ull;
        auto [startX, startY] = square_to_cord(square);
        for (auto [dx,dy] : moves) {
            int x = startX + dx;
            int y = startY + dy;
            while (is_valid_cord(x,y)) {
                attack |= make_bitboard(cord_to_square(x,y));
                if ((variation & (1ull<<cord_to_square(x,y)))!=0) break;
                x += dx;
                y += dy;
            }
        }
        return attack;
    }
    template<PieceType Pt>
    auto& make_magic_arr() {
        static_assert(
            Pt == PieceType::BISHOP || Pt == PieceType::ROOK,
            "magic can only be bishops or rooks"
        );

        if constexpr (Pt == PieceType::BISHOP) {
            static std::array<std::array<BitBoard, 512>, 64> attacks{};
            return attacks;
        } else {
            static std::array<std::array<BitBoard, 4096>, 64> attacks{};
            return attacks;
        }
    }
    template<PieceType Pt>
    auto make_slider_attacks(const std::array<BitBoard,64>& occupancies,const std::array<std::pair<int, int>, 4> &moves,const std::array<BitBoard,64> &magic,const std::array<int,64> &shifts) {
        static_assert(Pt == PieceType::BISHOP || Pt == PieceType::ROOK,"magic can only be bishops or rooks");
        auto& attacks = make_magic_arr<Pt>();
        for (Square square = 0; square < 64; square++) {
            const BitBoard occupancy = occupancies[square];
            auto variations = generate_occupancy_variations<Pt>(occupancy);
            const int count = 1 << std::popcount(occupancy);
            for (int i = 0; i < count; i++)
            {
                const BitBoard variation = variations[i];

                BitBoard attack =
                    generate_slider_attack(
                        variation,
                        square,
                        moves
                    );

                int magic_index =
                    static_cast<int>(
                        (variation * magic[square]) >> shifts[square]
                    );

                attacks[square][magic_index] = attack;
            }
        }
        return attacks;
    }

    constexpr std::array<BitBoard, 64> king_attacks = make_king_attacks();
    constexpr std::array<BitBoard, 64> knight_attacks = make_knight_attacks();
    inline const auto rook_attacks = make_slider_attacks<PieceType::ROOK>(rook_occupancy,rook_moves, PrecomputedMagics::rook_magic, PrecomputedMagics::rook_shifts);
    inline const auto bishop_attacks = make_slider_attacks<PieceType::BISHOP>(bishop_occupancy,bishop_moves, PrecomputedMagics::bishop_magic, PrecomputedMagics::bishop_shifts);
    constexpr BitBoard get_rook_attack(const Square square,const BitBoard variation) {
        const int magic_index =
                   static_cast<int>(
                       ((variation & rook_occupancy[square])* PrecomputedMagics::rook_magic[square]) >> PrecomputedMagics::rook_shifts[square]
                   );
        return rook_attacks[square][magic_index];

    }
    constexpr BitBoard get_bishop_attack(const Square square,const BitBoard variation) {
        const int magic_index =
            static_cast<int>(
                       ((variation & bishop_occupancy[square])* PrecomputedMagics::bishop_magic[square]) >> PrecomputedMagics::bishop_shifts[square]
                   );
        return bishop_attacks[square][magic_index];
    }
    template<PieceType Pt>
    constexpr BitBoard get_attacks_bb(const Square square,const BitBoard occupancy) {
        static_assert(Pt != PieceType::EMPTY, "Cannot get attacks for EMPTY");
        static_assert(Pt != PieceType::PAWN, "PieceType pawn not supported.");
        assert(is_valid_square(square));
        if constexpr (Pt == PieceType::KNIGHT) {
            return knight_attacks[square];
        }else if constexpr (Pt == PieceType::KING) {
            return king_attacks[square];
        }else if constexpr (Pt == PieceType::ROOK) {
            return get_rook_attack(square,occupancy);
        }else if constexpr (Pt == PieceType::BISHOP) {
            return get_bishop_attack(square,occupancy);
        }else if constexpr (Pt == PieceType::QUEEN) {
            return get_rook_attack(square, occupancy)
            | get_bishop_attack(square, occupancy);
        }
    }
    template<Color c>
    constexpr BitBoard get_pawns_attacks(const BitBoard pawns) {
        return shift_pawns<c,Direction::LEFT>(pawns) | shift_pawns<c,Direction::RIGHT>(pawns);
    }
    template<Color c>
    constexpr BitBoard get_single_pawn_attacks(const Square square) {
        BitBoard s = make_bitboard(square);
        if constexpr (c == Color::WHITE) {
            return get_pawns_attacks<Color::WHITE>(s);
        }else {
            return get_pawns_attacks<Color::BLACK>(s);
        }
    }
    template<PieceType Pt>
    constexpr BitBoard get_attacks_bb(const Square square) {
        static_assert(Pt != PieceType::PAWN, "PieceType not supported.");
        static_assert(Pt != PieceType::ROOK && Pt != PieceType::BISHOP && Pt != PieceType::QUEEN, "Please provide occupancy bitboard");
        assert(is_valid_square(square));
        return  get_attacks_bb<Pt>(square,0ull);
    }
    constexpr auto make_line_between() {
        constexpr BitBoard empty = 0ull;
        std::array<std::array<BitBoard,64>,64> line_between{};
        for (Square from = 0; from < 64; from++) {
            for (Square to = 0; to < 64; to++) {
                if ((get_attacks_bb<PieceType::ROOK>(from,empty) & make_bitboard(to))!=0 || (get_attacks_bb<PieceType::BISHOP>(from,empty) & make_bitboard(to))!=0) {
                    auto [x1,y1] = square_to_cord(from);
                    auto [x2,y2] = square_to_cord(to);

                    const int dx = (x1-x2 == 0)? 0:(x1>x2) ? -1 : 1;
                    const int dy = (y1-y2 == 0)? 0:(y1>y2) ? -1 : 1;

                    x1 += dx;
                    y1 += dy;
                    if (dx != 0 || dy != 0) {
                        while (x1 != x2 || y1 != y2) {
                            line_between[from][to] |= make_bitboard(cord_to_square(x1,y1));
                            x1 += dx;
                            y1 += dy;
                        }
                        line_between[from][to] |= make_bitboard(to);
                    }else {
                        line_between[from][to] = empty;
                    }
                }
            }
        }

        return line_between;
    }
    constexpr auto make_line_bb() {
        constexpr BitBoard empty = 0ull;
        std::array<std::array<BitBoard,64>,64> line_bb{};

        for (Square from = 0; from < 64; from++) {
            for (Square to = 0; to < 64; to++) {

                auto rook_attack = get_attacks_bb<PieceType::ROOK>(from, empty);
                auto bishop_attack = get_attacks_bb<PieceType::BISHOP>(from, empty);

                if ((rook_attack & make_bitboard(to)) ||
                    (bishop_attack & make_bitboard(to))) {

                    auto [x1,y1] = square_to_cord(from);
                    auto [x2,y2] = square_to_cord(to);

                    const int dx = (x2 > x1) - (x2 < x1);
                    const int dy = (y2 > y1) - (y2 < y1);

                    // walk backwards
                    int x = x1;
                    int y = y1;

                    while (x >= 0 && x < 8 && y >= 0 && y < 8) {
                        line_bb[from][to] |= make_bitboard(cord_to_square(x,y));
                        x -= dx;
                        y -= dy;
                    }

                    // walk forwards
                    x = x1 + dx;
                    y = y1 + dy;

                    while (x >= 0 && x < 8 && y >= 0 && y < 8) {
                        line_bb[from][to] |= make_bitboard(cord_to_square(x,y));
                        x += dx;
                        y += dy;
                    }
                    }
            }
        }
        return line_bb;
    }
    inline const auto line_between = make_line_between();
    inline const auto line_bb = make_line_bb();
    constexpr bool validate_shifts()
    {
        for (int sq = 0; sq < 64; sq++)
        {
            if (PrecomputedMagics::rook_shifts[sq] != 64 - std::popcount(rook_occupancy[sq]))
                return false;

            if (PrecomputedMagics::bishop_shifts[sq] != 64 - std::popcount(bishop_occupancy[sq]))
                return false;
        }

        return true;
    }
    static_assert(cord_to_square(0,0) == a1); // A1
    static_assert(cord_to_square(7,7) == h8); // H8

}
