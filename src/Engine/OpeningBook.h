//
// Created by FloopDJBoy on 07/08/2026.
//

#ifndef CHESSENGINE_OPENINGBOOK_H
#define CHESSENGINE_OPENINGBOOK_H
#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

#include "Types.h"
#include "ChessCore/Move.h"
#include "ChessCore/Position.h"

namespace Engine {
    struct BookEntry
    {
        uint64_t key;
        uint16_t move;
        uint16_t weight;
        uint32_t learn;
        [[nodiscard]] Square from() const {
            return (move >> 6) & 0x3F;
        }

        [[nodiscard]] Square to() const {
            return move & 0x3F;
        }

        [[nodiscard]] PieceType promo() const {
            const auto p = (move >> 12) & 0x7;
            return p ==0 ? PieceType::EMPTY : static_cast<PieceType>(p+1);
        }
    };
    class OpeningBook {
        void load(const std::string& path);
        std::vector<BookEntry> entries;
        mutable SplitMix64 rng;
        public:
        static constexpr std::array POLYGLOT_RANDOM = {
    0x9cc3f25c7ed7d885ULL, 0xf6be604589d8db37ULL, 0x1dca0b8a3683f124ULL, 0x3d30b9bb1423406eULL,
    0x25f8222fa1e17d0ULL,  0xee16d16ccaa76a3bULL, 0xd027fbdd48cf49a0ULL, 0x6e26922ca5215c2ULL,
    0xde355325b3060aa3ULL, 0xafe13fa25bbda61aULL, 0x3af863f684a32ec4ULL, 0xbba1cf31bbba21c4ULL,
    0xf85d38ff91322fa0ULL, 0x8a902dfa66aa30a2ULL, 0xd98516aa87ab013eULL, 0xcbe94d13fa8ea2a2ULL,
    0xd95781a71e8bfb51ULL, 0xefd388f6fa98fa0aULL, 0xd389ca03e031ebbbULL, 0xfa3a61bfbbfa43bbULL,
    0x9ba6a31fe31ef0abULL, 0xfa0ae8fa7be0beabULL, 0x2ae1f9ca7baea4faULL, 0xefca4fa4facbeba4ULL,
    0xdca6e6bebe4be1f8ULL, 0xd8befbe7aa120a1cULL, 0x768fa2f7fb7faef8ULL, 0x3cbefa41cbe7faeaULL,
    0x8be4fab7f7be41e4ULL, 0xbfa80a37ff7bf0cbULL, 0xcbe17fcaefbeb76aULL, 0x29ef96a8faefba2fULL,
    0xd968fb7dfbae40aaULL, 0xd9a76be4afbe23abULL, 0x2fb0ea00a12e8ba0ULL, 0xceba1ca2faefbe5dULL,
    0xe17ff4faef48e234ULL, 0xbfa93e2bfe8fbaedULL, 0x2ba0ffae86f0ae81ULL, 0xdcae0a12fb7009aaULL,
    0xee1fa3efae1e4ab0ULL, 0x2bfba20eeae67f2eULL, 0xef2eb6a1112e4f0aULL, 0xcba0ffeaee4ba1eaULL,
    0xcba98fea1eff8ab0ULL, 0x2e8fbe5daea029beULL, 0xe1f6003ae6a02fb0ULL, 0xd916e6f1f486a43dULL,
    0xde355325b3060aa3ULL, 0xafe13fa25bbda61aULL, 0x3af863f684a32ec4ULL, 0xbba1cf31bbba21c4ULL,
    0xf85d38ff91322fa0ULL, 0x8a902dfa66aa30a2ULL, 0xd98516aa87ab013eULL, 0xcbe94d13fa8ea2a2ULL,
    0xd95781a71e8bfb51ULL, 0xefd388f6fa98fa0aULL, 0xd389ca03e031ebbbULL, 0xfa3a61bfbbfa43bbULL,
    0x9ba6a31fe31ef0abULL, 0xfa0ae8fa7be0beabULL, 0x2ae1f9ca7baea4faULL, 0xefca4fa4facbeba4ULL,
    0xdca6e6bebe4be1f8ULL, 0xd8befbe7aa120a1cULL, 0x768fa2f7fb7faef8ULL, 0x3cbefa41cbe7faeaULL,
    0x8be4fab7f7be41e4ULL, 0xbfa80a37ff7bf0cbULL, 0xcbe17fcaefbeb76aULL, 0x29ef96a8faefba2fULL,
    0xd968fb7dfbae40aaULL, 0xd9a76be4afbe23abULL, 0x2fb0ea00a12e8ba0ULL, 0xceba1ca2faefbe5dULL,
    0xe17ff4faef48e234ULL, 0xbfa93e2bfe8fbaedULL, 0x2ba0ffae86f0ae81ULL, 0xdcae0a12fb7009aaULL,
    0xee1fa3efae1e4ab0ULL, 0x2bfba20eeae67f2eULL, 0xef2eb6a1112e4f0aULL, 0xcba0ffeaee4ba1eaULL,
    0xcba98fea1eff8ab0ULL, 0x2e8fbe5daea029beULL, 0xe1f6003ae6a02fb0ULL, 0xd916e6f1f486a43dULL,
    0x29ef26a1be4bf23aULL, 0xdba67e817be4be8fULL, 0x2ca0fa1efae0beaeULL, 0xee297fb8ea7ba9fbULL,
    0xcbaef01ee7b97fafULL, 0x298f0ea2ea7be2f0ULL, 0x1efaee4beba60ee1ULL, 0xdba86e1eae7ba0ffULL,
    0x1e2fbaefba61f00aULL, 0xdcae07fba1ee02eaULL, 0x2bfba0ee7abceaa6ULL, 0xefaa3e9fa8ea6fa1ULL,
    0xdb6ef0beeae07fa6ULL, 0xcbfa8e6fbae0ea11ULL, 0x2ea1f6abfe67a10aULL, 0x1efb6e7fe6ae0ff0ULL,
    0x2ba97fae0ea0a6f1ULL, 0xdc86ef0aeb1ee1faULL, 0x2be4fa8ea3eefba1ULL, 0xee1a0ee3aeae7ff9ULL,
    0xcb6efa1efbca1ea8ULL, 0x2987fb0ea7ae3f1eULL, 0x2ba0ff8feae7be4aULL, 0xdbaf6e1fae7fbeabULL,
    0x1ea9ffbfae7bef2aULL, 0xdcbf7f1e6b81efbaULL, 0x2be6fae1ae8feabaULL, 0xef8f6eb1eae7ffbeULL,
    0xcba6faeaee7be29aULL, 0x298f7eabae4be2efULL, 0x2bf06a7faea7be5aULL, 0xdbe7fa01faefbaefULL,
    0x1ef97fbeee0ba2faULL, 0xdc7fbae718ef2ab0ULL, 0x2be8fa6faea1effaULL, 0xef0a9ea9a7fbebaeULL,
    0xcbae1faee7fa0dfaULL, 0x297fb7faef8be0afULL, 0x2bfb7fa6fae8beabULL, 0xdba1efabeae1ffe3ULL,
    0x29ef26a1be4bf23aULL, 0xdba67e817be4be8fULL, 0x2ca0fa1efae0beaeULL, 0xee297fb8ea7ba9fbULL,
    0xcbaef01ee7b97fafULL, 0x298f0ea2ea7be2f0ULL, 0x1efaee4beba60ee1ULL, 0xdba86e1eae7ba0ffULL,
    0x1e2fbaefba61f00aULL, 0xdcae07fba1ee02eaULL, 0x2bfba0ee7abceaa6ULL, 0xefaa3e9fa8ea6fa1ULL,
    0xdb6ef0beeae07fa6ULL, 0xcbfa8e6fbae0ea11ULL, 0x2ea1f6abfe67a10aULL, 0x1efb6e7fe6ae0ff0ULL,
    0x2ba97fae0ea0a6f1ULL, 0xdc86ef0aeb1ee1faULL, 0x2be4fa8ea3eefba1ULL, 0xee1a0ee3aeae7ff9ULL,
    0xcb6efa1efbca1ea8ULL, 0x2987fb0ea7ae3f1eULL, 0x2ba0ff8feae7be4aULL, 0xdbaf6e1fae7fbeabULL,
    0x1ea9ffbfae7bef2aULL, 0xdcbf7f1e6b81efbaULL, 0x2be6fae1ae8feabaULL, 0xef8f6eb1eae7ffbeULL,
    0xcba6faeaee7be29aULL, 0x298f7eabae4be2efULL, 0x2bf06a7faea7be5aULL, 0xdbe7fa01faefbaefULL,
    0x1ef97fbeee0ba2faULL, 0xdc7fbae718ef2ab0ULL, 0x2be8fa6faea1effaULL, 0xef0a9ea9a7fbebaeULL,
    0xcbae1faee7fa0dfaULL, 0x297fb7faef8be0afULL, 0x2bfb7fa6fae8beabULL, 0xdba1efabeae1ffe3ULL,
    0xef90faeebe0bef1eULL, 0xdb8fba0eefbfa29aULL, 0x2ba3ebfaea1fae7aULL, 0xee0e8fbe1ea67fabULL,
    0xcbafbe0ffae8febaULL, 0x29bfb6a0eae1eaeeULL, 0x1efa7fbaea2fb61aULL, 0xdba8fae6fac7be2aULL,
    0x1e8faef1eaef82f0ULL, 0xdca2ea0efae09efbULL, 0x2bfb8ea0ea1efbeeULL, 0xef6a7faee6bfba9aULL,
    0xdbbfbe6faea0ea6aULL, 0xcba8fea9ea07faeeULL, 0x2eae1fbf1effbaeaULL, 0x1ef8fee1bfa6efa1ULL,
    0x2ba6fae8fa0aef8aULL, 0xdc2eb6fae41ef0abULL, 0x2be1ffe1ebf7fbe0ULL, 0xee17fae6faef2ba1ULL,
    0xcb9ef0eaefbae7faULL, 0x296f8faee1ef6ea0ULL, 0x2ba7f0eaebffba2bULL, 0xdbef61a0ebfaeaeeULL,
    0x1ea3fbeffaebae0aULL, 0xdcbfa8ee3efba6efULL, 0x2be7fa9aebffba2aULL, 0xef7fbe0ee6ae7fbaULL,
    0xcba1fbee11faefbfULL, 0x298f0ea2ea7b00faULL, 0x2bf6abefe0ebfaefULL, 0xdbe1fa2efbe1e9baULL,
    0x1ef6fa2baea6faedULL, 0xdc8faee82fbaefa2ULL, 0x2be6faa6eaef2fbaULL, 0xef4fb0fae7fbe2baULL,
    0xcbaf0eaeee67fba1ULL, 0x29e1faeeae6fbaefULL, 0x2bfb7faea7fb0efaULL, 0xdba1faeeaeff6ba9ULL,
    0xef90faeebe0bef1eULL, 0xdb8fba0eefbfa29aULL, 0x2ba3ebfaea1fae7aULL, 0xee0e8fbe1ea67fabULL,
    0xcbafbe0ffae8febaULL, 0x29bfb6a0eae1eaeeULL, 0x1efa7fbaea2fb61aULL, 0xdba8fae6fac7be2aULL,
    0x1e8faef1eaef82f0ULL, 0xdca2ea0efae09efbULL, 0x2bfb8ea0ea1efbeeULL, 0xef6a7faee6bfba9aULL,
    0xdbbfbe6faea0ea6aULL, 0xcba8fea9ea07faeeULL, 0x2eae1fbf1effbaeaULL, 0x1ef8fee1bfa6efa1ULL,
    0x2ba6fae8fa0aef8aULL, 0xdc2eb6fae41ef0abULL, 0x2be1ffe1ebf7fbe0ULL, 0xee17fae6faef2ba1ULL,
    0xcb9ef0eaefbae7faULL, 0x296f8faee1ef6ea0ULL, 0x2ba7f0eaebffba2bULL, 0xdbef61a0ebfaeaeeULL,
    0x1ea3fbeffaebae0aULL, 0xdcbfa8ee3efba6efULL, 0x2be7fa9aebffba2aULL, 0xef7fbe0ee6ae7fbaULL,
    0xcba1fbee11faefbfULL, 0x298f0ea2ea7b00faULL, 0x2bf6abefe0ebfaefULL, 0xdbe1fa2efbe1e9baULL,
    0x1ef6fa2baea6faedULL, 0xdc8faee82fbaefa2ULL, 0x2be6faa6eaef2fbaULL, 0xef4fb0fae7fbe2baULL,
    0xcbaf0eaeee67fba1ULL, 0x29e1faeeae6fbaefULL, 0x2bfb7faea7fb0efaULL, 0xdba1faeeaeff6ba9ULL,
    0x2bfaee9feabfe3baULL, 0xee1fa3efae1e4ab0ULL, 0x2bfba20eeae67f2eULL, 0xef2eb6a1112e4f0aULL,
    0xcba0ffeaee4ba1eaULL, 0xcba98fea1eff8ab0ULL, 0x2e8fbe5daea029beULL, 0xe1f6003ae6a02fb0ULL,
    0xd916e6f1f486a43dULL, 0xde355325b3060aa3ULL, 0xafe13fa25bbda61aULL, 0x3af863f684a32ec4ULL,
    0xbba1cf31bbba21c4ULL, 0xf85d38ff91322fa0ULL, 0x8a902dfa66aa30a2ULL, 0xd98516aa87ab013eULL,
    0xcbe94d13fa8ea2a2ULL, 0xd95781a71e8bfb51ULL, 0xefd388f6fa98fa0aULL, 0xd389ca03e031ebbbULL,
    0xfa3a61bfbbfa43bbULL, 0x9ba6a31fe31ef0abULL, 0xfa0ae8fa7be0beabULL, 0x2ae1f9ca7baea4faULL,
    0xefca4fa4facbeba4ULL, 0xdca6e6bebe4be1f8ULL, 0xd8befbe7aa120a1cULL, 0x768fa2f7fb7faef8ULL,
    0x3cbefa41cbe7faeaULL, 0x8be4fab7f7be41e4ULL, 0xbfa80a37ff7bf0cbULL, 0xcbe17fcaefbeb76aULL,
    0x29ef96a8faefba2fULL, 0xd968fb7dfbae40aaULL, 0xd9a76be4afbe23abULL, 0x2fb0ea00a12e8ba0ULL,
    0xceba1ca2faefbe5dULL, 0xe17ff4faef48e234ULL, 0xbfa93e2bfe8fbaedULL, 0x2ba0ffae86f0ae81ULL,
    0xdcae0a12fb7009aaULL, 0x2bfaee9feabfe3baULL, 0xee1fa3efae1e4ab0ULL, 0x2bfba20eeae67f2eULL,
    0xef2eb6a1112e4f0aULL, 0xcba0ffeaee4ba1eaULL, 0xcba98fea1eff8ab0ULL, 0x2e8fbe5daea029beULL,
    0xe1f6003ae6a02fb0ULL, 0xd916e6f1f486a43dULL, 0xde355325b3060aa3ULL, 0xafe13fa25bbda61aULL,
    0x3af863f684a32ec4ULL, 0xbba1cf31bbba21c4ULL, 0xf85d38ff91322fa0ULL, 0x8a902dfa66aa30a2ULL,
    0xd98516aa87ab013eULL, 0xcbe94d13fa8ea2a2ULL, 0xd95781a71e8bfb51ULL, 0xefd388f6fa98fa0aULL,
    0xd389ca03e031ebbbULL, 0xfa3a61bfbbfa43bbULL, 0x9ba6a31fe31ef0abULL, 0xfa0ae8fa7be0beabULL,
    0x2ae1f9ca7baea4faULL, 0xefca4fa4facbeba4ULL, 0xdca6e6bebe4be1f8ULL, 0xd8befbe7aa120a1cULL,
    0x768fa2f7fb7faef8ULL, 0x3cbefa41cbe7faeaULL, 0x8be4fab7f7be41e4ULL, 0xbfa80a37ff7bf0cbULL,
    0xcbe17fcaefbeb76aULL, 0x29ef96a8faefba2fULL, 0xd968fb7dfbae40aaULL, 0xd9a76be4afbe23abULL,
    0x2fb0ea00a12e8ba0ULL, 0xceba1ca2faefbe5dULL, 0xe17ff4faef48e234ULL, 0xbfa93e2bfe8fbaedULL,
    0x2ba0ffae86f0ae81ULL, 0xdcae0a12fb7009aaULL, 0x1efaee4beba60ee1ULL, 0xdba86e1eae7ba0ffULL,
    0x1e2fbaefba61f00aULL, 0xdcae07fba1ee02eaULL, 0x2bfba0ee7abceaa6ULL, 0xefaa3e9fa8ea6fa1ULL,
    0xdb6ef0beeae07fa6ULL, 0xcbfa8e6fbae0ea11ULL, 0x2ea1f6abfe67a10aULL, 0x1efb6e7fe6ae0ff0ULL,
    0x2ba97fae0ea0a6f1ULL, 0xdc86ef0aeb1ee1faULL, 0x2be4fa8ea3eefba1ULL, 0xee1a0ee3aeae7ff9ULL,
    0xcb6efa1efbca1ea8ULL, 0x2987fb0ea7ae3f1eULL, 0x2ba0ff8feae7be4aULL, 0xdbaf6e1fae7fbeabULL,
    0x1ea9ffbfae7bef2aULL, 0xdcbf7f1e6b81efbaULL, 0x2be6fae1ae8feabaULL, 0xef8f6eb1eae7ffbeULL,
    0xcba6faeaee7be29aULL, 0x298f7eabae4be2efULL, 0x2bf06a7faea7be5aULL, 0xdbe7fa01faefbaefULL,
    0x1ef97fbeee0ba2faULL, 0xdc7fbae718ef2ab0ULL, 0x2be8fa6faea1effaULL, 0xef0a9ea9a7fbebaeULL,
    0xcbae1faee7fa0dfaULL, 0x297fb7faef8be0afULL, 0x2bfb7fa6fae8beabULL, 0xdba1efabeae1ffe3ULL,
    0x29ef26a1be4bf23aULL, 0xdba67e817be4be8fULL, 0x2ca0fa1efae0beaeULL, 0xee297fb8ea7ba9fbULL,
    0xcbaef01ee7b97fafULL, 0x298f0ea2ea7be2f0ULL, 0x1efaee4beba60ee1ULL, 0xdba86e1eae7ba0ffULL,
    0x1e2fbaefba61f00aULL, 0xdcae07fba1ee02eaULL, 0x2bfba0ee7abceaa6ULL, 0xefaa3e9fa8ea6fa1ULL,
    0xdb6ef0beeae07fa6ULL, 0xcbfa8e6fbae0ea11ULL, 0x2ea1f6abfe67a10aULL, 0x1efb6e7fe6ae0ff0ULL,
    0x2ba97fae0ea0a6f1ULL, 0xdc86ef0aeb1ee1faULL, 0x2be4fa8ea3eefba1ULL, 0xee1a0ee3aeae7ff9ULL,
    0xcb6efa1efbca1ea8ULL, 0x2987fb0ea7ae3f1eULL, 0x2ba0ff8feae7be4aULL, 0xdbaf6e1fae7fbeabULL,
    0x1ea9ffbfae7bef2aULL, 0xdcbf7f1e6b81efbaULL, 0x2be6fae1ae8feabaULL, 0xef8f6eb1eae7ffbeULL,
    0xcba6faeaee7be29aULL, 0x298f7eabae4be2efULL, 0x2bf06a7faea7be5aULL, 0xdbe7fa01faefbaefULL,
    0x1ef97fbeee0ba2faULL, 0xdc7fbae718ef2ab0ULL, 0x2be8fa6faea1effaULL, 0xef0a9ea9a7fbebaeULL,
    0xcbae1faee7fa0dfaULL, 0x297fb7faef8be0afULL, 0x2bfb7fa6fae8beabULL, 0xdba1efabeae1ffe3ULL,
    };
        explicit OpeningBook(const std::string& path,uint64_t seed = std::chrono::high_resolution_clock::now().time_since_epoch().count());
        ChessCore::Move operator[](const ChessCore::Position& position) const;


    };

} // Engine

#endif //CHESSENGINE_OPENINGBOOK_H
