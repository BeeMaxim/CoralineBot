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

int COUNT = 0;

namespace Utility {
/// Clamp a value between lo and hi. Available in c++17.
    template<class T> constexpr const T& clamp(const T& v, const T& lo, const T& hi) {
        return v < lo ? lo : v > hi ? hi : v;
    }
}

const int COLOR_NB = 2; // Number of sides
const int SQUARE_NB = 64; // Number of squares
const int PIECE_NB = 6; // Number of piece types
const int CONTINUATION_HISTORY_SIZE = 6; // Number of continuation history levels

// Main history table
int mainHistory[COLOR_NB][SQUARE_NB * SQUARE_NB];

// Continuation history tables
int continuationHistory[300][PIECE_NB][SQUARE_NB];

// Capture history table
int captureHistory[PIECE_NB][SQUARE_NB][PIECE_NB];

Stockfish::Move killer_moves[300][2];


void update_continuation_histories(Stockfish::Piece pc, Stockfish::Square to, int bonus, int ply) {
    static constexpr std::array<std::pair<int, int>, 6> conthist_bonuses = {
      {{1, 1025}, {2, 621}, {3, 325}, {4, 512}, {5, 122}, {6, 534}}};

    continuationHistory[0][pc][to] += bonus;

    for (const auto [i, weight] : conthist_bonuses)
    {
        // Only update the first 2 continuation histories if we are in check
        //if (ss->inCheck && i > 2)
         //   break;
        //if (((ss - i)->currentMove).is_ok())
        if (ply - i >= 0) {
            //continuationHistory[ply - i][pc][to] -= bonus * weight / 1024;
        }
    }
}

// Function to update main and continuation history
void update_stats(const Stockfish::Position& pos, Stockfish::Move move, int bonus, int ply) {
    if (!pos.capture(move)) {
        int side = pos.side_to_move();
        int from = move.from_sq();
        Stockfish::Square to = move.to_sq();
        int index = from * SQUARE_NB + to;
        mainHistory[side][index] += bonus;

        Stockfish::Piece pc = pos.moved_piece(move);

        update_continuation_histories(pc, to, bonus, ply);
        /*
        Stockfish::Piece pc = pos.moved_piece(move);
        for (int i = 0; i < CONTINUATION_HISTORY_SIZE; ++i) {
            continuationHistory[i][pc][to] += bonus;
        }*/
    }
}




// Function to update capture history
void update_capture_history(const Stockfish::Position& pos, Stockfish::Move move, int bonus) {
    if (pos.capture(move)) {
        Stockfish::Piece pc = pos.moved_piece(move);
        Stockfish::Square to = move.to_sq();
        Stockfish::PieceType captured = type_of(pos.piece_on(to));
        captureHistory[pc][to][captured] += bonus;
    }
}

// Function to calculate bonus based on depth
int stat_bonus(int depth) {
    // Example bonus calculation based on depth
    return depth * depth;
}

// Initialize history tables
void initialize_history() {
    //memset(mainHistory, 0, sizeof(mainHistory));
    for (int i = 0; i < COLOR_NB; ++i) {
        for (int j = 0; j < SQUARE_NB * SQUARE_NB; ++j) {
            mainHistory[i][j] = 0;
        }
    }
    //memset(continuationHistory, 0, sizeof(continuationHistory));
    //memset(captureHistory, 0, sizeof(captureHistory));
}


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
    ++COUNT;
    /*
    Stockfish::Value npm_w = position.non_pawn_material(Stockfish::WHITE);
    Stockfish::Value npm_b = position.non_pawn_material(Stockfish::BLACK);
    Stockfish::Value npm   = Utility::clamp(npm_w + npm_b, Stockfish::Value::EndgameLimit, Stockfish::Value::MidgameLimit);

  // Map total non-pawn material into [PHASE_ENDGAME, PHASE_MIDGAME]
    Stockfish::Phase gamePhase = Stockfish::Phase(((npm - Stockfish::Value::EndgameLimit) * Stockfish::Phase::PHASE_MIDGAME) / 
                (Stockfish::Value::MidgameLimit - Stockfish::Value::EndgameLimit));*/
    // 535
    // int material = 300 * position.count<Stockfish::PAWN>() + position.non_pawn_material();
    auto color = position.side_to_move(); 
    int material = 300 * (position.count<Stockfish::PAWN>(color) - position.count<Stockfish::PAWN>(Stockfish::Color(1 - color)));
    // material = npm_w - npm_b;
    // std::cerr << npm_w << ' '<< npm_b << '\n';
    material += position.non_pawn_material(color) - position.non_pawn_material(Stockfish::Color(1 - color));

    int mobility = evaluate_mobility<Stockfish::WHITE>(position) - evaluate_mobility<Stockfish::BLACK>(position);

    Stockfish::Score score = Stockfish::SCORE_ZERO;
    /*
    score +=  pieces<Stockfish::WHITE, Stockfish::KNIGHT>(position) - pieces<Stockfish::BLACK, Stockfish::KNIGHT>(position)
            + pieces<Stockfish::WHITE, Stockfish::BISHOP>(position) - pieces<Stockfish::BLACK, Stockfish::BISHOP>(position)
            + pieces<Stockfish::WHITE, Stockfish::ROOK  >(position) - pieces<Stockfish::BLACK, Stockfish::ROOK  >(position)
            + pieces<Stockfish::WHITE, Stockfish::QUEEN >(position) - pieces<Stockfish::BLACK, Stockfish::QUEEN >(position);*/

    // Stockfish::ScaleFactor sf = scale_factor(Stockfish::eg_value(score));
    /*
    Stockfish::ScaleFactor sf = Stockfish::ScaleFactor::SCALE_FACTOR_NORMAL;
    Stockfish::Value v = Stockfish::mg_value(score) ; // * int(gamePhase)
       + eg_value(score) * int(Stockfish::Phase::PHASE_MIDGAME - gamePhase) * sf / Stockfish::ScaleFactor::SCALE_FACTOR_NORMAL;*/

    // v /= Stockfish::Phase::PHASE_MIDGAME;
    // v = Stockfish::Value((int)v / (int)Stockfish::Phase::PHASE_MIDGAME);

    // std::cerr << mobility << '\n';

    // v = mg_value(score);
    
    if (position.side_to_move() == Stockfish::BLACK) mobility *= -1;

    // int score = material + mobility;

    // if (position.side_to_move() == Stockfish::BLACK) v = -v;

    return material + mobility;
}

int captures_search(Stockfish::Position& position, int alpha, int beta) {
	if (STOP) return 0;
	int score = marker(position);
	alpha = max(alpha, score);

    Stockfish::MovePicker mp(position, Stockfish::Move::none(), 0, mainHistory, continuationHistory[0], captureHistory);
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
T search(Stockfish::Position& position, int deep, int alpha, int beta, int ply, int global_ply, bool is_null_move=false) {
    if (STOP) {
        if constexpr (std::is_integral_v<T>) return 0;
        else return T();
    }

    if constexpr (std::is_integral_v<T>) {
        if (position.is_draw(global_ply)) {
            return 0;
        }
        if (deep <= 0) {
            return captures_search(position, alpha, beta);
            // return marker(position);
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
    /*
    if (!is_null_move && ply >= 4 && deep > 1 && !position.checkers()) {
        int R = 3 + deep / 6; // Reduction factor (adaptive based on depth)
        
        Stockfish::StateInfo null_st;

        position.do_null_move(null_st);

        int null_score = -search<int>(position, deep - 1 - R, -beta, -beta + 1, ply + 1, global_ply + 1, true);

        position.undo_null_move();

        if (null_score >= beta) {
            if constexpr (std::is_integral_v<T>) {
                return null_score; // Beta cutoff
            }
        }
    }*/

    Stockfish::MovePicker mp(position, tt_move, deep, mainHistory, continuationHistory[0], captureHistory);
    Stockfish::StateInfo killer;
    Stockfish::StateInfo new_st;

    int score = -1e9 - deep;
    Stockfish::Move move, final_move = Stockfish::Move::none();
    /*
    if constexpr (std::is_integral_v<T>) {
        for (int i = 0; i < 2; ++i) {
            Stockfish::Move killer_move = killer_moves[ply][i];
            if (killer_move != Stockfish::Move::none() && position.legal(killer_move)) {
                position.do_move(killer_move, killer);
                int cur = -search<int>(position, deep - 1, -beta, -alpha, ply + 1);
                position.undo_move(killer_move);

                if (cur > score) {
                    score = cur;
                    alpha = score;
                    if (alpha >= beta) {
                        killer_moves[ply][i] = killer_move;
                        return score;
                    }
                }
            }
        }
    }*/

    move = Stockfish::Move::none();

    while ((move = mp.next_move()) != Stockfish::Move::none()) {

        if (!position.legal(move)) {
            continue;
        }

        position.do_move(move, new_st);

        int cur_score = -search<int>(position, deep - 1, -beta, -alpha, ply + 1, global_ply + 1, is_null_move);

        position.undo_move(move);

        if (cur_score > score) {
            score = cur_score;
            final_move = move;

            if (score > alpha) {
                alpha = score;
                if (!position.capture(move)) {
                    update_stats(position, move, stat_bonus(deep), global_ply);
                }
            }
            if (alpha >= beta) {
                killer_moves[ply][1] = killer_moves[ply][0];
                killer_moves[ply][0] = move;
                if (!position.capture(move)) {
                    update_stats(position, move, stat_bonus(deep), global_ply);
                } else {
                    // update_capture_history(position, move, stat_bonus(deep));
                }
                break;
            }
        }
    }
    if (final_move != Stockfish::Move::none() && (tt_move_ptr == nullptr || tt_move_ptr->depth < deep)) {
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
    for (int i = 0; i < 100; ++i) killer_moves[i][0] = killer_moves[i][1] = Stockfish::Move::none();
    COUNT = 0;
	STOP = false;
	time_t start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    initialize_history();
	Stockfish::Move move;

	for (int deep = 1; deep < 8; ++deep) {
		cout << "------------------------DEEP: " << deep << "-----------------------------\n";
		int alpha = -1e9 - 1000, beta = 1e9 + 1000;

		std::future<Stockfish::Move> thread = std::async(search<Stockfish::Move>, std::ref(position), deep, alpha, beta, 0, 0, false);
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
    std::cout << COUNT << '\n';

	return move;
}


Stockfish::Move stockfish_iterative(Stockfish::Position& position, time_t stop_time, int ply, bool is_hard=true) {
	STOP = false;
    initialize_history();
	time_t start = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	Stockfish::Move move;
	
	for (int32_t deep = 1; deep < 1000; ++deep) {
		int alpha = -1e9 - 1000, beta = 1e9 + 1000;
        if (is_hard) {
            std::future<Stockfish::Move> thread = std::async(search<Stockfish::Move>, std::ref(position), deep, alpha, beta, 0, ply, false);

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
        }
        else {
            if (std::chrono::high_resolution_clock::now().time_since_epoch().count() - start >= stop_time / 2) {
                break;
            }
            move = search<Stockfish::Move>(position, deep, alpha, beta, 0, ply, false);
        }

		std::cerr << "base depth: " << deep << endl;
	}
	STOP = false;

	return move;
}


void stockfish_battle(Stockfish::Move bot1(Stockfish::Position&), Stockfish::Move bot2(Stockfish::Position&)) {
    initialize_history();
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
