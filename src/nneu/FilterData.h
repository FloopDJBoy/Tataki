//
// Created by FloopDjBoy on 05/09/2026.
//

#ifndef TATAKI_FILTERDATA_H
#define TATAKI_FILTERDATA_H
#include <optional>
#include <string_view>

#include "Types.h"


namespace Engine::Eval::NNEU::FilterData {
    void pgn_to_viriformat(std::string_view pgn_path, std::string_view out_path);
    void pgn_to_viriformat_mt( std::string_view pgn_path, std::string_view out_path,unsigned threads);
};


#endif //TATAKI_FILTERDATA_H
