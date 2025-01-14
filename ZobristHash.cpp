#include "ZobristHash.h"
#include "Position.h"


void ZobristHash::init(Position& position) {
	this->hash = 0;
	uint8_t color;
	for (uint8_t square = 0; square < 64; ++square) {
		if (position.GetColor(Color::WHITE) & (1LLu << square)) color = Color::WHITE;
		else if (position.GetColor(Color::BLACK) & (1LLu << square)) color = Color::BLACK;
		else continue;

		for (uint8_t type = 0; type < 6; ++type) {
			if (position.pieces[color][type] & (1LLu << square)) {
				this->invert_piece(square, type, color);
				break;
			}
		}
	}
}

void ZobristHash::invert_piece(uint8_t square, uint8_t type, uint8_t color) {
	this->hash ^= Constants[square][color][type];
}

bool operator==(ZobristHash left, ZobristHash right) {
	return (left.hash == right.hash);
}

bool operator<(ZobristHash left, ZobristHash right) {
	return (left.hash < right.hash);
}