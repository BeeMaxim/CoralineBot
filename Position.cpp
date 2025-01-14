#include <iostream>

#include "Position.h"


BitBoard Position::GetColor(int color) {
	// Update(color);
	return all[color];
	BitBoard result = 0;
	for (int i = 0; i < 6; ++i) {
		result |= pieces[color][i];
	}
	return result;
}

BitBoard Position::GetAll() {
	return (GetColor(0) | GetColor(1));
}

void Position::Do(Move& move, int color) {
	if (move.special == 5) {
		set_0(pieces[color][Piece::KING], 4);
		set_1(pieces[color][Piece::KING], 6);
		set_0(pieces[color][Piece::ROOK], 7);
		set_1(pieces[color][Piece::ROOK], 5);
		Update(color);
		white_short_castling = false;
		white_castling_happened = true;
		return;
	}

	set_0(pieces[color][move.attack_type], move.from);
	set_0(all[color], move.from);
	//all[color] &= pieces[color][move.attack_type];

	if (move.special > 0 && move.special <= 4) {
		set_1(pieces[color][move.special], move.to);		
	}
	else {
		set_1(pieces[color][move.attack_type], move.to);
	}
	set_1(all[color], move.to);

	if (move.defend_type != -1) {
		set_0(pieces[color ^ 1][move.defend_type], move.to);
		set_0(all[color ^ 1], move.to);
		//Update(color);
		//Update(color ^ 1);
	}


	hash_.invert_piece(move.from, move.attack_type, color);
	hash_.invert_piece(move.to, move.attack_type, color);
	if (move.defend_type != -1) {
		hash_.invert_piece(move.to, move.defend_type, color ^ 1);
	}
	//Update(color);
	//Update(color ^ 1);
	if (color == Color::WHITE && !white_short_castling) {
		if (~(pieces[color][Piece::KING] & (1LLu << 4)) || 
			~(pieces[color][Piece::ROOK] & (1LLu << 7))) {
			white_short_castling = false;
		}
		// std::cout << "LOOOOL\n";
	}
}

void Position::UnDo(Move& move, int color) {
	set_1(pieces[color][move.attack_type], move.from);
	set_1(all[color], move.from);

	if (move.special > 0 && move.special <= 4) {
		set_0(pieces[color][move.special], move.to);
	}
	else {
		set_0(pieces[color][move.attack_type], move.to);
	}
	set_0(all[color], move.to);

	if (move.defend_type != -1) {
		set_1(pieces[color ^ 1][move.defend_type], move.to);
		set_1(all[color ^ 1], move.to);
	}
	//Update(color);
	//Update(color ^ 1);
}

void Position::Update(int color) {
	this->all[color] = 0;
	for (int i = 0; i < 6; ++i) {
		this->all[color] |= pieces[color][i];
	}
	actual = true;
}