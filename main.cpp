#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include <string>
#include <array>
#include <map>
#include <algorithm>
#include <future>
#include <chrono>
#include <bit>


#include "BitBoard.h"
#include "ZobristHash.h"
#include "Position.h"
#include "Move.h"
#include "MoveGeneration.h"
#include "newBots.h"

#include "position_.h"
#include "bitboard_.h"
#include "search.h"


atomic<bool> STOP = false;
int COUNTER = 0;
int TIMER = -1;




// vector<vector<int>> mark_tables[10];
TranspositionTable tt(50000);


string from_code(Move m) {
	string arr = "";
	if (m.special == 5) {
		arr = "0-0";
		return arr;
	}
	int x1 = m.from / 8;
	int y1 = m.from % 8;
	int x2 = m.to / 8;
	int y2 = m.to % 8;
	arr += 'a' + y1;
	arr += to_string(x1 + 1);
	arr += 'a' + y2;
	arr += to_string(x2 + 1);
	return arr;
}

vector<ZobristHash> history;


vector<int> PAWN_POSITIONS = {
	0, 0, 0, 0, 0, 0, 0, 0,
	4, 4, 4, 0, 0, 4, 4, 4,
	6, 8, 2, 10, 10, 2, 8, 6,
	6, 8, 12, 16, 16, 12, 8, 6,
	8, 12, 16, 24, 24, 16, 12, 8,
	12, 16, 24, 32, 32, 24, 16, 12,
    12, 16, 24, 32, 32, 24, 16, 12,
    0, 0, 0, 0, 0, 0, 0, 0
};

vector<int> KING_POSITIONS = {
	0, 0, -4, -10, -10, -4, 0, 0,
	-4, -4, -8, -12, -12, -8, -4, -4,
	-12, -16, -20, -20, -20, -20, -16, -12,
	-16, -20, -24, -24, -24, -24, -20, -16,
	-16, -20, -24, -24, -24, -24, -20, -16,
	-12, -16, -20, -20, -20, -20, -16, -12,
	-4, -4, -8, -12, -12, -8, -4, -4,
	0, 0, -4, -10, -10, -4, 0, 0 
};

vector<int> LATE_KING_POSITIONS = { 
	0, 6, 12, 18, 18, 12, 6, 0,
	6, 12, 18, 24, 24, 18, 12, 6,
	12, 18, 24, 30, 30, 24, 18, 12,
	18, 24, 30, 36, 36, 30, 24, 18,
	18, 24, 30, 36, 36, 30, 24, 18,
	12, 18, 24, 30, 30, 24, 18, 12,
	6, 12, 18, 24, 24, 18, 12, 6,
	0, 6, 12, 18, 18, 12, 6, 0 
};

int distance(int p1, int p2) {
	int x1 = p1 / 8, y1 = p1 % 8;
	int x2 = p2 / 8, y2 = p2 % 8;
	return max(abs(x1 - x2), abs(y1 - y2));
}

unordered_map<uint64_t, int> static_cash;

int MAX_STATIC_CASH_SIZE = 10000;

void ReplaceInStaticCash() {
	while (static_cash.size() > MAX_STATIC_CASH_SIZE) {
		static_cash.erase(static_cash.begin());
	}
}

int mobile[5] = {0, 8, 4, 2, 2};

int marker(Position& position) {
	++COUNT;
	// auto start = std::chrono::high_resolution_clock::now();
	
	auto it = static_cash.find(position.hash_.hash);
	
	if (it != static_cash.end()) {
		return it->second;
	}
	++COUNTER;
	int result = 0;
	int cost[6] = { 100, 300, 330, 500, 900, 1000 };
	// int mobile[5] = { 0, 8, 4, 2, 2 };
	int mains[2][2];

	for (int color = 0; color <= 1; ++color) {
		mains[color][0] = bsf(position.pieces[color][Piece::QUEEN]);
		mains[color][1] = bsf(position.pieces[color][Piece::KING]);
	}
	for (int color = 0; color <= 1; ++color) {
		int dir = (color ? -1 : 1);

		
		for (int i = 0; i < 5; ++i) {
			result += dir * cost[i] * count_1(position.pieces[color][i]);
		}
		
		for (int i = 1; i < 5; ++i) {
			BitBoard mask = position.pieces[color][i];
			while (mask) {
				int p = bsf(mask);
				set_0(mask, p);
				result += dir * mobile[i] * count_1(GeneratePseudoMove(position, p, color, i));
			}
		}

		//auto start = std::chrono::high_resolution_clock::now();
		
		BitBoard mask = position.pieces[color][0];
		while (mask) {
			int p = bsf(mask);
			set_0(mask, p);
			if (color == Color::WHITE) result += PAWN_POSITIONS[p];
			else result -= PAWN_POSITIONS[63 - p];
		}

		//auto finish = std::chrono::high_resolution_clock::now();
    	//TIMER += std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count();
		/*
		if (!position.white_castling_happened && !position.white_short_castling) {
			result -= 40;
		}*/
		mask = position.pieces[color][Piece::KING];
		if (count_1(position.GetAll()) > 11) {		
			result += dir * KING_POSITIONS[bsf(mask)];
		}
		else {
			result += dir * LATE_KING_POSITIONS[bsf(mask)];
		}
		// int queen = bsf(position.pieces[color][Piece::QUEEN]);
		// int king = bsf(position.pieces[color ^ 1][Piece::KING]);

		// if (position.pieces[color][Piece::QUEEN]) result += -4 * dir * distance(mains[color][0], mains[color ^ 1][1]);

		// mask = position.pieces[color][0];
	}
	ReplaceInStaticCash();
	static_cash[position.hash_.hash] = result;

	return result;
}

std::random_device rd;
std::mt19937 g(rd());

struct Entry {
	Entry(ZobristHash hash_, int32_t depth, Move best_move) : _hash(hash_), _depth(depth), 
		_best_move(best_move) {}

	friend bool operator <(Entry left, Entry right);

	ZobristHash _hash;
	int32_t _depth;
	Move _best_move;
};

bool operator <(Entry left, Entry right) {
	return left._hash < right._hash;
}

size_t hashFor(const ZobristHash& foo) {
    return foo.hash;
}

template <>
struct std::hash<ZobristHash> {
	std::size_t operator()(const ZobristHash& k) const {
		return k.hash;
	}
};

struct Ent {
	int index = 0;
	int score = 0;
	int info = 0;
	int deep = 0;
	int color = -1;
};

unordered_map<ZobristHash, Ent> cash;

bool Is3Times() {
	int cnt = 0;
	for (auto i : history) {
		if (i == history.back()) {
			++cnt;
		}
	}
	if (cnt >= 3) return 1;
	return 0;
}


int CASHES = 0;
unordered_map<ZobristHash, Ent> captures_cash;

int CaptureNEGAB(int color, Position& position, int alpha, int beta) {
	if (STOP) return 0;
	int score = marker(position);
	if (color == Color::BLACK) score = -score;
	//return score;

	alpha = max(alpha, score);

	vector<Move> result = GetAllMoves(position, color, true);

	auto key = [](Move& a, Move& b) -> bool {
		//if (a.defend_type == -1) return 0;
		//if (b.defend_type == -1) return 1;
		if (a.defend_type == b.defend_type) return a.attack_type < b.attack_type;
		return a.defend_type > b.defend_type;
		//return (a.attack_type - a.defend_type < b.attack_type - b.defend_type);
	};

	std::sort(result.begin(), result.end(), key);

	// int index = 0;

	for (auto& move : result) {
		// if (move.defend_type == -1) break;
		Position copy = position;
		copy.Do(move, color);
		//position.Do(move, color);

		score = -CaptureNEGAB(color ^ 1, copy, -beta, -alpha);

		//position.UnDo(move, color);

		alpha = max(alpha, score);

		if (alpha >= beta) break;
		// index += 1;
	}

	return alpha;
}

int EXACT = 0;

Move killers[1001];


int MAX_CASH_SIZE = 10000;


void ReplaceInCash() {
	while (cash.size() > MAX_CASH_SIZE) {
		cash.erase(cash.begin());
	}
}


int InnerNEGAB(int deep, int color, Position position, int alpha, int beta, int ply) {
	
	if (STOP) return 0;
	if (deep <= 0) {
		int score = CaptureNEGAB(color, position, alpha, beta);

		return score;
	}
	
	bool in_check = !IsLegalPosition(position, color);

	if (in_check && ply < 12) deep += 1;
	history.push_back(position.hash_);
	
	if (Is3Times()) {
		history.pop_back();
		return 0;
	}

	ReplaceInCash();
	auto ptr = cash.find(position.hash_);
	int best_move = -1;
	if (ptr != cash.end()) {
		int info = ptr->second.info;
		int sc = ptr->second.score;
	
		if (info && ptr->second.deep >= deep && ptr->second.color == color) {
			++CASHES;
			// cout << deep << ' ' << ptr->second.deep << '\n';
			// cout << "!!!\n" << ptr->second.score << '\n';
			
			if (info == 4 && info == 1) {
				//alpha = max(alpha, sc);
			}
			if (info == 1) {
				history.pop_back();
				return sc;
			}
			else if (info == 2 && sc <= alpha) {
				history.pop_back();
				return alpha;
			}
			else if (info == 4 && sc >= beta) {
				// ++CASHES;
				history.pop_back();
				return beta;
			}
			// else if (info == 4) alpha = max(alpha, sc);
		}
		// ++CASHES;
		// return 0;
		// cout << "???\n";
		best_move = ptr->second.index;
	}

	auto start = std::chrono::high_resolution_clock::now();

	vector<Move> result = GetAllMoves(position, color);

	if (best_move >= result.size()) best_move = -1;
	
	int cost = -10000 - deep;

	if (result.empty()) {
		history.pop_back();
		if (!in_check) {
			return 0;
		}
		return cost;
	}
	// cout << "deep: " << deep << '\n';
	// cout << -InnerNEGAB(deep - 3, color ^ 1, position, -beta, -alpha) << ' ' << beta << '\n';
	/*
	if (-InnerNEGAB(deep - 4, color ^ 1, position, -beta, -beta + 1) >= beta) { // null move
		history.pop_back();
		return beta; 
	}*/
	
	auto key = [&position, &color, &in_check, &ply](Move& a, Move& b) -> bool {
		if (in_check) {
			if (a.attack_type == Piece::KING && b.attack_type == Piece::KING) return 0;
			if (a.attack_type == Piece::KING) return 1;
			return 0;
		}
		if (a.defend_type == -1 && b.defend_type == -1) {
			//if (a == killers[ply]) return 1;
			//if (b == killers[ply]) return 0;
			// int mob_a = mobile[a.attack_type] * (count_1(GeneratePseudoMove(position, a.to, color, a.attack_type)) - count_1(GeneratePseudoMove(position, a.from, color, a.attack_type)));
			// int mob_b = mobile[b.attack_type] * (count_1(GeneratePseudoMove(position, b.to, color, b.attack_type)) - count_1(GeneratePseudoMove(position, b.from, color, b.attack_type)));
			// return (mob_a > mob_b);
			if (a.attack_type == Piece::PAWN && b.attack_type == Piece::PAWN) {
				int pawn_a, pawn_b;
				if (color == Color::WHITE) {
					pawn_a = PAWN_POSITIONS[a.to] - PAWN_POSITIONS[a.from];
					pawn_b = PAWN_POSITIONS[b.to] - PAWN_POSITIONS[b.from];
				}
				else {
					pawn_a = PAWN_POSITIONS[63 - a.to] - PAWN_POSITIONS[63 - a.from];
					pawn_b = PAWN_POSITIONS[63 - b.to] - PAWN_POSITIONS[63 - b.from];
				}
				return (pawn_a > pawn_b);
			}

			int a_type = -1;
			int b_type = -1;
			int x = a.to / 8;
			int y = a.to % 8;
			int dir = (color ? -1 : 1);
			
			for (int i = -1; i <= 1; i += 2) {
				if (x + dir >= 0 && x + dir < 8 && y + i >= 0 && y + i < 8) {
					int next = (x + dir) * 8 + y + i;
					if (position.pieces[color ^ 1][Piece::PAWN] & (1LLu << next)) {
						a_type = a.attack_type;
					}
				}
			}
			x = b.to / 8;
			y = b.to % 8;
			for (int i = -1; i <= 1; i += 2) {
				if (x + dir >= 0 && x + dir < 8 && y + i >= 0 && y + i < 8) {
					int next = (x + dir) * 8 + y + i;
					if (position.pieces[color ^ 1][Piece::PAWN] & (1LLu << next)) {
						b_type = b.attack_type;
					}
				}
			}

			// cout << '!' << ' ' << a_type << ' ' << b_type << endl;
			// Position cop1 = position, cop2 = position;
			// cop1.Do(a, color);
			// cop2.Do(b, color);
			// return a.attack_type > b.attack_type;
			if (a_type == -1 && b_type == -1) {
				//int mob_a = mobile[a.attack_type] * (count_1(GeneratePseudoMove(position, a.to, color, a.attack_type)) - count_1(GeneratePseudoMove(position, a.from, color, a.attack_type)));
				//int mob_b = mobile[b.attack_type] * (count_1(GeneratePseudoMove(position, b.to, color, b.attack_type)) - count_1(GeneratePseudoMove(position, b.from, color, b.attack_type)));
				//return (mob_a > mob_b);
				//return (a.attack_type > b.attack_type);
			}

			return a_type < b_type;

		}
		if (a.defend_type == -1) return 0;
		if (b.defend_type == -1) return 1;
		if (a.defend_type == b.defend_type) return a.attack_type < b.attack_type;
		return a.defend_type > b.defend_type;
		// return (a.attack_type - a.defend_type < b.attack_type - b.defend_type);
	};

	// auto start = std::chrono::high_resolution_clock::now();
	// std::cout << "before sort\n";
	auto finish = std::chrono::high_resolution_clock::now();

	
	std::sort(result.begin(), result.end(), key);
	// std::cout << "after sort\n";
	
    TIMER += std::chrono::duration_cast<std::chrono::nanoseconds>(finish-start).count() ;
	int R = 3;
	/*
	if (!in_check &&  ply > 1 && -InnerNEGAB(deep - deep / 6 - R - 1, color ^ 1, position, -beta, -alpha, ply + 1) >= beta) { // null move
		history.pop_back();
		return beta; 
	}*/
	/*
	if (deep <= 2 && !in_check) {
		int margin = 50;
		int static_score = marker(position);
		if (color == Color::BLACK) static_score = -static_score;
		if (static_score - margin >= beta) {
			history.pop_back();
			return beta;
		}
	}*/

	int score;
	bool first = false;
	bool full_mark = 0;
	
	if (best_move != -1) {
		Position copy = position;
		copy.Do(result[best_move], color);

		score = -InnerNEGAB(deep - 1, color ^ 1, copy, -beta, -alpha, ply + 1);

		if (score > cost) {
			cash[position.hash_].index = best_move;
			cost = score;
		}
		if (cost > alpha) {
			full_mark = 1;
			alpha = cost;
		}
		if (alpha >= beta) {
			if (deep >= cash[position.hash_].deep && cash[position.hash_].info != 1) {
				cash[position.hash_].score = alpha;
				cash[position.hash_].deep = deep;
				cash[position.hash_].color = color;
				cash[position.hash_].info = 4;
			}
			history.pop_back();
			return alpha;
		}
		first = true;
	}


	int index = 0;
	bool was_killer = true;
	Move killer_move;

	for (auto& move : result) {
		
		if (index == best_move) {
			++index;
			continue;
		}

		if (!was_killer && killers[ply].to != killers[ply].from && move.defend_type == -1) {
			bool ok = false;
			for (auto& i : result) {
				if (i == killers[ply]) ok = true;
			}
			if (ok) {
				killer_move = killers[ply];
				Position copy = position;
				copy.Do(killers[ply], color);

				score = -InnerNEGAB(deep - 1, color ^ 1, copy, -(alpha + 1), -alpha, ply + 1);
				if (score > alpha && score < beta) {
					score = -InnerNEGAB(deep - 1, color ^ 1, copy, -beta, -alpha, ply + 1);
				}

				if (score > cost) {
					cost = score;
					cash[position.hash_].index = index;
				}
				if (cost > alpha) {
					full_mark = 1;
					alpha = cost;
				}
				if (alpha >= beta) {
					if (deep >= cash[position.hash_].deep && cash[position.hash_].info != 1) {
						cash[position.hash_].score = alpha;
						cash[position.hash_].deep = deep;
						cash[position.hash_].color = color;
						cash[position.hash_].info = 4;
					}
					history.pop_back();
					return alpha;
				}
				was_killer = true;
			}
		}
		if (was_killer && killer_move == move) {
			++index;
			continue;
		}

		Position copy = position;
		copy.Do(move, color);
		if (first) {
			
			score = -InnerNEGAB(deep - 1, color ^ 1, copy, -(alpha + 1), -alpha, ply + 1);
			if (score > alpha && score < beta) {
				score = -InnerNEGAB(deep - 1, color ^ 1, copy, -beta, -alpha, ply + 1);
			}
		}
		else {
			score = -InnerNEGAB(deep - 1, color ^ 1, copy, -beta, -alpha, ply + 1);
		}

		// position.UnDo(move, color);

		// cost = max(cost, score);
		if (score > cost) {
			cost = score;
			/*
			if (index == 0 && best_move != -1) {
				cash[position.hash_].index = best_move;
			}
			else if (index == best_move) cash[position.hash_].index = 0;
			else cash[position.hash_].index = index;*/
			cash[position.hash_].index = index;
			// cash[position.hash_].info = 0;
			// cash[position.hash_].color = color;
			// cout << position.hash_.hash << ' ' << cash.size() << endl;
		}
		if (cost > alpha) {
			full_mark = 1;
			alpha = cost;
		}
		// alpha = max(alpha, score);
		if (alpha >= beta) {
			//if (index == 0 && best_move != -1) cash[position.hash_].index = best_move;
			//else if (index == best_move) cash[position.hash_].index = 0;
			//else cash[position.hash_].index = index;
			if (deep >= cash[position.hash_].deep && cash[position.hash_].info != 1) {
				cash[position.hash_].score = alpha;
				cash[position.hash_].deep = deep;
				cash[position.hash_].color = color;
				cash[position.hash_].info = 4;
			}
			if (move.defend_type == -1) killers[ply] = move;
			//killers[ply + 1] = {0, 0, 0, 0};
			history.pop_back();
			return alpha;
		}

		index += 1;
		first = true;
	}
	
	history.pop_back();
	++EXACT;
	
	if (deep >= cash[position.hash_].deep) {
		cash[position.hash_].score = alpha;
		cash[position.hash_].deep = deep;
		cash[position.hash_].color = color;
		if (full_mark) {
			cash[position.hash_].info = 1;
		}
		else {
			cash[position.hash_].info = 2;
		}
	}

	return cost;
}



pair<Move, int> NEGAB(int deep, int color, Position position, int alpha, int beta) {
	if (STOP) return { { 0, 0, 0, 0 }, 0 };
	if (deep == 0) {
		if (color == Color::WHITE) return { {0, 0, 0, 0}, marker(position) };
		else return { {0, 0, 0, 0}, -marker(position) };
	}

	
	history.push_back(position.hash_);
	if (Is3Times()) {
		history.pop_back();
		return { {0, 0, 0, 0}, 0 };
	}

	vector<Move> result = GetAllMoves(position, color);
	int cost = -10000 - deep;
	vector<Move> to_ch(0);

	bool in_check = !IsLegalPosition(position, color);
	// std::cout << deep << ' ' << alpha << ' '<< beta << '\n';
	auto key = [&position, &color, &in_check](Move& a, Move& b) -> bool {
		if (in_check) {
			if (a.attack_type == Piece::KING && b.attack_type == Piece::KING) return 0;
			if (a.attack_type == Piece::KING) return 1;
			return 0;
		}
		// if (a.special == 5) return 1;
		// if (b.special == 5) return 0;
		if (a.defend_type == -1 && b.defend_type == -1) {
			// int mob_a = mobile[a.attack_type] * (count_1(GeneratePseudoMove(position, a.to, color, a.attack_type)) - count_1(GeneratePseudoMove(position, a.from, color, a.attack_type)));
			// int mob_b = mobile[b.attack_type] * (count_1(GeneratePseudoMove(position, b.to, color, b.attack_type)) - count_1(GeneratePseudoMove(position, b.from, color, b.attack_type)));
			// return (mob_a > mob_b);
			if (a.attack_type == Piece::PAWN && b.attack_type == Piece::PAWN) {
				int pawn_a, pawn_b;
				if (color == Color::WHITE) {
					pawn_a = PAWN_POSITIONS[a.to] - PAWN_POSITIONS[a.from];
					pawn_b = PAWN_POSITIONS[b.to] - PAWN_POSITIONS[b.from];
				}
				else {
					pawn_a = PAWN_POSITIONS[63 - a.to] - PAWN_POSITIONS[63 - a.from];
					pawn_b = PAWN_POSITIONS[63 - b.to] - PAWN_POSITIONS[63 - b.from];
				}
				return (pawn_a > pawn_b);
			}

			int a_type = -1;
			int b_type = -1;
			int x = a.to / 8;
			int y = a.to % 8;
			int dir = (color ? -1 : 1);
			
			for (int i = -1; i <= 1; i += 2) {
				if (x + dir >= 0 && x + dir < 8 && y + i >= 0 && y + i < 8) {
					int next = (x + dir) * 8 + y + i;
					if (position.pieces[color ^ 1][Piece::PAWN] & (1LLu << next)) {
						a_type = a.attack_type;
					}
				}
			}
			x = b.to / 8;
			y = b.to % 8;
			for (int i = -1; i <= 1; i += 2) {
				if (x + dir >= 0 && x + dir < 8 && y + i >= 0 && y + i < 8) {
					int next = (x + dir) * 8 + y + i;
					if (position.pieces[color ^ 1][Piece::PAWN] & (1LLu << next)) {
						b_type = b.attack_type;
					}
				}
			}
			// cout << '!' << ' ' << a_type << ' ' << b_type << endl;
			// Position cop1 = position, cop2 = position;
			// cop1.Do(a, color);
			// cop2.Do(b, color);
			// return a.attack_type > b.attack_type;
			if (a_type == -1 && b_type == -1) {
				//int mob_a = mobile[a.attack_type] * (count_1(GeneratePseudoMove(position, a.to, color, a.attack_type)) - count_1(GeneratePseudoMove(position, a.from, color, a.attack_type)));
				//int mob_b = mobile[b.attack_type] * (count_1(GeneratePseudoMove(position, b.to, color, b.attack_type)) - count_1(GeneratePseudoMove(position, b.from, color, b.attack_type)));
				//return (mob_a > mob_b);
				//return (a.attack_type > b.attack_type);
			}

			return a_type < b_type;

		}
		if (a.defend_type == -1) return 0;
		if (b.defend_type == -1) return 1;
		if (a.defend_type == b.defend_type) return a.attack_type < b.attack_type;
		return a.defend_type > b.defend_type;
		// return (a.attack_type - a.defend_type < b.attack_type - b.defend_type);
	};


	std::sort(result.begin(), result.end(), key);
	// std::cout << "after result\n";
	
	auto ptr = cash.find(position.hash_);

	int best_move = -1;

	ReplaceInCash();
	if (ptr != cash.end()) {
		if (ptr->second.index < result.size()) {
			// cout << ptr->second << endl;
			//swap(result[0], result[ptr->second.index]);
			best_move = ptr->second.index;
		}
	}

	bool first = false;
	int score;

	// std::cout << "before best_move\n";

	if (best_move != -1) {
		Position copy = position;
		copy.Do(result[best_move], color);

		score = -InnerNEGAB(deep - 1, color ^ 1, copy, -beta, -alpha, 1);
		// std::cout << "score " << score << '\n';

		if (score > cost) {
			to_ch.push_back(result[best_move]);
			cost = score;
			cash[position.hash_].index = best_move;
		}
		alpha = max(alpha, score);

		first = true;
	}

	
	int index = 0;
	// cout << "!!!\n";
	for (auto& move : result) {
		if (index == best_move) {
			++index;
			continue;
		}
		// cout << deep << ' '  << alpha << ' ' << beta << '\n';
		Position copy = position;
		copy.Do(move, color);
		if (first) {
			score = -InnerNEGAB(deep - 1, color ^ 1, copy, -(alpha + 1), -alpha, 1);
			if (score > alpha && score < beta) {
				score = -InnerNEGAB(deep - 1, color ^ 1, copy, -beta, -alpha, 1);
			}
		}
		else {
			score = -InnerNEGAB(deep - 1, color ^ 1, copy, -beta, -alpha, 1);
		}
		// position.UnDo(move, color);

		// cout << score << ' ' << move.attack_type << ' ' << move.from << endl;

		if (score > cost) {
			to_ch.clear();
			to_ch.push_back(move);
			cash[position.hash_].index = index;
			//if (index == 0 && best_move != -1) {
			//}
			//else if (index == best_move) cash[position.hash_].index = 0;
			//else cash[position.hash_].index = index;
			// cash[position.hash_].info = 0;
			// cash[position.hash_].color = color;
			// cash[position.hash_] = index;
		}
		else if (score == cost) {
			to_ch.push_back(move);
		}
		cost = max(cost, score);
		alpha = max(alpha, score);

		// if (alpha >= beta) return { { 0, 0, 0, 0 }, alpha };

		// first = 1;
		index += 1;
	}

	// cout << "???\n";

	for (auto i : to_ch) {
		// cout << i.attack_type << ' ' << i.from << ' ' << i.to << endl;
	}


	// shuffle(to_ch.begin(), to_ch.end(), g);
	history.pop_back();
	if (to_ch.empty()) {
		
		if (IsLegalPosition(position, color)) {
			return { { 0, 0, 0, 0 }, 0 };
		}
		return { { 0, 0, 0, 0 }, cost };
	}
	// if (to_ch.size() > 1) return { to_ch[1], cost };

	cash[position.hash_].info = 1;
	cash[position.hash_].deep = deep;
	cash[position.hash_].score = cost;

	return { to_ch[0], cost };
}


Move NEG2AB(int color, Position& position) {
	int alpha = -100000, beta = 100000;
	// return LLIN4AB(color, position);
	return NEGAB(2, color, position, alpha, beta).first;
}

Move NEG4AB(int color, Position& position) {
	int alpha = -100000, beta = 100000;
	// return LLIN4AB(color, position);
	return NEGAB(4, color, position, alpha, beta).first;
}

Move NEG5AB(int color, Position& position) {
	int alpha = -100000, beta = 100000;
	return NEGAB(5, color, position, alpha, beta).first;
}

Move NEG6AB(int color, Position& position) {
	int alpha = -100000, beta = 100000;
	return NEGAB(6, color, position, alpha, beta).first;
}

Move NEGABTIME_test(int color, Position& position) {
	Position copy = position;
	STOP = false;
	time_t start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	// time_t stop_time = 2000000000;
	Move move;
	cash.clear();
	static_cash.clear();
	COUNTER = 0;
	CASHES = 0;
	EXACT = 0;
	for (int32_t deep = 1; deep < 8; ++deep) {
		cout << "------------------------DEEP: " << deep << "-----------------------------\n";
		int alpha = -100000, beta = 100000;
		std::future<pair<Move, int>> thread = std::async(NEGAB, deep, color, copy, alpha, beta);
		bool search = true;

		if (search) {
			auto result = thread.get();
			move = result.first;
		}
		else {
			STOP = true;
			thread.get();
			break;
		}

		// std::cerr << "base depth: " << deep << endl;
	}
	cout << "cash size: " << cash.size() << '\n';
	cout << "COUNTER: " << COUNT << '\n';
	cout << "marker time: " << TIMER << '\n';
	cout << "CASHES: " << CASHES << '\n';
	cout << "EXACT: " << EXACT << '\n';
	COUNT = 0;
	STOP = false;
	return move;
}

Move NEGABTIME(int color, Position& position, time_t stop_time) {
	STOP = false;
	time_t start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	// time_t stop_time = 2000000000;
	Move move = GetAllMoves(position, color)[0];
	
	for (int32_t deep = 1; deep < 1000; ++deep) {
		int alpha = -100000, beta = 100000;
		std::future<pair<Move, int>> thread = std::async(NEGAB, deep, color, position, alpha, beta);
		//std::future<pair<Move, int>> thread = std::async(searc, deep, position, alpha, beta);
		bool search = true;
		while (thread.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
			// cout << std::chrono::high_resolution_clock::now().time_since_epoch().count() - start << endl;
			if ((std::chrono::high_resolution_clock::now().time_since_epoch().count() - start) >= stop_time) {
				search = false;
				break;
			}
			// usleep(20000);
		}

		if (search) {
			auto result = thread.get();
			move = result.first;
		}
		else {
			STOP = true;
			thread.get();
			break;
		}

		std::cerr << "base depth: " << deep << endl;
	}
	STOP = false;
	return move;
}

int to_code(char c) {
	if (c == '1' || c == 'h') return 1;
	if (c == '2' || c == 'g') return 2;
	if (c == '3' || c == 'f') return 3;
	if (c == '4' || c == 'e') return 4;
	if (c == '5' || c == 'd') return 5;
	if (c == '6' || c == 'c') return 6;
	if (c == '7' || c == 'b') return 7;
	if (c == '8' || c == 'a') return 8;
}

string from_code(newmove m) {
	string arr = "";
	//cout << m.stX << ' ' << m.stY << ' ' << m.fX << ' ' << m.fY << ' ' << m.type << endl;
	if (m.oldY == 1) arr += 'h';
	if (m.oldY == 2) arr += 'g';
	if (m.oldY == 3) arr += 'f';
	if (m.oldY == 4) arr += 'e';
	if (m.oldY == 5) arr += 'd';
	if (m.oldY == 6) arr += 'c';
	if (m.oldY == 7) arr += 'b';
	if (m.oldY == 8) arr += 'a';
	arr += to_string(m.oldX);
	if (m.newY == 1) arr += 'h';
	if (m.newY == 2) arr += 'g';
	if (m.newY == 3) arr += 'f';
	if (m.newY == 4) arr += 'e';
	if (m.newY == 5) arr += 'd';
	if (m.newY == 6) arr += 'c';
	if (m.newY == 7) arr += 'b';
	if (m.newY == 8) arr += 'a';
	arr += to_string(m.newX);
	return arr;
}

typedef pair<newmove, int> pni;
typedef vector<vector<int>>& vvi;


void newBattle(newmove bot1(int, vvi), newmove bot2(int, vvi)) {
	vector<vector<int>> field(10, vector<int>(10, 0));
	field[1][1] = field[1][8] = 4;
	field[1][2] = field[1][7] = 2;
	field[1][3] = field[1][6] = 3;
	field[1][4] = 6; field[1][5] = 5;
	field[8][1] = field[8][8] = -4;
	field[8][2] = field[8][7] = -2;
	field[8][3] = field[8][6] = -3;
	field[8][4] = -6; field[8][5] = -5;
	for (int i = 1; i <= 8; i++) field[2][i] = 1;
	for (int i = 1; i <= 8; i++) field[7][i] = -1;
	int cnt = 0;
	while (1) {
		int start = clock();
		newmove M = bot1(1, field);
		if (M.newX == 0) {
			cout << "SECOND WIN!!!" << endl; break;
		}
		cnt++;
		if (M.type != 0) field[M.newX][M.newY] = M.type;
		else field[M.newX][M.newY] = field[M.oldX][M.oldY];
		field[M.oldX][M.oldY] = 0;
		cout << "White: " << from_code(M) << endl << endl;
		cout << clock() - start << endl; start = clock();
		M = bot2(-1, field);
		if (M.newX == 0) {
			cout << "FIRST WIN!!!" << endl; break;
		}
		if (M.type != 0) field[M.newX][M.newY] = M.type;
		else field[M.newX][M.newY] = field[M.oldX][M.oldY];
		field[M.oldX][M.oldY] = 0;
		cout << "Black: " << from_code(M) << endl << endl;
		cnt++;
		cout << clock() - start << endl;
	}
	cout << "Moves : " << cnt;
}

void newBattle1(Move bot1(int, Position&), Move bot2(int, Position&)) {
	// vector<vector<int>> field(10, vector<int>(10, 0));
	Position position;
	/*
	field[1][1] = field[1][8] = 4;
	field[1][2] = field[1][7] = 2;
	field[1][3] = field[1][6] = 3;
	field[1][4] = 6; field[1][5] = 5;
	field[8][1] = field[8][8] = -4;
	field[8][2] = field[8][7] = -2;
	field[8][3] = field[8][6] = -3;
	field[8][4] = -6; field[8][5] = -5;
	for (int i = 1; i <= 8; i++) field[2][i] = 1;
	for (int i = 1; i <= 8; i++) field[7][i] = -1;*/
	for (int i = 0; i <= 1; ++i) {
		for (int j = 0; j < 6; ++j) position.pieces[i][j] = 0;
	}
	for (int i = 0; i < 8; ++i) {
		set_1(position.pieces[0][Piece::PAWN], 8 + i);
		set_1(position.pieces[1][Piece::PAWN], 55 - i);
	}
	set_1(position.pieces[0][Piece::KNIGHT], 1);
	set_1(position.pieces[0][Piece::KNIGHT], 6);
	set_1(position.pieces[1][Piece::KNIGHT], 57);
	set_1(position.pieces[1][Piece::KNIGHT], 62);

	set_1(position.pieces[0][Piece::BISHOP], 2);
	set_1(position.pieces[0][Piece::BISHOP], 5);
	set_1(position.pieces[1][Piece::BISHOP], 58);
	set_1(position.pieces[1][Piece::BISHOP], 61);

	set_1(position.pieces[0][Piece::ROOK], 0);
	set_1(position.pieces[0][Piece::ROOK], 7);
	set_1(position.pieces[1][Piece::ROOK], 56);
	set_1(position.pieces[1][Piece::ROOK], 63);

	set_1(position.pieces[0][Piece::QUEEN], 3);
	set_1(position.pieces[0][Piece::KING], 4);
	set_1(position.pieces[1][Piece::QUEEN], 59);
	set_1(position.pieces[1][Piece::KING], 60);

	position.hash_.init(position);
	position.Update(Color::WHITE);
	position.Update(Color::BLACK);

	cout << "START!!!" << endl;

	int cnt = 0;
	while (1) {
		int start = clock();
		Move M = bot1(0, position);
		if (M.to == M.from) {
			cout << "SECOND WIN!!!" << endl; break;
		}
		cnt++;
		int attack_type = M.attack_type;
		int defend_type = -1;
		for (int u = 0; u < 6; ++u) {
			if (position.pieces[1][u] & (1LLu << M.to)) defend_type = u;
		}
		position.Do(M, Color::WHITE);
		/*
		set_0(position.pieces[0][M.attack_type], M.from);
		set_1(position.pieces[0][M.attack_type], M.to);
		if (defend_type != -1) {
			set_0(position.pieces[1][defend_type], M.to);
		}*/
		cout << "White: " << from_code(M) << endl << endl;
		cout << clock() - start << endl; start = clock();
		position.color ^= 1;
		M = bot2(1, position);
		if (M.from == M.to) {
			cout << "FIRST WIN!!!" << endl; break;
		}
		attack_type = M.attack_type;
		/*
		defend_type = -1;
		for (int u = 0; u < 6; ++u) {
			if (position.pieces[0][u] & (1LLu << M.to)) defend_type = u;
		}
		set_0(position.pieces[1][M.attack_type], M.from);
		set_1(position.pieces[1][M.attack_type], M.to);
		if (defend_type != -1) {
			set_0(position.pieces[0][defend_type], M.to);
		}*/
		position.Do(M, Color::BLACK);
		cout << "Black: " << from_code(M) << endl << endl;
		cnt++;
		cout << clock() - start << endl;
	}
	cout << "Moves : " << cnt;
}

Position FromFen(string fen) {
	int color = -1;
	int y = 8;
	int x = 0;
	Position position;

	for (int i = 0; i <= 1; ++i) {
		for (int j = 0; j < 6; ++j) position.pieces[i][j] = 0;
	}
	for (int i = 0; i < fen.size(); ++i) {
		int to = (y - 1) * 8 + x;
		if (fen[i] == ' ') {
			if (fen[i + 1] == 'w') color = 0;
			else color = 1;
			if (fen[i + 3] == '-') position.white_short_castling = false;
			if (fen[i + 4] == '-') position.white_long_castling = false;
			break;
		}
		if (fen[i] == 'r') {
			set_1(position.pieces[1][Piece::ROOK], to);
			x += 1;
		}
		else if (fen[i] == 'R') {
			set_1(position.pieces[0][Piece::ROOK], to);
			x += 1;
		}
		else if (fen[i] == 'n') {
			set_1(position.pieces[1][Piece::KNIGHT], to);
			x += 1;
		}
		else if (fen[i] == 'N') {
			set_1(position.pieces[0][Piece::KNIGHT], to);
			x += 1;
		}
		else if (fen[i] == 'b') {
			set_1(position.pieces[1][Piece::BISHOP], to);
			x += 1;
		}
		else if (fen[i] == 'B') {
			set_1(position.pieces[0][Piece::BISHOP], to);
			x += 1;
		}
		else if (fen[i] == 'q') {
			set_1(position.pieces[1][Piece::QUEEN], to);
			x += 1;
		}
		else if (fen[i] == 'Q') {
			set_1(position.pieces[0][Piece::QUEEN], to);
			x += 1;
		}
		else if (fen[i] == 'k') {
			set_1(position.pieces[1][Piece::KING], to);
			x += 1;
		}
		else if (fen[i] == 'K') {
			set_1(position.pieces[0][Piece::KING], to);
			x += 1;
		}
		else if (fen[i] == 'p') {
			set_1(position.pieces[1][Piece::PAWN], to);
			x += 1;
		}
		else if (fen[i] == 'P') {
			set_1(position.pieces[0][Piece::PAWN], to);
			x += 1;
		}
		else if (fen[i] == '/') {
			y -= 1;
			x = 0;
		}
		else {
			x += (fen[i] - '0');
		}
	}

	position.Update(Color::WHITE);
	position.Update(Color::BLACK);

	return position;
}

int main(int argc, char **argv) {
	Stockfish::Bitboards::init();
    Stockfish::Position::init();
	initialize_history();
	if (argc == 2 && std::string(argv[1]) == "ping-ping") {
		Stockfish::Position position;		
		std::string init_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
		Stockfish::StateListPtr states = Stockfish::StateListPtr(new std::deque<Stockfish::StateInfo>(1));
    	// position.set(init_fen, false, &states->back());
		int ply = 0;

		while (1) {
			std::string fen; std::string move; std::string str_time;
			std::getline(std::cin, fen);
			std::getline(std::cin, str_time);
			std::getline(std::cin, move);

			if (ply == 0) {
				position.set(fen, false, &states->back());
			}
			else if (move != "00") {
				auto m = to_move(position, move);

				if (m == Stockfish::Move::none()) {
					std::cerr << "beda" << '\n';
					break;
				}

				states->emplace_back();
				position.do_move(m, states->back());
				++ply;
			}

			// static_cash.clear();
			// for (int i = 0; i < 1001; ++i) killers[i] = {0, 0, 0, 0};

			int time = atoi(str_time.c_str());
			std::cerr << "TIME " << time << '\n';
			bool is_hard = true;

			time_t stop_time;
			if (time > 60) {
				TIME_LIMIT = 500000000;
			}
			else if (time > 25) {
				TIME_LIMIT = 300000000;
			}
			else if (time > 3) {
				TIME_LIMIT = 300000000;
				is_hard = false;
			}
			else {
				TIME_LIMIT = 60000000;
			}

			auto my_m = stockfish_iterative(position, stop_time, ply, is_hard=is_hard);

			std::cout << move_to_str(my_m) << std::endl;
			std::cout.flush();

			states->emplace_back();
			position.do_move(my_m, states->back());
			++ply;
		}
		return 0;
	}

	for (int i = 0; i < 1001; ++i) killers[i] = {0, 0, 0, 0};
	mark_tables[1] = {
		{0, 0, 0, 0, 0, 0, 0, 0, 0},
		{0, 1, 1, 1, 1, 1, 1, 1, 1},
		{0, 1, 1, 1, 1, 1, 1, 1, 1},
		{0, 1, 1, 1.25, 1.25, 1.25, 1.25, 1, 1},
		{0, 1, 1, 1.25, 1.5, 1.5, 1.25, 1, 1},
		{0, 1, 1, 1.25, 1.5, 1.5, 1.25, 1, 1},
		{0, 1, 1, 1.25, 1.25, 1.25, 1.25, 1, 1},
		{0, 1, 1, 1, 1, 1, 1, 1, 1},
		{0, 1, 1, 1, 1, 1, 1, 1, 1},
	};
	if (argc < 2) {
		stockfish_battle(stockfish_test, stockfish_test);
		//newBattle1(NEGABTIME_test, NEGABTIME_test);
		/*
        cout << thread::hardware_concurrency() << '\n';
		srand(time(nullptr));
		vector<vector<int>> field(9, vector<int>(9, 0));
		
		// newBattle1(NEG5AB, NEG5AB);*/
		return 0;
	}



	/*
	srand(time(nullptr));
	vector<vector<int>> field(9, vector<int>(9, 0));
	newBattle(LLIN3AB, LLIN1);
	field[1][1] = field[1][8] = 4;
	field[1][2] = field[1][7] = 2;
	field[1][3] = field[1][6] = 3;
	field[1][4] = 6; field[1][5] = 5;
	field[8][1] = field[8][8] = -4;
	field[8][2] = field[8][7] = -2;
	field[8][3] = field[8][6] = -3;
	field[8][4] = -6; field[8][5] = -5;
	for (int i = 1; i <= 8; i++) field[2][i] = 1;
	for (int i = 1; i <= 8; i++) field[7][i] = -1;*/
	// std::cout << argv[1] << '\n';

	std::string fen = std::string(argv[1]);

	vector<vector<int>> field(10, vector<int>(10, 0));
	int y = 8;
	int x = 0;
	int color = 3;

	Position position;

	for (int i = 0; i <= 1; ++i) {
		for (int j = 0; j < 6; ++j) position.pieces[i][j] = 0;
	}
	
	for (int i = 0; i < fen.size(); ++i) {
		int to = (y - 1) * 8 + x;
		if (fen[i] == ' ') {
			// std::cout << i << '\n';
			// std::cout << fen[i + 1] << '\n';
			// color = (fen[i + 1] == 'w' ? 1 : -1);
			if (fen[i + 1] == 'w') color = 0;
			else color = 1;
			if (fen[i + 3] == '-') position.white_short_castling = false;
			if (fen[i + 4] == '-') position.white_long_castling = false;
			break;
		}
		if (fen[i] == 'r') {
			set_1(position.pieces[1][Piece::ROOK], to);
			field[y][x] = -4; // rook
			x += 1;
		}
		else if (fen[i] == 'R') {
			set_1(position.pieces[0][Piece::ROOK], to);
			field[y][x] = 4;
			x += 1;
		}
		else if (fen[i] == 'n') {
			set_1(position.pieces[1][Piece::KNIGHT], to);
			field[y][x] = -2;
			x += 1;
		}
		else if (fen[i] == 'N') {
			set_1(position.pieces[0][Piece::KNIGHT], to);
			field[y][x] = 2;
			x += 1;
		}
		else if (fen[i] == 'b') {
			set_1(position.pieces[1][Piece::BISHOP], to);
			field[y][x] = -3;
			x += 1;
		}
		else if (fen[i] == 'B') {
			set_1(position.pieces[0][Piece::BISHOP], to);
			field[y][x] = 3;
			x += 1;
		}
		else if (fen[i] == 'q') {
			set_1(position.pieces[1][Piece::QUEEN], to);
			field[y][x] = -5;
			x += 1;
		}
		else if (fen[i] == 'Q') {
			set_1(position.pieces[0][Piece::QUEEN], to);
			field[y][x] = 5;
			x += 1;
		}
		else if (fen[i] == 'k') {
			set_1(position.pieces[1][Piece::KING], to);
			field[y][x] = -6;
			x += 1;
		}
		else if (fen[i] == 'K') {
			set_1(position.pieces[0][Piece::KING], to);
			field[y][x] = 6;
			x += 1;
		}
		else if (fen[i] == 'p') {
			set_1(position.pieces[1][Piece::PAWN], to);
			field[y][x] = -1;
			x += 1;
		}
		else if (fen[i] == 'P') {
			set_1(position.pieces[0][Piece::PAWN], to);
			field[y][x] = 1;
			x += 1;
		}
		else if (fen[i] == '/') {
			y -= 1;
			x = 0;
		}
		else {
			x += (fen[i] - '0');
		}
	}

	// std::cout << argc << '\n';
	// std::cout << argv[1] << '\n';
	//  color = (std::string(argv[2]) == "w" ? 1 : -1);
	// std::cout << color << '\n';
	/*
	for (int i = 1; i <= 8; ++i) {
		for (int j = 1; j <= 8; ++j) {
			std::cout << field[i][j] << ' ';
		}
		cout << '\n';
	}*/
	position.Update(Color::WHITE);
	position.Update(Color::BLACK);
	position.hash_.init(position);
	int time = atoi(argv[2]);

	Stockfish::StateListPtr states = Stockfish::StateListPtr(new std::deque<Stockfish::StateInfo>(1));
	Stockfish::Position pos;

	pos.set(fen, false, &states->back());

	std::string init_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
	Stockfish::Position init_pos;

	for (int i = 3; i < argc; ++i) {
		std::string move = std::string(argv[i]);
		Position cur = FromFen(string(argv[i]));
		cur.hash_.init(cur);
		history.push_back(cur.hash_);
	}

	states = Stockfish::StateListPtr(new std::deque<Stockfish::StateInfo>(1));
    init_pos.set(init_fen, false, &states->back());
	int ply = 0;

    for (int i = 3; i < argc; ++i) {
		std::string move = std::string(argv[i]);
        auto m = to_move(init_pos, move);

        if (m == Stockfish::Move::none()) {
			std::cerr << "beda" << '\n';
            break;
		}

        states->emplace_back();
        init_pos.do_move(m, states->back());
		++ply;
    }
	// Move move = {0, 0, 0, 0};

	time_t stop_time;
	if (time > 60) {
		stop_time = 5000000000;
	}
	else if (time > 25) {
		stop_time = 3000000000;
	}
	else if (time > 5) {
		stop_time = 2000000000;
	}
	else {
		stop_time = 100000000;
	}

	// move = NEGABTIME(color, position, stop_time);
	Stockfish::Move move = stockfish_iterative(init_pos, stop_time, ply);

	/*
	if (time > 60) {
		move = NEG5AB(color, position);
	}
	else if (time > 15) {
		move = NEG5AB(color, position);
	}
	else if (time > 5) {
		move = NEG4AB(color, position);
	}
	else {
		move = NEG4AB(color, position);
	}
	//  newmove move = LLIN5AB(color, field);*/
	// std::cout << from_code(move);

	std::cout << move_to_str(move);

	return 0;
}

