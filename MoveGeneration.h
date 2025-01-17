#pragma once

#include <vector>

#include "BitBoard.h"
#include "Move.h"
#include "Position.h"

BitBoard GeneratePseudoPawnMove(Position& position, int p, int color, bool captures = false) {
	int dir = (color ? -1 : 1);
	int x = p / 8;
	int y = p % 8;
	BitBoard result = 0;
	for (int i = -1; i <= 1; i += 2) {
		int to = (x + dir) * 8 + y + i;
		if (y + i >= 0 && y + i < 8 && (position.GetColor(color ^ 1) & (1LLu << to))) {
			result |= (1LLu << to);
		}
	}
	if (!captures) {
		int to = (x + dir) * 8 + y;
		if ((position.GetAll() & (1LLu << to)) == 0) {
			result |= (1LLu << to);
			if ((color && x == 6) || (!color && x == 1)) {
				to += 8 * dir;
				if ((position.GetAll() & (1LLu << to)) == 0) {
					result |= (1LLu << to);
				}
			}
		}
	}
	
	return result;
}

namespace KnightMasks {
	static consteval uint8_t absSubtract(uint8_t left, uint8_t right) {
		if (left >= right) {
			return left - right;
		}
		return right - left;
	}
	static consteval std::array<BitBoard, 64> CalcMasks() {
		std::array<BitBoard, 64> masks{};

		uint8_t dx;
		uint8_t dy;

		for (uint8_t x0 = 0; x0 < 8; x0 = x0 + 1) {
			for (uint8_t y0 = 0; y0 < 8; y0 = y0 + 1) {

				for (uint8_t x1 = 0; x1 < 8; x1 = x1 + 1) {
					for (uint8_t y1 = 0; y1 < 8; y1 = y1 + 1) {

						dx = absSubtract(x0, x1);
						dy = absSubtract(y0, y1);

						if ((dx == 2 and dy == 1) or (dx == 1 and dy == 2)) {
							set_1(masks[y0 * 8 + x0], y1 * 8 + x1);
						}
					}
				}
			}
		}

		return masks;
	}
	static constexpr std::array<BitBoard, 64> MASKS = CalcMasks();
}

namespace KingMasks {
	static consteval uint8_t absSubtract(uint8_t left, uint8_t right) {
		if (left >= right) {
			return left - right;
		}
		return right - left;
	}
	static consteval std::array<BitBoard, 64> CalcMasks() {
		std::array<BitBoard, 64> masks{};

		uint8_t dx;
		uint8_t dy;

		for (uint8_t x0 = 0; x0 < 8; x0 = x0 + 1) {
			for (uint8_t y0 = 0; y0 < 8; y0 = y0 + 1) {

				for (uint8_t x1 = 0; x1 < 8; x1 = x1 + 1) {
					for (uint8_t y1 = 0; y1 < 8; y1 = y1 + 1) {

						dx = absSubtract(x0, x1);
						dy = absSubtract(y0, y1);

						if (dx <= 1 and dy <= 1) {
							set_1(masks[y0 * 8 + x0], y1 * 8 + x1);
						}
					}
				}
			}
		}

		return masks;
	}
	static constexpr std::array<BitBoard, 64> MASKS = CalcMasks();
}

BitBoard GeneratePseudoKnightMove(Position& position, int p, int color, bool captures = false) {
	BitBoard result = KnightMasks::MASKS[p];
	if (captures) return result & position.GetColor(color ^ 1);
	return result & (~position.GetColor(color));
	/*BitBoard result = 0;
	int x = p / 8;
	int y = p % 8;
	for (int i = -2; i <= 2; i++) {
		for (int j = -2; j <= 2; j++) {
			if (i == 0 || j == 0 || abs(i) == abs(j)) continue;
			if (x + i < 0 || x + i >= 8 || y + j < 0 || y + j >= 8) continue;
			int to = (x + i) * 8 + y + j;
			result |= (1LLu << to);
		}
	}
	return result & (~position.GetColor(color));*/
}

namespace SlidersMasks {
	struct Direction {
		static constexpr int8_t North = 0;
		static constexpr int8_t South = 1;
		static constexpr int8_t West = 2;
		static constexpr int8_t East = 3;

		static constexpr int8_t NorthWest = 4;
		static constexpr int8_t NorthEast = 5;
		static constexpr int8_t SouthWest = 6;
		static constexpr int8_t SouthEast = 7;
	};


	static consteval BitBoard calc_mask(uint8_t p, int8_t direction) {
		BitBoard mask = 0;

		int8_t x = p % 8;
		int8_t y = p / 8;

		for (; ;) {
			switch (direction) {
			case SlidersMasks::Direction::North: y = y + 1; break;
			case SlidersMasks::Direction::South: y = y - 1; break;
			case SlidersMasks::Direction::West: x = x - 1; break;
			case SlidersMasks::Direction::East: x = x + 1; break;

			case SlidersMasks::Direction::NorthWest: y = y + 1; x = x - 1; break;
			case SlidersMasks::Direction::NorthEast: y = y + 1; x = x + 1; break;
			case SlidersMasks::Direction::SouthWest: y = y - 1; x = x - 1; break;
			case SlidersMasks::Direction::SouthEast: y = y - 1; x = x + 1; break;
			}

			if (x > 7 or x < 0 or y > 7 or y < 0) break;

			set_1(mask, y * 8 + x);
		}

		return mask;
	}


	static consteval std::array<std::array<BitBoard, 8>, 64> calc_masks() {
		std::array<std::array<BitBoard, 8>, 64> masks{};

		for (uint8_t i = 0; i < 64; i = i + 1) {
			for (uint8_t j = 0; j < 8; j = j + 1) masks[i][j] = SlidersMasks::calc_mask(i, j);
		}

		return masks;
	}


	static constexpr std::array<std::array<BitBoard, 8>, 64> Masks = SlidersMasks::calc_masks();
};

BitBoard CalcRay(Position& position, int p, int color, int direction, int is_bsr, bool captures = false) {
	BitBoard blockers = SlidersMasks::Masks[p][direction] & position.GetAll();
	if (blockers == 0) {
		if (captures) return 0;
		return SlidersMasks::Masks[p][direction];
	}

	uint8_t blocking_square;

	if (is_bsr) blocking_square = bsr(blockers);
	else blocking_square = bsf(blockers);

	BitBoard moves;
	if (captures) moves = 0;
	else moves = SlidersMasks::Masks[p][direction] ^ SlidersMasks::Masks[blocking_square][direction];

	if (position.GetColor(color) & (1LLu << blocking_square)) set_0(moves, blocking_square);
	else set_1(moves, blocking_square);

	return moves;
}

BitBoard GeneratePseudoBishopMove(Position& position, int p, int color, bool captures = false) {
	BitBoard nw = CalcRay(position, p, color, SlidersMasks::Direction::NorthWest, false, captures);
	BitBoard ne = CalcRay(position, p, color, SlidersMasks::Direction::NorthEast, false, captures);
	BitBoard sw = CalcRay(position, p, color, SlidersMasks::Direction::SouthWest, true, captures);
	BitBoard se = CalcRay(position, p, color, SlidersMasks::Direction::SouthEast, true, captures);

	return nw | ne | sw | se;
	/*int x = p / 8;
	int y = p % 8;
	BitBoard result = 0;
	for (int i = -1; i <= 1; i += 2) {
		for (int j = -1; j <= 1; j += 2) {
			int nowx = x, nowy = y;
			while (1) {
				nowx += i; nowy += j;
				if (nowx < 0 || nowx >= 8 || nowy < 0 || nowy >= 8) break;
				int to = nowx * 8 + nowy;
				if (position.GetColor(color) & (1LLu << to)) break;
				result |= (1LLu << to);
				if (position.GetColor(color ^ 1) & (1LLu << to)) {
					break;
				}
			}
		}
	}
	return result;*/
}

BitBoard GeneratePseudoRookMove(Position& position, int p, int color, bool captures = false) {
	BitBoard n = CalcRay(position, p, color, SlidersMasks::Direction::North, false, captures);
	BitBoard s = CalcRay(position, p, color, SlidersMasks::Direction::South, true, captures);
	BitBoard w = CalcRay(position, p, color, SlidersMasks::Direction::West, true, captures);
	BitBoard e = CalcRay(position, p, color, SlidersMasks::Direction::East, false, captures);

	return n | s | w | e;
	/*int x = p / 8;
	int y = p % 8;
	BitBoard result = 0;
	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			if (((i == 0) ^ (j == 0)) == 0) continue;
			int nowx = x, nowy = y;
			while (1) {
				nowx += i; nowy += j;
				if (nowx < 0 || nowx >= 8 || nowy < 0 || nowy >= 8) break;
				int to = nowx * 8 + nowy;
				if (position.GetColor(color) & (1LLu << to)) break;

				result |= (1LLu << to);
				if (position.GetColor(color ^ 1) & (1LLu << to)) {
					break;
				}
			}
		}
	}
	return result;*/
}

BitBoard GeneratePseudoQueenMove(Position& position, int p, int color, bool captures = false) {
	int x = p / 8;
	int y = p % 8;
	BitBoard result = 0;
	return GeneratePseudoBishopMove(position, p, color, captures) | 
		GeneratePseudoRookMove(position, p, color, captures);
}

BitBoard GeneratePseudoKingMove(Position& position, int p, int color, bool captures = false) {
	BitBoard result = KingMasks::MASKS[p];
	if (captures) return result & position.GetColor(color ^ 1);
	return result & (~position.GetColor(color));
	/*int x = p / 8;
	int y = p % 8;
	BitBoard result = 0;
	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			if (i == 0 && j == 0) continue;
			if (x + i < 0 || x + i >= 8 || y + j < 0 || y + j >= 8) continue;
			int to = (x + i) * 8 + y + j;
			if (position.GetColor(color) & (1LLu << to)) {
				continue;
			}
			result |= (1LLu << to);
		}
	}
	return result;*/
}

BitBoard GeneratePseudoMove(Position& position, int p, int color, int piece, bool captures = false) {
	if (piece == 0) {
		return GeneratePseudoPawnMove(position, p, color, captures);
	}
	else if (piece == 1) {
		return GeneratePseudoKnightMove(position, p, color, captures);
	}
	else if (piece == 2) {
		return GeneratePseudoBishopMove(position, p, color, captures);
	}
	else if (piece == 3) {
		return GeneratePseudoRookMove(position, p, color, captures);
	}
	else if (piece == 4) {
		return GeneratePseudoQueenMove(position, p, color, captures);
	}
	else if (piece == 5) {
		return GeneratePseudoKingMove(position, p, color, captures);
	}
}

bool IsUnderAttack(Position position, int p, int color) {
	int x = p / 8;
	int y = p % 8;

	for (int i = 1; i < 6; ++i) {
		// set_0(position.pieces[color][Piece::KING], p_king);
		set_1(position.pieces[color][i], p);
		position.Update(color);
		if (GeneratePseudoMove(position, p, color, i, false) & position.pieces[color ^ 1][i]) return 0;
	}

	if (color == Color::WHITE) {
		for (int i = -1; i <= 1; i += 2) {
			if (y + i >= 0 && y + i < 8 && x + 1 < 8) {
				int pawn = (x + 1) * 8 + y + i;
				if (position.pieces[Color::BLACK][Piece::PAWN] & (1LLu << pawn)) return 0;
			}
		}
	}
	else {
		for (int i = -1; i <= 1; i += 2) {
			if (y + i >= 0 && y + i < 8 && x - 1 >= 0) {
				int pawn = (x - 1) * 8 + y + i;
				if (position.pieces[Color::WHITE][Piece::PAWN] & (1LLu << pawn)) return 0;
			}
		}
	}

	return 1;
}

bool IsLegalPosition(Position& position, int color, bool captures = false) {
	int p_king = bsf(position.pieces[color][Piece::KING]);
	int x = p_king / 8;
	int y = p_king % 8;

	for (int i = 1; i < 6; ++i) {
		// set_0(position.pieces[color][Piece::KING], p_king);
		//set_1(position.pieces[color][i], p_king);
		//position.Update(color);
		if (GeneratePseudoMove(position, p_king, color, i, false) & position.pieces[color ^ 1][i]) return 0;
	}

	if (color == Color::WHITE) {
		for (int i = -1; i <= 1; i += 2) {
			if (y + i >= 0 && y + i < 8 && x + 1 < 8) {
				int pawn = (x + 1) * 8 + y + i;
				if (position.pieces[Color::BLACK][Piece::PAWN] & (1LLu << pawn)) return 0;
			}
		}
	}
	else {
		for (int i = -1; i <= 1; i += 2) {
			if (y + i >= 0 && y + i < 8 && x - 1 >= 0) {
				int pawn = (x - 1) * 8 + y + i;
				if (position.pieces[Color::WHITE][Piece::PAWN] & (1LLu << pawn)) return 0;
			}
		}
	}

	return 1;
}

bool IsLegal(Position position, int color, Move move) {
	/*set_0(position.pieces[color][move.attack_type], move.from);
	set_1(position.pieces[color][move.attack_type], move.to);
	for (int i = 0; i < 6; ++i) {
		set_0(position.pieces[color ^ 1][i], move.to);
	}*/
	position.Do(move, color);
	// position.Update(color);
	// position.Update(color ^ 1);
	int p_king = bsf(position.pieces[color][Piece::KING]);
	int x = p_king / 8;
	int y = p_king % 8;

	for (int i = 1; i < 6; ++i) {
		// set_0(position.pieces[color][Piece::KING], p_king);
		//set_1(position.pieces[color][i], p_king);
		// position.Do(Move())
		//position.Update(color);
		if (GeneratePseudoMove(position, p_king, color, i, true) & position.pieces[color ^ 1][i]) return 0;
	}

	if (color == Color::WHITE) {
		for (int i = -1; i <= 1; i += 2) {
			if (y + i >= 0 && y + i < 8 && x + 1 < 8) {
				int pawn = (x + 1) * 8 + y + i;
				if (position.pieces[Color::BLACK][Piece::PAWN] & (1LLu << pawn)) return 0;
			}
		}
	}
	else {
		for (int i = -1; i <= 1; i += 2) {
			if (y + i >= 0 && y + i < 8 && x - 1 >= 0) {
				int pawn = (x - 1) * 8 + y + i;
				if (position.pieces[Color::WHITE][Piece::PAWN] & (1LLu << pawn)) return 0;
			}
		}
	}

	return 1;
}

void MaskToMove(Position& position, std::vector<Move>& result, BitBoard mask, int from, int type, int color) {
	// vector<Move> result;
	while (mask) {
		int x = bsf(mask);
		set_0(mask, x);
		int defend_type = -1;
		for (int u = 0; u < 6; ++u) {
			if (position.pieces[color ^ 1][u] & (1LLu << x)) {
				defend_type = u;
				break;
			}
		}

		Move move(from, x, type, defend_type);
		if (IsLegal(position, color, move)) {
			if (type == Piece::PAWN && (x / 8 == 0 || x / 8 == 7)) {
				// result.push_back(move);
				for (int i = 1; i <= 4; ++i) {
					move.special = i;
					//result.push_back(move);
				}
			}
			else {
				result.push_back(move);
			}
		}
	}
	// return result;
}

std::vector<Move> GetAllMoves(Position& position, int color, bool captures = false) {
	std::vector<Move> moves;	
    moves.reserve(218);

	
	BitBoard pawn_mask = position.pieces[color][Piece::PAWN];
	while (pawn_mask) {
		int x = bsf(pawn_mask);
		//set_0(pawn_mask, x);
		pawn_mask &= (pawn_mask - 1);
		BitBoard move_mask = GeneratePseudoPawnMove(position, x, color, captures);
		MaskToMove(position, moves, move_mask, x, Piece::PAWN, color);
	}
	
	BitBoard queen_mask = position.pieces[color][Piece::QUEEN];
	while (queen_mask) {
		int x = bsf(queen_mask);
		//set_0(queen_mask, x);
		queen_mask &= (queen_mask - 1);
		BitBoard move_mask = GeneratePseudoQueenMove(position, x, color, captures);
		MaskToMove(position, moves, move_mask, x, Piece::QUEEN, color);
	}


	BitBoard rook_mask = position.pieces[color][Piece::ROOK];
	while (rook_mask) {
		int x = bsf(rook_mask);
		//set_0(rook_mask, x);
		rook_mask &= (rook_mask - 1);
		BitBoard move_mask = GeneratePseudoRookMove(position, x, color, captures);
		MaskToMove(position, moves, move_mask, x, Piece::ROOK, color);
	}



	BitBoard bishop_mask = position.pieces[color][Piece::BISHOP];
	while (bishop_mask) {
		int x = bsf(bishop_mask);
		//set_0(bishop_mask, x);
		bishop_mask &= (bishop_mask - 1);
		BitBoard move_mask = GeneratePseudoBishopMove(position, x, color, captures);
		MaskToMove(position, moves, move_mask, x, Piece::BISHOP, color);
	}

	BitBoard knight_mask = position.pieces[color][Piece::KNIGHT];
	while (knight_mask) {
		int x = bsf(knight_mask);
		//set_0(knight_mask, x);
		knight_mask &= (knight_mask - 1);
		BitBoard move_mask = GeneratePseudoKnightMove(position, x, color, captures);
		MaskToMove(position, moves, move_mask, x, Piece::KNIGHT, color);
	}


	BitBoard king_mask = position.pieces[color][Piece::KING];
	while (king_mask) {
		int x = bsf(king_mask);
		//set_0(king_mask, x);
		king_mask &= (king_mask - 1);
		BitBoard move_mask = GeneratePseudoKingMove(position, x, color, captures);
		MaskToMove(position, moves, move_mask, x, Piece::KING, color);
	}

	/*
	if (color == Color::WHITE) {
		if (position.white_short_castling) {
			if (!IsUnderAttack(position, 4, color) && !IsUnderAttack(position, 5, color) &&
				!IsUnderAttack(position, 6, color) && (position.GetAll() & (3LLu << 5))) {
				Move move(0, 0, 0, 0);
				move.special = 5;
				moves.push_back(move);
			}
		}
	}*/

	return moves;
}