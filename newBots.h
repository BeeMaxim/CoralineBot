#include <iostream>
#include <vector>
#include <random>
#include <ctime>
#include "newRules1.h"

using namespace std;

std::vector<vector<double>> mark_tables[10];
int marks[7] = { 0, 100, 300, 320, 500, 900, 1000 };

int marker(vector<vector<int>>& field) {
	int res = 0;
	for (int i = 1; i <= 8; i++) {
		for (int j = 1; j <= 8; j++) {
			if (field[i][j] == 0) continue;
			int sign = 1;
			if (field[i][j] < 0) sign = -1;
			
			if (abs(field[i][j]) == 1 || abs(field[i][j] == 2)) {
				// cout << mark_tables[1][i][j] << '\n';
				res += mark_tables[1][i][j] * marks[abs(field[i][j])] * sign;
			}
			else res += marks[abs(field[i][j])] * sign;
		}
	}
	return res;
}


pair<newmove, int> LIN(int deep, int color, vector<vector<int>>& field) {
	if (deep == 0) {
		return { { 0, 0, 0, 0 }, marker(field) };
	}
	vector<newmove> res = getallmoves(field, color);
	newmove chMove = { 0, 0, 0, 0 }; int cost = -10000 * color;
	vector<newmove> to_ch(1, chMove);
	for (auto &i : res) {
		int OLD = field[i.oldX][i.oldY], NEW = field[i.newX][i.newY];
		if (i.type != 0) field[i.newX][i.newY] = i.type;
		else field[i.newX][i.newY] = field[i.oldX][i.oldY];
		field[i.oldX][i.oldY] = 0;
		if (!newcheck(field, color)) {
			auto p = LIN(deep - 1, -color, field);
			if (color == 1 && p.second > cost) {
				cost = p.second; to_ch.clear(); to_ch.push_back(i);
			}
			else if (color == 1 && p.second == cost) to_ch.push_back(i);
			if (color == -1 && p.second < cost) {
				cost = p.second; to_ch.clear(); to_ch.push_back(i);
			}
			else if (color == -1 && p.second == cost) to_ch.push_back(i);
		}
		field[i.oldX][i.oldY] = OLD;
		field[i.newX][i.newY] = NEW;
	}
	// random_shuffle(to_ch.begin(), to_ch.end());
	return { to_ch[0], cost };
}

newmove LLIN1(int color, vector<vector<int>>& field) {
	return LIN(1, color, field).first;
}

newmove LLIN2(int color, vector<vector<int>>& field) {
	return LIN(2, color, field).first;
}

newmove LLIN3(int color, vector<vector<int>>& field) {
	return LIN(3, color, field).first;
}

newmove LLIN4(int color, vector<vector<int>>& field) {
	return LIN(4, color, field).first;
}

pair<newmove, int> LINAB(int deep, int color, vector<vector<int>>& field, int alpha, int beta) {
	if (deep == 0) {
		return { {0, 0, 0, 0 }, marker(field) };
	}
	vector<newmove> res = getallmoves(field, color);
	newmove chMove = { 0, 0, 0, 0 }; int cost = -10000 * color;
	vector<newmove> to_ch(1, chMove);
	
	/*int cur_cost = -10000 * color, best = 0;
	for (int u = 0; u < res.size(); ++u) {
		newmove i = res[u];
		int OLD = field[i.oldX][i.oldY], NEW = field[i.newX][i.newY];
		if (i.type != 0) field[i.newX][i.newY] = i.type;
		else field[i.newX][i.newY] = field[i.oldX][i.oldY];
		field[i.oldX][i.oldY] = 0;
		if (!newcheck(field, color)) {
			if (marker(field) * color > cur_cost) {
				cur_cost = marker(field) * color;
				best = u;
			}
		}
		field[i.oldX][i.oldY] = OLD;
		field[i.newX][i.newY] = NEW;
	}
	swap(res[0], res[best]);*/
	auto key = [&field, &color](newmove& a, newmove& b) -> bool {
		if (field[a.newX][a.newY] == 0 && field[b.newX][b.newY] == 0) {
			if (field[a.newX + color][a.newY - 1] == -color || field[a.newX + color][a.newY + 1] == -color) {
				return 0;
			}
			return 1;
		}
		if (field[a.newX][a.newY] == 0) return 0;
		if (field[b.newX][b.newY] == 0) return 1;
		if (abs(field[a.newX][a.newY]) == abs(field[b.newX][b.newY])) {
			return (abs(field[a.oldX][a.oldY]) < abs(field[b.oldX][b.oldY]));
		}
		return (abs(field[a.newX][a.newY]) > abs(field[b.newX][b.newY]));
	};
	sort(res.begin(), res.end(), key);
	for (auto &i : res) {
		int OLD = field[i.oldX][i.oldY], NEW = field[i.newX][i.newY];
		if (i.type != 0) field[i.newX][i.newY] = i.type * color;
		else field[i.newX][i.newY] = field[i.oldX][i.oldY];
		field[i.oldX][i.oldY] = 0;
		if (!newcheck(field, color)) {
			auto p = LINAB(deep - 1, -color, field, alpha, beta);
			if (color == 1 && p.second > beta) {
				field[i.oldX][i.oldY] = OLD;
				field[i.newX][i.newY] = NEW;
				return { i, p.second };
			}
			else if (color == 1) alpha = max(alpha, p.second);
			if (color == -1 && p.second < alpha) {
				field[i.oldX][i.oldY] = OLD;
				field[i.newX][i.newY] = NEW;
				return { i, p.second };
			}
			else if (color == -1) beta = min(beta, p.second);
			if (color == 1 && p.second > cost) {
				cost = p.second; to_ch.clear(); to_ch.push_back(i);
			}
			else if (color == 1 && p.second == cost) to_ch.push_back(i);
			if (color == -1 && p.second < cost) {
				cost = p.second; to_ch.clear(); to_ch.push_back(i);
			}
			else if (color == -1 && p.second == cost) to_ch.push_back(i);
		}
		field[i.oldX][i.oldY] = OLD;
		field[i.newX][i.newY] = NEW;
	}
	// random_shuffle(to_ch.begin(), to_ch.end());
	return { to_ch[0], cost };
}

newmove LLIN1AB(int color, vector<vector<int>>& field) {
	int alpha = -1000, beta = 1000;
	return LINAB(1, color, field, alpha, beta).first;
}

newmove LLIN2AB(int color, vector<vector<int>>& field) {
	int alpha = -1000, beta = 1000;
	return LINAB(2, color, field, alpha, beta).first;
}

newmove LLIN4AB(int color, vector<vector<int>>& field) {
	int alpha = -10000, beta = 10000;
	return LINAB(4, color, field, alpha, beta).first;
}

newmove LLIN3AB(int color, vector<vector<int>>& field) {
	int alpha = -10000, beta = 10000;
	return LINAB(3, color, field, alpha, beta).first;
}

newmove LLIN5AB(int color, vector<vector<int>>& field) {
	int alpha = -10000, beta = 10000;
	return LINAB(5, color, field, alpha, beta).first;
}

newmove LLIN6AB(int color, vector<vector<int>>& field) {
	int alpha = -10000, beta = 10000;
	return LINAB(6, color, field, alpha, beta).first;
}

newmove LLIN7AB(int color, vector<vector<int>>& field) {
	int alpha = -10000, beta = 10000;
	return LINAB(7, color, field, alpha, beta).first;
}

pair<newmove, int> LINAB_2(int deep, int color, vector<vector<int>>& field, int alpha, int beta, int mark) {
	if (deep == 0) {
		return { {0, 0, 0, 0 }, mark };
	}
	vector<newmove> res = getallmoves(field, color);
	newmove chMove = { 0, 0, 0, 0 }; int cost = -100 * color;
	vector<newmove> to_ch(1, chMove);
	for (auto i : res) {
		int OLD = field[i.oldX][i.oldY], NEW = field[i.newX][i.newY];
		int newMark = mark - field[i.newX][i.newY];
		if (i.type != 0) {
			field[i.newX][i.newY] = i.type;
		}
		else field[i.newX][i.newY] = field[i.oldX][i.oldY];
		newMark += field[i.newX][i.newY] - field[i.oldX][i.oldY];
		field[i.oldX][i.oldY] = 0;
		if (!newcheck(field, color)) {
			auto p = LINAB_2(deep - 1, -color, field, alpha, beta, newMark);
			if (color == 1 && p.second > beta) {
				field[i.oldX][i.oldY] = OLD;
				field[i.newX][i.newY] = NEW;
				return { i, p.second };
			}
			else if (color == 1) alpha = max(alpha, p.second);
			if (color == -1 && p.second < alpha) {
				field[i.oldX][i.oldY] = OLD;
				field[i.newX][i.newY] = NEW;
				return { i, p.second };
			}
			else if (color == -1) beta = min(beta, p.second);
			if (color == 1 && p.second > cost) {
				cost = p.second; to_ch.clear(); to_ch.push_back(i);
			}
			else if (color == 1 && p.second == cost) to_ch.push_back(i);
			if (color == -1 && p.second < cost) {
				cost = p.second; to_ch.clear(); to_ch.push_back(i);
			}
			else if (color == -1 && p.second == cost) to_ch.push_back(i);
		}
		field[i.oldX][i.oldY] = OLD;
		field[i.newX][i.newY] = NEW;
	}
	// random_shuffle(to_ch.begin(), to_ch.end());
	return { to_ch[0], cost };
}

newmove LLIN4AB_2(int color, vector<vector<int>>& field) {
	int alpha = -1000, beta = 1000;
	return LINAB_2(4, color, field, alpha, beta, marker(field)).first;
}

pair<newmove, int> LINAB_3(int deep, int color, vector<vector<int>>& field, int alpha, int beta, int mark) {
	if (deep == 0) {
		return { {0, 0, 0, 0 }, mark };
	}
	vector<newmove> res = getallmoves(field, color);
	// random_shuffle(res.begin(), res.end());
	newmove chMove = { 0, 0, 0, 0 }; int cost = -100 * color;
	for (auto i : res) {
		int OLD = field[i.oldX][i.oldY], NEW = field[i.newX][i.newY];
		int newMark = mark - field[i.newX][i.newY];
		if (i.type != 0) {
			field[i.newX][i.newY] = i.type;
		}
		else field[i.newX][i.newY] = field[i.oldX][i.oldY];
		newMark += field[i.newX][i.newY] - field[i.oldX][i.oldY];
		field[i.oldX][i.oldY] = 0;
		if (!newcheck(field, color)) {
			auto p = LINAB_3(deep - 1, -color, field, alpha, beta, newMark);
			if (color == 1 && p.second > beta) {
				field[i.oldX][i.oldY] = OLD;
				field[i.newX][i.newY] = NEW;
				return { i, p.second };
			}
			else if (color == 1) alpha = max(alpha, p.second);
			if (color == -1 && p.second < alpha) {
				field[i.oldX][i.oldY] = OLD;
				field[i.newX][i.newY] = NEW;
				return { i, p.second };
			}
			else if (color == -1) beta = min(beta, p.second);
			if (color == 1 && p.second > cost) {
				cost = p.second; chMove = i;
			}
			if (color == -1 && p.second < cost) {
				cost = p.second; chMove = i;
			}
		}
		field[i.oldX][i.oldY] = OLD;
		field[i.newX][i.newY] = NEW;
	}
	return { chMove, cost };
}

newmove LLIN4AB_3(int color, vector<vector<int>>& field) {
	int alpha = -1000, beta = 1000;
	return LINAB_2(4, color, field, alpha, beta, marker(field)).first;
}
