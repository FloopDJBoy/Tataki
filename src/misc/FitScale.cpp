//
// Created by FloopDjBoy on 07/09/2026.
//

#include "FitScale.h"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace FitScale {
    namespace {
        constexpr double MATE_MARK   = 1e9;

        // ---------------------------------------------------------------------------
        // Histogram over integer centipawns. r in {0, 0.5, 1} is white-relative.
        // ---------------------------------------------------------------------------
        struct Histogram {
            int cap;
            std::vector<uint64_t> count;
            std::vector<double>   sum_r;
            std::vector<double>   sum_r2;

            explicit Histogram(const int c)
                : cap(c), count(2 * c + 1, 0), sum_r(2 * c + 1, 0.0), sum_r2(2 * c + 1, 0.0) {}

            inline void add(const int cp, const double r) {
                if (cp < -cap || cp > cap) return;
                const size_t i = static_cast<size_t>(cp + cap);
                ++count[i];
                sum_r[i]  += r;
                sum_r2[i] += r * r;
            }

            void merge(const Histogram& o) {
                for (size_t i = 0; i < count.size(); ++i) {
                    count[i]  += o.count[i];
                    sum_r[i]  += o.sum_r[i];
                    sum_r2[i] += o.sum_r2[i];
                }
            }

            [[nodiscard]] uint64_t total() const {
                uint64_t n = 0;
                for (const auto c : count) n += c;
                return n;
            }
        };

        inline double sigmoid(const double x) {
            if (x < -60.0) return 0.0;
            if (x >  60.0) return 1.0;
            return 1.0 / (1.0 + std::exp(-x));
        }

        // MSE = (1/N) * sum_b [ n_b*s_b^2 - 2*s_b*sum_r_b + sum_r2_b ]
        double mse(const Histogram& h, const double k) {
            if (k <= 0.0) return 1e300;
            double acc = 0.0;
            uint64_t n = 0;
            for (size_t i = 0; i < h.count.size(); ++i) {
                if (!h.count[i]) continue;
                const double s = sigmoid((static_cast<double>(i) - h.cap) / k);
                acc += static_cast<double>(h.count[i]) * s * s - 2.0 * s * h.sum_r[i] + h.sum_r2[i];
                n   += h.count[i];
            }
            return n ? acc / static_cast<double>(n) : 1e300;
        }

        double logloss(const Histogram& h, const double k) {
            if (k <= 0.0) return 1e300;
            constexpr double eps = 1e-12;
            double acc = 0.0;
            uint64_t n = 0;
            for (size_t i = 0; i < h.count.size(); ++i) {
                if (!h.count[i]) continue;
                const double p = std::min(std::max(sigmoid((static_cast<double>(i) - h.cap) / k), eps), 1.0 - eps);
                acc -= h.sum_r[i] * std::log(p)
                     + (static_cast<double>(h.count[i]) - h.sum_r[i]) * std::log(1.0 - p);
                n   += h.count[i];
            }
            return n ? acc / static_cast<double>(n) : 1e300;
        }

        template <typename F>
        double golden(const Histogram& h, double a, double b, F objective, const int iters = 80) {
            constexpr double phi = 0.618033988749894848;
            double c = b - phi * (b - a), d = a + phi * (b - a);
            double fc = objective(h, c), fd = objective(h, d);
            for (int i = 0; i < iters; ++i) {
                if (fc < fd) { b = d; d = c; fd = fc; c = b - phi * (b - a); fc = objective(h, c); }
                else         { a = c; c = d; fc = fd; d = a + phi * (b - a); fd = objective(h, d); }
            }
            return 0.5 * (a + b);
        }

        // ---------------------------------------------------------------------------
        // Comment score: "+0.95/18 0.419s" -> 95.  Returns false for book/mate/garbage.
        // ---------------------------------------------------------------------------
        inline bool parse_score(const char*& p, const char* const end, int& cp) {
            int sign = 1;
            if      (p < end && *p == '+') ++p;
            else if (p < end && *p == '-') { sign = -1; ++p; }

            if (p >= end) return false;
            if (*p == 'M' || *p == 'm') return false;          // mate: no scale info
            if (!(static_cast<uint8_t>(*p - '0') < 10)) return false;   // "book", etc.

            int whole = 0;
            while (p < end && static_cast<uint8_t>(*p - '0') < 10) whole = whole * 10 + (*p++ - '0');

            int frac = 0, digits = 0;
            if (p < end && *p == '.') {
                ++p;
                while (p < end && static_cast<uint8_t>(*p - '0') < 10 && digits < 2) {
                    frac = frac * 10 + (*p++ - '0');
                    ++digits;
                }
            }
            while (digits++ < 2) frac *= 10;

            cp = sign * (whole * 100 + frac);
            return true;
        }

        // Side to move from a FEN tag line: [FEN "<board> w KQkq - 1 9"]
        bool fen_white_to_move(const std::string_view line) {
            const auto q = line.find('"');
            if (q == std::string_view::npos) return true;
            const auto sp = line.find(' ', q + 1);
            if (sp == std::string_view::npos || sp + 1 >= line.size()) return true;
            return line[sp + 1] != 'b';
        }

        // ---------------------------------------------------------------------------
        void scan_range(const std::string_view pgn, Histogram& h) {
            double result       = -1.0;    // -1 = unknown / skip
            bool   white_to_move = true;

            size_t p = 0;
            while (p < pgn.size()) {
                size_t nl = pgn.find('\n', p);
                if (nl == std::string_view::npos) nl = pgn.size();
                std::string_view line = pgn.substr(p, nl - p);
                p = nl + 1;

                if (!line.empty() && line.back() == '\r') line.remove_suffix(1);
                if (line.empty()) continue;

                if (line.front() == '[') {
                    if (line.starts_with("[Event ")) {
                        result = -1.0;
                        white_to_move = true;          // default: startpos
                    } else if (line.starts_with("[Result ")) {
                        const auto q = line.find('"');
                        if (q != std::string_view::npos) {
                            const auto v = line.substr(q + 1);
                            result = v.starts_with("1-0")     ? 1.0
                                   : v.starts_with("0-1")     ? 0.0
                                   : v.starts_with("1/2-1/2") ? 0.5
                                                              : -1.0;
                        }
                    } else if (line.starts_with("[FEN ")) {
                        white_to_move = fen_white_to_move(line);
                    }
                    continue;
                }

                if (result < 0.0) continue;            // unfinished game

                // movetext: every '{' opens a comment belonging to the move just played
                const char* q   = line.data();
                const char* end = q + line.size();
                while (q < end) {
                    const auto open = static_cast<const char*>(std::memchr(q, '{', end - q));
                    if (!open) break;
                    q = open + 1;

                    int cp = 0;
                    if (parse_score(q, end, cp)) {
                        h.add(white_to_move ? cp : -cp, result);
                    }
                    white_to_move = !white_to_move;

                    const auto close = static_cast<const char*>(std::memchr(q, '}', end - q));
                    q = close ? close + 1 : end;
                }
            }
        }

        size_t snap_to_game(const std::string_view d, const size_t from) {
            if (from == 0) return 0;
            const size_t hit = d.find("\n[Event ", from);
            return hit == std::string_view::npos ? d.size() : hit + 1;
        }
    } // namespace

    int fit_scale(const std::string& game_path,unsigned threads,int cap) {
        if (threads == 0) threads = std::max(1u, std::thread::hardware_concurrency());

        std::string data;
        {
            std::ifstream f(game_path, std::ios::binary | std::ios::ate);
            if (!f) { std::cerr << "cannot open " << game_path << '\n'; return 1; }
            const auto size = f.tellg();
            data.resize(size);
            f.seekg(0);
            f.read(data.data(), size);
        }
        threads = static_cast<unsigned>(std::min<size_t>(threads, std::max<size_t>(1, data.size() / (1u << 20))));

        std::vector<size_t> bounds(threads + 1);
        bounds[0] = 0;
        for (unsigned i = 1; i < threads; ++i) bounds[i] = snap_to_game(data, data.size() * i / threads);
        bounds[threads] = data.size();
        for (unsigned i = 1; i <= threads; ++i) bounds[i] = std::max(bounds[i], bounds[i - 1]);

        std::vector<Histogram> parts(threads, Histogram(cap));
        std::vector<std::thread> pool;
        for (unsigned i = 0; i < threads; ++i) {
            if (bounds[i] == bounds[i + 1]) continue;
            pool.emplace_back([&, i] {
                scan_range(std::string_view(data).substr(bounds[i], bounds[i + 1] - bounds[i]), parts[i]);
            });
        }
        for (auto& t : pool) t.join();

        Histogram h(cap);
        for (const auto& part : parts) h.merge(part);

        const uint64_t n = h.total();
        if (n < 1000) { std::cerr << "only " << n << " positions\n"; return 1; }

        const double k_mse = golden(h, 20.0, 2000.0, mse);
        const double k_ll  = golden(h, 20.0, 2000.0, logloss);

        // r in {0,0.5,1}: sum_r = w + 0.5d, sum_r2 = w + 0.25d  =>  d = 4*(sum_r - sum_r2)
        double draws = 0.0;
        for (size_t i = 0; i < h.count.size(); ++i)
            draws += 4.0 * (h.sum_r[i] - h.sum_r2[i]);

        std::cout << std::fixed;
        std::cout << "positions        : " << n << '\n'
                  << "draw rate        : " << std::setprecision(1) << 100.0 * draws / static_cast<double>(n) << "%\n"
                  << "threads          : " << threads << "\n\n"
                  << "K (min MSE)      : " << std::setprecision(1) << k_mse << '\n'
                  << "K (min log-loss) : " << k_ll << '\n'
                  << std::setprecision(5)
                  << "  MSE at K       : " << mse(h, k_mse)
                  << "   (at 400: " << mse(h, 400.0) << ")\n\n";

        static constexpr int EDGES[] = { -1000,-700,-500,-350,-250,-175,-120,-75,-35,
                                          35,75,120,175,250,350,500,700,1000 };
        std::cout << "calibration at K = " << std::setprecision(1) << k_mse << '\n'
                  << std::setw(16) << "bucket" << std::setw(12) << "n"
                  << std::setw(10) << "mean cp" << std::setw(9) << "actual"
                  << std::setw(11) << "predicted" << '\n';

        for (size_t e = 0; e + 1 < std::size(EDGES); ++e) {
            const int lo = std::max(EDGES[e], -cap), hi = std::min(EDGES[e + 1], cap);
            uint64_t bn = 0; double bsum = 0.0, bcp = 0.0;
            for (int c = lo; c < hi; ++c) {
                const auto i = static_cast<size_t>(c + cap);
                if (i >= h.count.size()) continue;
                bn   += h.count[i];
                bsum += h.sum_r[i];
                bcp  += static_cast<double>(h.count[i]) * c;
            }
            if (bn < 200) continue;
            const double mid = bcp / static_cast<double>(bn);
            std::cout << std::setw(8) << lo << " .." << std::setw(6) << hi
                      << std::setw(12) << bn
                      << std::setprecision(0) << std::setw(10) << mid
                      << std::setprecision(3) << std::setw(9) << bsum / static_cast<double>(bn)
                      << std::setw(11) << sigmoid(mid / k_mse) << '\n';
        }
        return 0;
    }
} // FitScale