//
// Created by FloopDjBoy on 07/09/2026.
//

#ifndef TATAKI_FITSCALE_H
#define TATAKI_FITSCALE_H
#include <string_view>

namespace FitScale {
    constexpr int    DEFAULT_CAP = 1500;   // ignore |cp| above this
    int fit_scale(const std::string& game_path,unsigned threads,int cap = DEFAULT_CAP);
} // FitScale

#endif //TATAKI_FITSCALE_H
