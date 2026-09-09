//
// Created by FloopDjBoy on 06/09/2026.
//

#ifndef TATAKI_VIRIFORMAT_H
#define TATAKI_VIRIFORMAT_H
#include "ChessCore/FenHelper.h"
#include "ChessCore/Position.h"

namespace Engine::Eval::NNUE::ViriFormat {
    // Marlinformat: pawn 0, knight 1, bishop 2, rook 3, queen 4, king 5.
    // Ours:         PAWN 1, KNIGHT 2, BISHOP 3, ROOK  4, QUEEN 5, KING 6.  -> subtract 1.
    // Colour: Color::BLACK == 8 is already bit 3 of the nibble, so it ORs in directly.
    constexpr uint8_t UNMOVED_ROOK = 6;

    // Absolute, NOT side-to-move relative. Black win is 0, white win is 2.
    enum class GameResult : uint8_t {
        BLACK_WIN = 0,
        DRAW      = 1,
        WHITE_WIN = 2
    };

    // ---------------------------------------------------------------------
    // 32-byte marlinformat PackedBoard. Field order is load-bearing.
    // ---------------------------------------------------------------------
    struct PackedBoard {
        BitBoard              occupancy       = 0;
        std::array<uint8_t,16> pieces          {};
        uint8_t               stm_ep          = 64;
        uint8_t               halfmove_clock  = 0;
        uint16_t              fullmove_number = 0;
        Score                 eval            = 0;   // white-relative, may be 0
        GameResult            result          = GameResult::DRAW;
        uint8_t               extra           = 0;

        void set_piece(const int i, const uint8_t v) {
            pieces[i / 2] |= static_cast<uint8_t>((v & 0xF) << (4 * (i & 1)));
        }
        [[nodiscard]] uint8_t get_piece(const int i) const {
            return (pieces[i / 2] >> (4 * (i & 1))) & 0xF;
        }

        PackedBoard() = default;

        PackedBoard(const ChessCore::Position& pos, const Score white_relative_eval,
                    const GameResult res)
            : eval(white_relative_eval), result(res)
        {
            occupancy = pos.all_bb();

            // Rooks that still carry castling rights get piece type 6 instead of 3.
            BitBoard unmoved = 0;
            const CastlingRight cr = pos.castling_rights();
            const auto has = [cr](const CastlingRight r) {
                return (cr & r) != CastlingRight::None;
            };
            if (has(CastlingRight::WhiteKingSide))  unmoved |= 1ull << h1;
            if (has(CastlingRight::WhiteQueenSide)) unmoved |= 1ull << a1;
            if (has(CastlingRight::BlackKingSide))  unmoved |= 1ull << h8;
            if (has(CastlingRight::BlackQueenSide)) unmoved |= 1ull << a8;

            BitBoard occ = occupancy;
            int i = 0;
            while (occ) {
                const Square sq = ChessCore::BitBoards::pop_lsb(occ);   // ascending square order
                const Piece  p  = pos.square(sq);

                uint8_t type = static_cast<uint8_t>(ChessCore::Pieces::getType(p)) - 1;
                if ((unmoved >> sq) & 1ull) type = UNMOVED_ROOK;

                set_piece(i++, static_cast<uint8_t>(type | (p & ChessCore::Pieces::colorMask)));
            }

            const Square ep = pos.ep_square();
            stm_ep = static_cast<uint8_t>((ep == NO_SQUARE ? 64 : ep)
                   | (pos.side_to_move() == Color::BLACK ? 0x80 : 0x00));

            // Both of these are explicitly allowed to be zero by the spec.
            // halfmove_clock  = static_cast<uint8_t>(pos.half_clock());
            // fullmove_number = static_cast<uint16_t>(pos.fullmove_number());
        }
    };

    static_assert(sizeof(PackedBoard) == 32, "PackedBoard must be exactly 32 bytes");
    static_assert(std::is_trivially_copyable_v<PackedBoard>);

    // ---------------------------------------------------------------------
    // 16-bit move: from in bits 0-5, to in 6-11, promo in 12-13, type in 14-15.
    // Castling is encoded king-takes-rook.
    // ---------------------------------------------------------------------
    constexpr uint16_t VIRI_TYPE[4] = { 0, 3, 1, 2 };  // NORMAL, PROMOTION, EN_PASSANT, CASTLING

    constexpr uint16_t encode_move(const ChessCore::Move m) {
        const uint16_t from = m.from();
        uint16_t to = m.to();

        if (m.get_type() == MoveType::CASTLING)
            to = (to == g1) ? h1 : (to == c1) ? a1 : (to == g8) ? h8 : a8;

        const uint16_t promo = (m.raw() >> 12) & 0b11;
        const uint16_t type  = VIRI_TYPE[static_cast<uint16_t>(m.get_type()) >> 14];

        return static_cast<uint16_t>(from | (to << 6) | (promo << 12) | (type << 14));
    }

    // ---------------------------------------------------------------------
    // One game: header, then (move, score) pairs, then four zero bytes.
    // Reused across games - clear(), don't reallocate.
    // ---------------------------------------------------------------------
    class GameBuffer {
        std::vector<uint8_t> buf;
        bool init_ = false;
    public:
        GameBuffer() { buf.reserve(4096); }

        void begin(const PackedBoard& header) {
            buf.clear();
            init_ = true;
            const auto* raw = reinterpret_cast<const uint8_t*>(&header);
            buf.insert(buf.end(), raw, raw + sizeof(PackedBoard));
        }

        void push(const ChessCore::Move m, const Score white_relative_cp) {
            const uint16_t em = encode_move(m);
            const auto     sc = static_cast<uint16_t>(white_relative_cp);
            buf.push_back(static_cast<uint8_t>(em & 0xFF));   buf.push_back(static_cast<uint8_t>(em >> 8));
            buf.push_back(static_cast<uint8_t>(sc & 0xFF));   buf.push_back(static_cast<uint8_t>(sc >> 8));
        }
        [[nodiscard]] bool init() const {
            return init_;
        }

        // Terminator. Returns the finished game; append it to the output buffer
        // as one unit so a crash truncates at a game boundary.
        [[nodiscard]] const std::vector<uint8_t>& finish() {
            init_ = false;
            buf.insert(buf.end(), 4, 0);
            return buf;
        }
    };

    // ---------------------------------------------------------------------
    // Startpos test vector, lifted from the spec's worked example.
    // If this fails, flip the nibble order in set_piece/get_piece.
    // ---------------------------------------------------------------------
    inline bool self_test() {
        static constexpr uint8_t EXPECTED[32] = {
            0xff,0xff,0x00,0x00, 0x00,0x00,0xff,0xff,   // occupancy
            0x16,0x42,0x25,0x61, 0x00,0x00,0x00,0x00,   // white back rank, white pawns
            0x88,0x88,0x88,0x88, 0x9e,0xca,0xad,0xe9,   // black pawns, black back rank
            0x40,                                        // white to move, no ep
            0x00,                                        // halfmove clock
            0x01,0x00,                                   // fullmove counter = 1
            0x00,0x00,                                   // score = 0
            0x02,                                        // white win
            0x00                                         // extra
        };

        const ChessCore::Position pos(ChessCore::FenHelper::STARTING_POSITION_FEN);
        PackedBoard pb(pos, 0, GameResult::WHITE_WIN);
        pb.fullmove_number = 1;                          // the example sets this

        return std::memcmp(&pb, EXPECTED, 32) == 0;
    }
}

#endif //TATAKI_VIRIFORMAT_H
