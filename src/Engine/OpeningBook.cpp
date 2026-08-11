//
// Created by FloopDJBoy on 07/08/2026.
//

#include "OpeningBook.h"

#include <fstream>
#include <iostream>

namespace Engine {
    using namespace ChessCore;
    static uint16_t swap16(const uint16_t x)
    {
        return (x >> 8) | (x << 8);
    }


    static uint32_t swap32(const uint32_t x)
    {
        return ((x >> 24) & 0xff)
             | ((x >> 8)  & 0xff00)
             | ((x << 8)  & 0xff0000)
             | ((x << 24) & 0xff000000);
    }


    static uint64_t swap64(const uint64_t x)
    {
        return ((x >> 56) & 0x00000000000000FFULL)
             | ((x >> 40) & 0x000000000000FF00ULL)
             | ((x >> 24) & 0x0000000000FF0000ULL)
             | ((x >> 8)  & 0x00000000FF000000ULL)
             | ((x << 8)  & 0x000000FF00000000ULL)
             | ((x << 24) & 0x0000FF0000000000ULL)
             | ((x << 40) & 0x00FF000000000000ULL)
             | ((x << 56) & 0xFF00000000000000ULL);
    }
    void OpeningBook::load(const std::string &path) {
        std::ifstream file(path,std::ios::binary);
        if (!file) {
            std::cerr << "Could not open book: "
                  << path << "\n";
            return;
        }
        BookEntry entry{};
        while (file.read(reinterpret_cast<char*>(&entry), sizeof(BookEntry))) {
            entry.key = swap64(entry.key);
            entry.move = swap16(entry.move);
            entry.weight = swap16(entry.weight);
            entry.learn = swap32(entry.learn);
            entries.push_back(entry);
        }
        std::sort(entries.begin(),entries.end(),[](const BookEntry& a, const BookEntry& b){return a.key < b.key;});
    }
    OpeningBook::OpeningBook(const std::string &path, const uint64_t seed) : rng(seed) {
        load(path);
    }

    Move OpeningBook::operator[](const Position& position) const {
        //binary search by key
        const auto key = position.polyglot_hash();
        auto it = std::lower_bound(entries.begin(),entries.end(),key,[](const BookEntry& e, uint64_t k){return e.key < k;});
        if (it == entries.end() || it->key != key) {
            return Move::none();
        }
        std::vector<const BookEntry*> moves;
        for (auto p = it; p != entries.end() && p->key == key; ++p)
            moves.push_back(&*p);

        if (moves.size() == 1)

            return position.parse_move(moves[0]->from(),moves[0]->to(),moves[0]->promo());
        // weighted selection
        uint32_t total = 0;
        for (const auto e : moves)
            total += e->weight;

        uint32_t r = rng.next() % total;
        for (const auto e : moves)
        {
            if (r < e->weight)
                return position.parse_move(e->from(),e->to(),e->promo());
            r -= e->weight;
        }
        return Move::none();
    }
} // Engine