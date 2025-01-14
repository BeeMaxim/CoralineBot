#include <iostream>
#include <algorithm>
#include <vector>

using namespace std;


struct newmove {
	int oldX, oldY, newX, newY, type;
	newmove(int oldx, int oldy, int newx, int newy) {
		oldX = oldx; oldY = oldy; newX = newx; newY = newy; type = 0;
	}
	newmove(int oldx, int oldy, int newx, int newy, int TYPE) {
		oldX = oldx; oldY = oldy; newX = newx; newY = newy; type = TYPE;
	}
};

bool ok(int x, int y, int color, vector<vector<int>>& field) {
	if (x < 1 || x > 8 || y < 1 || y > 8) return 0;
	return (field[x][y] * color <= 0);
}

bool ok_b(int x, int y, int color, vector<vector<int>>& field) {
	if (x < 1 || x > 8 || y < 1 || y > 8) return 0;
	return (field[x][y] * color < 0);
}

vector<newmove> pawnMove(vector<vector<int>>& field, int x, int y) { //vzyatie na prohode!!!
	int dir = (field[x][y] > 0 ? 1 : -1);
	vector<newmove> res;
	for (int i = -1; i <= 1; i += 2) {
		if (ok_b(x + dir, y + i, dir, field)) {
			if (x + dir == 8 || x + dir == 1) {
				for (int type = 2; type < 6; type++) {
					res.push_back({ x, y, x + dir, y + i, type });
				}
			}
			else res.push_back({ x, y, x + dir, y + i });
		}
	}
	if (x + dir == 8 || x + dir == 1) {
		if (field[x + dir][y] == 0) {
			for (int type = 2; type < 6; type++) {
				res.push_back({ x, y, x + dir, y, type });
			}
		}
	}
	else {
		if (field[x + dir][y] == 0) res.push_back({ x, y, x + dir, y });
		if (dir == 1 && x == 2 && field[x + 1][y] == 0 && field[x + 2][y] == 0) {
			res.push_back({ x, y, x + 2, y });
		}
		if (dir == -1 && x == 7 && field[x - 1][y] == 0 && field[x - 2][y] == 0) {
			res.push_back({ x, y, x - 2, y });
		}
	}
	return res;
}

vector<newmove> knightMove(vector<vector<int>>& field, int x, int y) {
	vector<newmove> res; int color = (field[x][y] > 0 ? 1 : -1);
	for (int i = -2; i <= 2; i++) {
		for (int j = -2; j <= 2; j++) {
			if (i == 0 || j == 0 || abs(i) == abs(j)) continue;
			if (ok(x + i, y + j, field[x][y], field)) res.push_back({ x, y, x + i, y + j });
		}
	}
	return res;
}

vector<newmove> bishopMove(vector<vector<int>>& field, int x, int y) {
	vector<newmove> res; int color = (field[x][y] > 0 ? 1 : -1);
	for (int i = -1; i <= 1; i += 2) {
		for (int j = -1; j <= 1; j += 2) {
			int nowx = x, nowy = y;
			while (1) {
				nowx += i; nowy += j;
				if (!ok(nowx, nowy, color, field)) break;
				if (field[nowx][nowy] * color < 0) {
					res.push_back({ x, y, nowx, nowy }); break;
				}
				res.push_back({ x, y, nowx, nowy });
			}
		}
	}
	return res;
}

vector<newmove> rookMove(vector<vector<int>>& field, int x, int y) {
	vector<newmove> res; int color = (field[x][y] > 0 ? 1 : -1);
	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			if (((i == 0) ^ (j == 0)) == 0) continue;
			int nowx = x, nowy = y;
			while (1) {
				nowx += i; nowy += j;
				if (!ok(nowx, nowy, color, field)) break;
				if (field[nowx][nowy] * color < 0) {
					res.push_back({ x, y, nowx, nowy }); break;
				}
				res.push_back({ x, y, nowx, nowy });
			}
		}
	}
	return res;
}

vector<newmove> queenMove(vector<vector<int>>& field, int x, int y) {
	vector<newmove> res = bishopMove(field, x, y);
	for (auto &i : rookMove(field, x, y)) res.push_back(i);
	return res;
}

vector<newmove> kingMove(vector<vector<int>>& field, int x, int y) {
	vector<newmove> res; int color = (field[x][y] > 0 ? 1 : -1);
	for (int i = -1; i <= 1; i++) {
		for (int j = -1; j <= 1; j++) {
			if (i == 0 && j == 0) continue;
			if (ok(x + i, y + j, color, field)) res.push_back({ x, y, x + i, y + j });
		}
	}
	return res;
}


vector<newmove> getallmoves(vector<vector<int>>& field, int color) {
	vector<newmove> res;
	for (int i = 1; i <= 8; i++) {
		for (int j = 1; j <= 8; j++) {
			if (field[i][j] == 0) continue;
			
			else if (field[i][j] == 1 * color) for (auto &u : pawnMove(field, i, j)) res.push_back(u);
			else if (field[i][j] == 2 * color) for (auto &u : knightMove(field, i, j)) res.push_back(u);
			else if (field[i][j] == 3 * color) for (auto &u : bishopMove(field, i, j)) res.push_back(u);
			else if (field[i][j] == 4 * color) for (auto &u : rookMove(field, i, j)) res.push_back(u);
			else if (field[i][j] == 5 * color) for (auto &u : queenMove(field, i, j)) res.push_back(u);
			else if (field[i][j] == 6 * color) for (auto &u : kingMove(field, i, j)) res.push_back(u);
			/*else if (field[i][j] == 1 * color) res.emplace_back(pawnMove(field, i, j));
			else if (field[i][j] == 2 * color) res.emplace_back(knightMove(field, i, j));
			else if (field[i][j] == 3 * color) res.emplace_back(bishopMove(field, i, j));
			else if (field[i][j] == 4 * color) res.emplace_back(rookMove(field, i, j));
			else if (field[i][j] == 5 * color) res.emplace_back(queenMove(field, i, j));
			else if (field[i][j] == 6 * color) res.emplace_back(kingMove(field, i, j));*/
		}
	}
	return res;
}

bool newcheck(vector<vector<int>>& field, int color) {
	int x = 0, y = 0;
	for (int i = 1; i <= 8; i++) {
		for (int j = 1; j <= 8; j++) {
			if (field[i][j] == 6 * color) {
				x = i; y = j;
			}
		}
	}
	vector<newmove> MOVES = getallmoves(field, color * -1);
	for (auto &i : MOVES) {
		if (i.newX == x && i.newY == y) return 1;
	}
	return 0;
}


