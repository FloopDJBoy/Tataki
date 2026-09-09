//
// Created by FloopDjBoy on 05/09/2026.
//


#include "FilterData.h"

#include <cassert>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

#include "ViriFormat.h"
#include "ChessCore/FenHelper.h"
#include "ChessCore/Position.h"

namespace Engine::Eval::NNUE::FilterData {
    namespace {

        // The format's own mate marker (see the spec's worked example), which is
        // not necessarily the engine's internal MATE_SCORE.
        constexpr Score MATE_CP = 32767;

        constexpr size_t OUT_BUFFER_FLUSH = 32u << 20;   // 32 MB

        constexpr bool is_digit(const char c)   { return static_cast<uint8_t>(c - '0') < 10; }
        constexpr bool is_file_ch(const char c) { return c >= 'a' && c <= 'h'; }
        constexpr bool is_rank_ch(const char c) { return c >= '1' && c <= '8'; }

        constexpr PieceType piece_type_of(const char c) {
            switch (c) {
                case 'K': return PieceType::KING;
                case 'Q': return PieceType::QUEEN;
                case 'R': return PieceType::ROOK;
                case 'B': return PieceType::BISHOP;
                case 'N': return PieceType::KNIGHT;
                default:  return PieceType::EMPTY;
            }
        }

        struct SanResult {
            ChessCore::Move move;
            bool gives_check;
            [[nodiscard]] bool ok() const { return move != ChessCore::Move::none(); }
        };

        // Mover-relative centipawns. Mate becomes +/-MATE_CP, sign preserved.
        // Returns false only if there is no parseable score at all.
        bool parse_score(const char*& p, const char* const end, Score& cp) {
            int sign = 1;
            if      (p < end && *p == '+') ++p;
            else if (p < end && *p == '-') { sign = -1; ++p; }

            if (p < end && (*p == 'M' || *p == 'm')) {
                ++p;
                while (p < end && is_digit(*p)) ++p;
                cp = static_cast<Score>(sign * MATE_CP);
                return true;
            }

            if (p >= end || !is_digit(*p)) return false;

            int whole = 0;
            while (p < end && is_digit(*p)) whole = whole * 10 + (*p++ - '0');

            int frac = 0, digits = 0;
            if (p < end && *p == '.') {
                ++p;
                while (p < end && is_digit(*p) && digits < 2) {
                    frac = frac * 10 + (*p++ - '0');
                    ++digits;
                }
            }
            while (digits++ < 2) frac *= 10;

            const int value = whole * 100 + frac;
            cp = static_cast<Score>(sign * (value > MATE_CP ? MATE_CP : value));
            return true;
        }

        SanResult resolve_san(const ChessCore::Position& pos, std::string_view san) {
            using ChessCore::Move;

            bool gives_check = false;
            while (!san.empty()) {
                const char c = san.back();
                if      (c == '+' || c == '#') { gives_check = true; san.remove_suffix(1); }
                else if (c == '!' || c == '?') { san.remove_suffix(1); }
                else break;
            }
            if (san.empty()) return { Move::none(), false };

            const bool white = pos.side_to_move() == Color::WHITE;

            if (san == "O-O" || san == "0-0")
                return { Move::make<MoveType::CASTLING>(white ? e1 : e8, white ? g1 : g8), gives_check };
            if (san == "O-O-O" || san == "0-0-0")
                return { Move::make<MoveType::CASTLING>(white ? e1 : e8, white ? c1 : c8), gives_check };

            PieceType promo = PieceType::EMPTY;
            if (san.size() > 2 && san[san.size() - 2] == '=') {
                promo = piece_type_of(san.back());
                if (promo == PieceType::EMPTY) return { Move::none(), false };
                san.remove_suffix(2);
            }

            PieceType moving = piece_type_of(san.front());
            size_t begin = 0;
            if (moving == PieceType::EMPTY) moving = PieceType::PAWN;
            else                            begin = 1;

            if (san.size() < begin + 2) return { Move::none(), false };
            const size_t dest = san.size() - 2;
            if (!is_file_ch(san[dest]) || !is_rank_ch(san[dest + 1]))
                return { Move::none(), false };

            const auto to = static_cast<Square>((san[dest] - 'a') + (san[dest + 1] - '1') * 8);

            int from_file = -1, from_rank = -1;
            for (size_t i = begin; i < dest; ++i) {
                const char c = san[i];
                if (c == 'x') continue;
                if      (is_file_ch(c)) from_file = c - 'a';
                else if (is_rank_ch(c)) from_rank = c - '1';
                else return { Move::none(), false };
            }

            Move found = Move::none();
            for (const Move m : pos.legal_moves()) {
                if (m.to() != to) continue;
                if (ChessCore::Pieces::getType(pos.square(m.from())) != moving) continue;
                if (from_file >= 0 && ChessCore::BitBoards::file_of(m.from()) != from_file) continue;
                if (from_rank >= 0 && ChessCore::BitBoards::rank_of(m.from()) != from_rank) continue;

                if (promo != PieceType::EMPTY) {
                    if (m.get_type() != MoveType::PROMOTION) continue;
                    if (m.promotion_type() != promo) continue;
                } else if (m.get_type() == MoveType::PROMOTION) {
                    continue;
                }

                assert(found == Move::none() && "ambiguous SAN - movegen disagrees with fastchess");
                found = m;
#ifdef NDEBUG
                break;
#endif
            }
            return { .move = found, .gives_check = gives_check };
        }

        // Replays the whole movetext, pushing every move. Returns false if any
        // token fails to resolve, in which case the game must be discarded.
        bool parse_movetext(const std::string_view text,
                            ChessCore::Position& pos,
                            ViriFormat::GameBuffer& game)
        {
            const char* p         = text.data();
            const char* const end = p + text.size();

            while (p < end) {
                while (p < end && static_cast<uint8_t>(*p) <= ' ') ++p;
                if (p == end) break;

                // move numbers ("9." "9...") and results ("1-0" "1/2-1/2" "*")
                if (is_digit(*p) || *p == '*') {
                    while (p < end && (is_digit(*p) || *p == '.' || *p == '-'
                                    || *p == '/' || *p == '*')) ++p;
                    continue;
                }

                const char* const san_begin = p;
                while (p < end && static_cast<uint8_t>(*p) > ' ' && *p != '{') ++p;
                const std::string_view san(san_begin, p - san_begin);

                while (p < end && static_cast<uint8_t>(*p) <= ' ') ++p;

                Score cp = 0;
                if (p < end && *p == '{') {
                    ++p;
                    if (!parse_score(p, end, cp)) cp = 0;
                    const auto close = static_cast<const char*>(std::memchr(p, '}', end - p));
                    p = close ? close + 1 : end;
                }

                // comment scores are mover-relative; viriformat wants white-relative
                if (pos.side_to_move() == Color::BLACK) cp = static_cast<Score>(-cp);

                const auto [m, gives_check] = resolve_san(pos, san);
                if (!m.operator!=(ChessCore::Move::none())) return false;   // unresolvable

                assert(gives_check == pos.gives_check(m) && "SAN suffix disagrees with gives_check");

                game.push(m, cp);
                pos.make_move(m, gives_check);
            }
            return true;
        }

        void flush(std::ofstream& out, std::vector<uint8_t>& buf) {
            if (buf.empty()) return;
            out.write(reinterpret_cast<const char*>(buf.data()),
                      static_cast<std::streamsize>(buf.size()));
            buf.clear();
        }
        struct ShardStats { size_t written = 0, dropped = 0; };

        // Read the whole PGN into memory. 3 GB against 32 GB of RAM is fine and
        // avoids platform-specific mmap.
        bool slurp(const std::string& path, std::string& out) {
            std::ifstream f(path, std::ios::binary | std::ios::ate);
            if (!f) return false;
            const auto size = f.tellg();
            if (size < 0) return false;
            out.resize(static_cast<size_t>(size));
            f.seekg(0);
            f.read(out.data(), size);
            return static_cast<bool>(f);
        }

        // Move `from` forward to the first byte of the next game.
        // "\n[Event " matches both LF and CRLF files.
        size_t snap_to_game(const std::string_view d, const size_t from) {
            if (from == 0) return 0;
            const size_t hit = d.find("\n[Event ", from);
            return hit == std::string_view::npos ? d.size() : hit + 1;
        }

        // ------------------------------------------------------------------
        // Processes one shard. Identical logic to the single-threaded version,
        // but iterates lines over a string_view instead of using getline, and
        // appends to a caller-owned byte vector instead of a file.
        // ------------------------------------------------------------------
        void process_range(const std::string_view pgn,
                           std::vector<uint8_t>& out,
                           ShardStats& stats)
        {
            using namespace ChessCore;

            std::string_view fen;          // points into `pgn`, which outlives us
            std::string      movetext;
            auto result  = ViriFormat::GameResult::DRAW;
            bool skip    = false;
            bool have_mt = false;

            ViriFormat::GameBuffer game;
            movetext.reserve(4096);

            const auto tag_value = [](const std::string_view line) -> std::string_view {
                const auto a = line.find('"');
                if (a == std::string_view::npos) return {};
                const auto b = line.find('"', a + 1);
                if (b == std::string_view::npos) return {};
                return line.substr(a + 1, b - a - 1);
            };

            const auto emit_game = [&] {
                if (!have_mt) return;

                if (!skip) {
                    Position pos{ fen.empty() ? std::string(FenHelper::STARTING_POSITION_FEN)
                                              : std::string(fen) };

                    game.begin(ViriFormat::PackedBoard(pos, 0, result));

                    if (parse_movetext(movetext, pos, game)) {
                        const auto& bytes = game.finish();
                        out.insert(out.end(), bytes.begin(), bytes.end());
                        ++stats.written;
                    } else {
                        ++stats.dropped;
                    }
                } else {
                    ++stats.dropped;
                }

                fen     = {};
                movetext.clear();
                result  = ViriFormat::GameResult::DRAW;
                skip    = false;
                have_mt = false;
            };

            size_t p = 0;
            while (p < pgn.size()) {
                size_t nl = pgn.find('\n', p);
                if (nl == std::string_view::npos) nl = pgn.size();

                std::string_view line = pgn.substr(p, nl - p);
                p = nl + 1;

                if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
                if (line.empty()) continue;

                if (line.front() == '[') {
                    if (have_mt) emit_game();

                    if (line.starts_with("[FEN ")) {
                        fen = tag_value(line);
                    } else if (line.starts_with("[Result ")) {
                        const auto r = tag_value(line);
                        result = (r == "1-0") ? ViriFormat::GameResult::WHITE_WIN
                               : (r == "0-1") ? ViriFormat::GameResult::BLACK_WIN
                                              : ViriFormat::GameResult::DRAW;
                        if (r == "*") skip = true;
                    } else if (line.starts_with("[Termination ")) {
                        if (tag_value(line).contains("abandoned")) skip = true;
                    }
                } else {
                    movetext += line;
                    movetext += ' ';
                    have_mt = true;
                }
            }
            emit_game();
        }


    } // namespace

    void pgn_to_viriformat(const std::string_view pgn_path, const std::string_view out_path) {
        using namespace ChessCore;

        std::ifstream pgn_file{std::string(pgn_path)};
        std::ofstream out_file{std::string(out_path), std::ios::binary};
        if (!pgn_file.is_open()) { std::cerr << "Error opening pgn file\n";   return; }
        if (!out_file)           { std::cerr << "Error creating output file\n"; return; }

        // --- per-game state, all declared OUTSIDE the read loop ------------
        std::string fen;
        std::string movetext;
        auto result   = ViriFormat::GameResult::DRAW;
        bool skip     = false;
        bool have_mt  = false;

        ViriFormat::GameBuffer game;
        std::vector<uint8_t> out_buf;
        out_buf.reserve(OUT_BUFFER_FLUSH + (64u << 10));

        size_t written = 0, dropped = 0;

        const auto tag_value = [](const std::string& line) -> std::string {
            const auto a = line.find('"');
            if (a == std::string::npos) return {};
            const auto b = line.find('"', a + 1);
            if (b == std::string::npos) return {};
            return line.substr(a + 1, b - a - 1);
        };

        const auto emit_game = [&] {
            if (!have_mt) return;

            if (!skip) {
                Position pos(fen.empty() ? std::string(FenHelper::STARTING_POSITION_FEN) : fen);

                // Header is the START position, before any move is made.
                game.begin(ViriFormat::PackedBoard(pos, 0, result));

                if (parse_movetext(movetext, pos, game)) {
                    const auto& bytes = game.finish();
                    out_buf.insert(out_buf.end(), bytes.begin(), bytes.end());
                    ++written;
                    if (out_buf.size() >= OUT_BUFFER_FLUSH) flush(out_file, out_buf);
                } else {
                    ++dropped;   // buffer discarded, never appended
                }
            } else {
                ++dropped;
            }

            fen.clear();
            movetext.clear();
            result  = ViriFormat::GameResult::DRAW;
            skip    = false;
            have_mt = false;
        };

        std::string line;
        while (std::getline(pgn_file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();   // CRLF
            if (line.empty()) continue;

            if (line.front() == '[') {
                if (have_mt) emit_game();        // a tag after movetext = new game

                if (line.starts_with("[FEN ")) {
                    fen = tag_value(line);
                } else if (line.starts_with("[Result ")) {
                    const auto r = tag_value(line);
                    result = (r == "1-0") ? ViriFormat::GameResult::WHITE_WIN
                           : (r == "0-1") ? ViriFormat::GameResult::BLACK_WIN
                                          : ViriFormat::GameResult::DRAW;
                    if (r == "*") skip = true;   // unfinished
                } else if (line.starts_with("[Termination ")) {
                    const auto cause = tag_value(line);
                    if (cause.contains("abandoned")) skip = true;
                }
            } else {
                movetext += line;
                movetext += ' ';                 // movetext wraps across lines
                have_mt = true;
            }
        }
        emit_game();                             // the final game
        flush(out_file, out_buf);

        std::cerr << "wrote " << written << " games, dropped " << dropped << '\n';
    }
    void pgn_to_viriformat_mt(const std::string_view pgn_path,
                              const std::string_view out_path,
                              unsigned threads)
    {
        std::string data;
        if (!slurp(std::string(pgn_path), data)) {
            std::cerr << "Error reading pgn file\n";
            return;
        }
        if (threads == 0) threads = std::max(1u, std::thread::hardware_concurrency());
        threads = static_cast<unsigned>(std::min<size_t>(threads, std::max<size_t>(1, data.size() / (1u << 20))));

        // --- boundaries, snapped forward to game starts and made monotonic ---
        std::vector<size_t> bounds(threads + 1);
        bounds[0] = 0;
        for (unsigned i = 1; i < threads; ++i)
            bounds[i] = snap_to_game(data, data.size() * i / threads);
        bounds[threads] = data.size();
        for (unsigned i = 1; i <= threads; ++i)
            bounds[i] = std::max(bounds[i], bounds[i - 1]);

        std::vector<std::vector<uint8_t>> shard_out(threads);
        std::vector<ShardStats>           shard_stats(threads);
        std::vector<std::thread>          pool;
        pool.reserve(threads);

        for (unsigned i = 0; i < threads; ++i) {
            if (bounds[i] == bounds[i + 1]) continue;      // empty shard
            pool.emplace_back([&, i] {
                shard_out[i].reserve(1u << 24);            // 16 MB, grows as needed
                process_range(std::string_view(data).substr(bounds[i], bounds[i + 1] - bounds[i]),
                              shard_out[i], shard_stats[i]);
            });
        }
        for (auto& t : pool) t.join();

        // --- write shards in order: deterministic output ---------------------
        std::ofstream out_file{ std::string(out_path), std::ios::binary };
        if (!out_file) { std::cerr << "Error creating output file\n"; return; }

        size_t written = 0, dropped = 0;
        for (unsigned i = 0; i < threads; ++i) {
            out_file.write(reinterpret_cast<const char*>(shard_out[i].data()),
                           static_cast<std::streamsize>(shard_out[i].size()));
            written += shard_stats[i].written;
            dropped += shard_stats[i].dropped;
        }

        std::cerr << "wrote " << written << " games, dropped " << dropped
                  << " (" << threads << " threads)\n";
    }
}
