#pragma once

struct Move {
	int from = 0, to = 0, attack_type = 0, defend_type = 0, special = 0;

	Move(int from, int to, int attack_type, int defend_type) : from(from), to(to), 
		attack_type(attack_type), defend_type(defend_type) {
		special = 0;
	}

	Move() {}

	bool operator==(const Move& other) {
		return (to == other.to && from == other.from && attack_type == other.attack_type && defend_type == other.defend_type && special == other.special);
	}

};


enum Color {
	WHITE = 0,
	BLACK = 1
};

enum Piece {
	PAWN = 0,
	KNIGHT = 1,
	BISHOP = 2,
	ROOK = 3,
	QUEEN = 4,
	KING = 5
};