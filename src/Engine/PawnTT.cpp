#include "PawnTT.h"

namespace Engine {

    PawnTT::PawnTT() : tt(std::make_unique<PawnEntry[]>(PAWN_TT_SIZE)) {
        clear();
    }

    void PawnTT::clear() {
        std::memset(tt.get(), 0, PAWN_TT_SIZE * sizeof(PawnEntry));
    }

} // namespace Engine