//
// Created by FloopDJBoy on 06/08/2026.
//

#pragma once
#include <cstdint>
#include "Position.h"
namespace ChessCore::preft {
    uint64_t divide(const Position& original, int depth);
    void test(const Position& original , int max_depth);

} // ChessCore

