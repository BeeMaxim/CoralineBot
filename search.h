#include "position_.h"
#include "movegen.h"
#include "types.h"
#include "position_.h"
#include "misc.h"
#include "evaluate.h"
#include "movepicker.h"
#include "tt.h"

#include <iostream>
#include <string>
#include <future>
#include <chrono>
#include <bit>


extern atomic<bool> STOP;
extern TranspositionTable tt;


const std::string move_to_str(Stockfish::Move m, bool chess960 = false) {
    if (m == Stockfish::Move::none())
        return "(none)";

    if (m == Stockfish::Move::null())
        return "0000";

    Stockfish::Square from = m.from_sq();
    Stockfish::Square to = m.to_sq();

    if (m.type_of() == Stockfish::CASTLING && !chess960)
        to = make_square(to > from ? Stockfish::FILE_G : Stockfish::FILE_C, rank_of(from));

    std::string move = std::string{char('a' + file_of(from)), char('1' + rank_of(from))}
                        + std::string{char('a' + file_of(to)), char('1' + rank_of(to))};

    if (m.type_of() == Stockfish::PROMOTION)
        move += " pnbrqk"[m.promotion_type()];
    return move;
  
}


std::string to_lower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](auto c) { return std::tolower(c); });
    return str;
}


Stockfish::Move to_move(const Stockfish::Position& pos, std::string str) {
    // str = to_lower(str);

    for (const auto& m : Stockfish::MoveList<Stockfish::LEGAL>(pos)) {
        if (str == move_to_str(m, false))
            return m;
    }

    return Stockfish::Move::none();
}


int marker(Stockfish::Position& position) {
    // 535
    // int material = 300 * position.count<Stockfish::PAWN>() + position.non_pawn_material();
    auto color = position.side_to_move();
    int material = 300 * (position.count<Stockfish::PAWN>(color) - position.count<Stockfish::PAWN>(Stockfish::Color(1 - color)));
    material += position.non_pawn_material(color) - position.non_pawn_material(Stockfish::Color(1 - color));

    int mobility = evaluate_mobility<Stockfish::WHITE>(position) - evaluate_mobility<Stockfish::BLACK>(position);
    
    if (position.side_to_move() == Stockfish::BLACK) mobility *= -1;

    int score = material + mobility;

    return score;
}

int captures_search(Stockfish::Position& position, int alpha, int beta) {
	if (STOP) return 0;

	int score = marker(position);
	alpha = max(alpha, score);

    Stockfish::MovePicker mp(position, Stockfish::Move::none(), 0);
    Stockfish::StateInfo new_st;

    Stockfish::Move move;

	while ((move = mp.next_move()) != Stockfish::Move::none()) {
        if (!position.legal(move)) {
            continue;
        }
		position.do_move(move, new_st);

		score = -captures_search(position, -beta, -alpha);

		position.undo_move(move);

		alpha = max(alpha, score);

		if (alpha >= beta) break;
	}

	return alpha;
}


template<typename T>
T search(Stockfish::Position& position, int deep, int alpha, int beta, int ply) {
    if (STOP) {
        if constexpr (std::is_integral_v<T>) return 0;
        else return Stockfish::Move();
    }

    if constexpr (std::is_integral_v<T>) {
        if (position.is_draw(ply)) {
            return 0;
        }
        if (deep <= 0) {
            return captures_search(position, alpha, beta);
            return marker(position);
        }
    }

    Stockfish::Move tt_move = Stockfish::Move::none();
    auto tt_move_ptr = tt.probe(position.key());
    if (tt_move_ptr != nullptr) {
        tt_move = tt_move_ptr->move;
        if constexpr (std::is_integral_v<T>) {
            int value = tt_move_ptr->value;
            if (tt_move_ptr->depth >= deep &&
                (tt_move_ptr->bound == BOUND_EXACT ||
                (tt_move_ptr->bound == BOUND_LOWER && value >= beta) ||
                (tt_move_ptr->bound == BOUND_UPPER && value <= alpha))) {
                    return value;
            }
        }
    }

    Stockfish::MovePicker mp(position, tt_move, deep);
    Stockfish::StateInfo new_st;

    int score = -1e9 - deep;
    Stockfish::Move move, final_move = Stockfish::Move::none();

    while ((move = mp.next_move()) != Stockfish::Move::none()) {

        if (!position.legal(move)) {
            continue;
        }
        
        position.do_move(move, new_st);

        int cur_score = -search<int>(position, deep - 1, -beta, -alpha, ply + 1);

        position.undo_move(move);

        if (cur_score > score) {
            score = cur_score;
            final_move = move;

            if (score > alpha) {
                alpha = score;
            }
            if (alpha >= beta) {
                break;
            }
        }
    }
    if (final_move != Stockfish::Move::none()) {
        Bound bound = (score >= beta) ? BOUND_LOWER :
              (score <= alpha) ? BOUND_UPPER : BOUND_EXACT;
        tt.save(position.key(), score, bound, deep, final_move);
    }

    if constexpr (std::is_integral_v<T>) {
        if (final_move == Stockfish::Move::none()) {
            if (position.checkers()) return score;
            return 0;
        }
        return score;
    } 
    else {
        std::cerr << "SCORE: " << score << '\n';
        return final_move;
    }
};


Stockfish::Move stockfish_test(Stockfish::Position& position) {
	STOP = false;
	time_t start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	Stockfish::Move move;

	for (int deep = 1; deep < 7; ++deep) {
		cout << "------------------------DEEP: " << deep << "-----------------------------\n";
		int alpha = -1e9 - 1000, beta = 1e9 + 1000;

		std::future<Stockfish::Move> thread = std::async(search<Stockfish::Move>, std::ref(position), deep, alpha, beta, 0);
		bool still_search = true;

		if (still_search) {
			move = thread.get();
		}
		else {
			STOP = true;
			thread.get();
			break;
		}

	}
	STOP = false;

	return move;
}


Stockfish::Move stockfish_iterative(Stockfish::Position& position, time_t stop_time, int ply) {
	STOP = false;
	time_t start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	Stockfish::Move move;
	
	for (int32_t deep = 1; deep < 1000; ++deep) {
		int alpha = -100000, beta = 100000;
		std::future<Stockfish::Move> thread = std::async(search<Stockfish::Move>, std::ref(position), deep, alpha, beta, ply);

		bool search = true;
		while (thread.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {
			if ((std::chrono::high_resolution_clock::now().time_since_epoch().count() - start) >= stop_time) {
				search = false;
				break;
			}
		}

		if (search) {
			move = thread.get();
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


void stockfish_battle(Stockfish::Move bot1(Stockfish::Position&), Stockfish::Move bot2(Stockfish::Position&)) {
	Stockfish::StateListPtr states = Stockfish::StateListPtr(new std::deque<Stockfish::StateInfo>(1));
	Stockfish::Position position;
	std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

	position.set(fen, false, &states->back());

	cout << "START!!!" << endl;

	int cnt = 0;
	while (1) {
		int start = clock();
		Stockfish::Move m = bot1(position);
		
        ++cnt;

        states->emplace_back();

		position.do_move(m, states->back());
		cout << "White: " << move_to_str(m) << endl << endl;
		cout << clock() - start << endl; start = clock();

		m = bot2(position);

        ++cnt;

        states->emplace_back();

		position.do_move(m, states->back());
		cout << "Black: " << move_to_str(m) << endl << endl;
		cout << clock() - start << endl;
	}
	cout << "Moves : " << cnt;
    tt.clear();
}


void test() {
    Stockfish::Position pos;
    // std::cout << search<int>() << ' '<< search<std::string>() << '\n';

    std::string fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"; // Starting position
    Stockfish::StateInfo si;
    pos.set(fen, false, &si);

    Stockfish::MoveList<Stockfish::GenType::LEGAL> moves(pos);
    std::cout << "Legal moves:" << std::endl;
    for (const Stockfish::Move& move : moves) {
        // std::cout << "!!!\n";
        std::cout << move_to_str(move) << std::endl;
    }

    Stockfish::StateInfo newst;
    pos.do_move(*moves.begin(), newst);
    Stockfish::MoveList<Stockfish::GenType::LEGAL> new_moves(pos);
    std::cout << "New Legal moves:" << std::endl;
    for (const Stockfish::Move& move : new_moves) {
        // std::cout << "!!!\n";
        std::cout << move_to_str(move) << std::endl;
    }
}
