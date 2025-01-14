#pragma once

#include <array>
#include <bit>
#include <cstdint>

typedef uint64_t BitBoard;

constexpr inline uint8_t count_1(BitBoard bb) {
    return std::popcount(bb);
}

constexpr inline void set_1(BitBoard& bb, uint8_t square) {
	bb = bb | (1ull << square);
}

 constexpr inline void set_0(BitBoard& bb, uint8_t square) {
	bb = bb & (~(1ull << square));
}

static constexpr std::array<uint8_t, 64> BitScanTable = {
	0, 47,  1, 56, 48, 27,  2, 60,
	57, 49, 41, 37, 28, 16,  3, 61,
	54, 58, 35, 52, 50, 42, 21, 44,
	38, 32, 29, 23, 17, 11,  4, 62,
	46, 55, 26, 59, 40, 36, 15, 53,
	34, 51, 20, 43, 31, 22, 10, 45,
	25, 39, 14, 33, 19, 30,  9, 24,
	13, 18,  8, 12,  7,  6,  5, 63
};

constexpr inline uint8_t bsf(BitBoard bb) {
	return BitScanTable[((bb ^ (bb - 1)) * 0x03f79d71b4cb0a89) >> 58];
}

constexpr inline uint8_t bsr(BitBoard bb) {
	bb = bb | (bb >> 1);
	bb = bb | (bb >> 2);
	bb = bb | (bb >> 4);
	bb = bb | (bb >> 8);
	bb = bb | (bb >> 16);
	bb = bb | (bb >> 32);

	return BitScanTable[(bb * 0x03f79d71b4cb0a89) >> 58];
}