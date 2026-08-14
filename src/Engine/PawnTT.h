#ifndef CHESSENGINE_PAWNTT_H
#define CHESSENGINE_PAWNTT_H

#include <memory>
#include <cstring>
#include "Types.h"

namespace Engine {

    struct alignas(16) PawnEntry {
        Key key{0};
        ScorePair score{0, 0};
    };

    static_assert(sizeof(PawnEntry) == 16);
    static_assert(alignof(PawnEntry) == 16);

    class PawnTT {
    public:
        // 32,768 entries (~512 KB memory footprint)
        constexpr static size_t PAWN_TT_BITS = 15;
        constexpr static size_t PAWN_TT_SIZE = 1ULL << PAWN_TT_BITS; 
        constexpr static uint64_t PAWN_TT_MASK = PAWN_TT_SIZE - 1;

        PawnTT();

        void clear();


        [[nodiscard]] PawnEntry* operator[](const Key key) const {
            PawnEntry* e = &tt[key & PAWN_TT_MASK];
            return (e->key == key) ? e : nullptr;
        }

        void insert(const Key key, const ScorePair score) {
            tt[key & PAWN_TT_MASK] = PawnEntry{ .key = key, .score = score };
        }

    private:
        std::unique_ptr<PawnEntry[]> tt;
    };

} // namespace Engine

#endif // CHESSENGINE_PAWNTT_H