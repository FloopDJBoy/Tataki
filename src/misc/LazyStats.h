//
// Created by FloopDJBoy on 14/08/2026.
//

#ifndef CHESSENGINE_LAZYSTATS_H
#define CHESSENGINE_LAZYSTATS_H
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>

#include "Types.h"

namespace Engine::LazyTuning {
    struct LazyStats{
        uint64_t count = 0;
        std::array<uint64_t, 1000> buckets{}; // Buckets from 0 to 999 centipawns error
        int max_error = 0;
        void add(const int error) {
            ++count;
            const Score clamped_err = std::clamp(error, 0, 999);
            buckets[clamped_err]++;
            max_error = std::max(max_error, error);
        }
        void print_report() const {
            if (count == 0) return;

            uint64_t running_sum = 0;
            int p90 = 0, p95 = 0, p98 = 0, p99 = 0, p99_5 = 0;

            for (int i = 0; i < 1000; ++i) {
                running_sum += buckets[i];
                const double pct = (static_cast<double>(running_sum) / count) * 100.0;

                if (pct >= 90.0 && p90 == 0) p90 = i;
                if (pct >= 95.0 && p95 == 0) p95 = i;
                if (pct >= 98.0 && p98 == 0) p98 = i;
                if (pct >= 99.0 && p99 == 0) p99 = i;
                if (pct >= 99.5 && p99_5 == 0) p99_5 = i;
            }

            std::cout << "\n=========================================\n";
            std::cout << "     LAZY EVALUATION ERROR REPORT        \n";
            std::cout << "=========================================\n";
            std::cout << "Positions Processed : " << count << "\n";
            std::cout << "Max Observed Delta  : " << max_error << " cp\n";
            std::cout << "-----------------------------------------\n";
            std::cout << "90.0th Percentile   : " << p90   << " cp\n";
            std::cout << "95.0th Percentile   : " << p95   << " cp\n";
            std::cout << "98.0th Percentile   : " << p98   << " cp\n";
            std::cout << "99.0th Percentile   : " << p99   << " cp\n";
            std::cout << "99.5th Percentile   : " << p99_5 << " cp\n";
            std::cout << "=========================================\n";
        }
    };
    void run_lazy_tuning(const std::string& epd_path);

} // Engine::LazyStats

#endif //CHESSENGINE_LAZYSTATS_H
