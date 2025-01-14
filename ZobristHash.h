#pragma once

#include <array>
#include <cstdint>

struct Position;

class ZobristHash {
public:
	uint64_t hash = 0;

	void init(Position& position);

	void invert_piece(uint8_t square, uint8_t type, uint8_t color);

	friend bool operator==(ZobristHash left, ZobristHash right);
	friend bool operator<(ZobristHash left, ZobristHash right);
};

static constexpr std::uint64_t Seed = 0x98f107;
static constexpr std::uint64_t Multiplier = 0x71abc9;
static constexpr std::uint64_t Summand = 0xff1b3f;


static consteval std::uint64_t next_random(std::uint64_t previous) {
	return Multiplier * previous + Summand;
}

static consteval std::array<std::array<std::array<std::uint64_t, 6>, 2>, 64> calc_constants() {
	std::array<std::array<std::array<uint64_t, 6>, 2>, 64> constants{};

	uint64_t previous = Seed;

	for (uint8_t square = 0; square < 64; ++square) {
		for (uint8_t side = 0; side < 2; ++side) {
			for (uint8_t type = 0; type < 6; ++type) {
				previous = next_random(previous);
				constants[square][side][type] = previous;
			}
		}
	}

	return constants;
}


static constexpr std::array<std::array<std::array<uint64_t, 6>, 2>, 64> Constants = calc_constants();
static constexpr std::uint64_t BlackMove = next_random(Constants[63][1][5]);
static constexpr std::uint64_t WhiteLongCastling = next_random(BlackMove);
static constexpr std::uint64_t WhiteShortCastling = next_random(WhiteLongCastling);
static constexpr std::uint64_t BlackLongCastling = next_random(WhiteShortCastling);
static constexpr std::uint64_t BlackShortCastling = next_random(BlackLongCastling);