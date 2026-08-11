//
// Created by FloopDJBoy on 02/08/2026.
//

#pragma once
#include <cstdint>
#include <cassert>
#include <string>

#include "../Types.h"
namespace ChessCore {
    // bit  0- 5: destination square (from 0 to 63)
    // bit  6-11: origin square (from 0 to 63)
    // bit 12-13: promotion piece type - 2 (from KNIGHT-2 to QUEEN-2)
    // bit 14-15: special move flag: promotion (1), en passant (2), castling (3)
    class Move {
        uint16_t data;
        static constexpr uint16_t TO_MASK   = 0x3F;
        static constexpr uint16_t FROM_MASK = 0xFC0;
        static constexpr uint16_t PROMO_MASK = 0x3000;
        static constexpr uint16_t TYPE_MASK = 0xC000;
        public:
        Move() = default;
        explicit constexpr Move(const uint16_t data) : data(data) {}
        constexpr Move(const Square from, const Square to) : data((from<<6) | to) {}

        [[nodiscard]] constexpr Square from() const
        {
            return static_cast<Square>((data >> 6) & 0x3F);
        }
        template<MoveType type>
        constexpr static Move make(const Square from, const Square to) {
            static_assert(type!=MoveType::PROMOTION);
            return   Move((from<<6 )| to | static_cast<uint16_t>(type));
        }
        constexpr static Move make(const Square from, const Square to, const PieceType promotion) {
            return  Move((from << 6)
            |to
            |((static_cast<uint16_t>(promotion) -static_cast<uint16_t>(PieceType::KNIGHT)) << 12)
            |static_cast<uint16_t>(MoveType::PROMOTION));
        }
        [[nodiscard]]
        constexpr PieceType promotion_type() const
        {
            assert(get_type() == MoveType::PROMOTION);
            return static_cast<PieceType>(
                ((data >> 12) & 0b11) + static_cast<uint8_t>(PieceType::KNIGHT)
            );
        }
        [[nodiscard]] constexpr MoveType get_type() const {return static_cast<MoveType>(data & TYPE_MASK);}
        [[nodiscard]] constexpr Square to() const
        {
            return static_cast<Square>(data & 0x3F);
        }
        bool operator==(const Move  m) const {return data == m.data;}
        bool operator!=(const Move  m) const {return data != m.data;}
        [[nodiscard]] std::string to_string() const
        {
            if (data == 0)
                return "0000";

            auto square_to_string = [](Square sq) -> std::string {
                const int s = static_cast<int>(sq);
                std::string result;
                result += char('a' + (s & 7));
                result += char('1' + (s >> 3));
                return result;
            };

            std::string result = square_to_string(from()) + square_to_string(to());

            if (get_type() == MoveType::PROMOTION)
            {
                switch (promotion_type())
                {
                    case PieceType::KNIGHT: result += 'n'; break;
                    case PieceType::BISHOP: result += 'b'; break;
                    case PieceType::ROOK:   result += 'r'; break;
                    case PieceType::QUEEN:  result += 'q'; break;
                    default: break;
                }
            }

            return result;
        }
        static constexpr Move none() {return Move(0);}

        [[nodiscard]] constexpr uint16_t raw() const
        {
            return data;
        }
    };
    static_assert(sizeof(Move) == sizeof(uint16_t));
    static_assert(std::is_trivially_copyable_v<Move>);
}
