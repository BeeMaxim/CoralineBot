#pragma once 

#include "BitBoard.h"
#include "Move.h"
#include "ZobristHash.h"

class ZobristHash;

struct Position {
	
	int color = Color::WHITE;
	ZobristHash hash_;
	bool white_short_castling = true;
	bool white_long_castling = true;
	bool black_short_castling = true;
	bool black_long_castling = true;
	bool white_castling_happened = false;
	bool black_castling_happened = false;
	bool actual = false;

	BitBoard GetColor(int color);

	BitBoard GetAll();

	void Do(Move& move, int color);

	void UnDo(Move& move, int color);

	void Update(int color);


	BitBoard pieces[2][6];
	BitBoard all[2];
};