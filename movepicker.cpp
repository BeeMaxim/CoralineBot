#include <cassert>
#include <limits>

#include "bitboard_.h"
#include "misc.h"
#include "position_.h"
#include "movegen.h"
#include "movepicker.h"


namespace Stockfish {

namespace {

enum Stages {
    // generate main search moves
    MAIN_TT,
    CAPTURE_INIT,
    GOOD_CAPTURE,
    QUIET_INIT,
    GOOD_QUIET,
    BAD_CAPTURE,
    BAD_QUIET,

    // generate evasion moves
    EVASION_TT,
    EVASION_INIT,
    EVASION,

    // generate probcut moves
    PROBCUT_TT,
    PROBCUT_INIT,
    PROBCUT,

    // generate qsearch moves
    QSEARCH_TT,
    QCAPTURE_INIT,
    QCAPTURE
};

// Sort moves in descending order up to and including a given limit.
// The order of moves smaller than the limit is left unspecified.
void partial_insertion_sort(ExtMove* begin, ExtMove* end, int limit) {

    for (ExtMove *sortedEnd = begin, *p = begin + 1; p < end; ++p)
        if (p->value >= limit)
        {
            ExtMove tmp = *p, *q;
            *p          = *++sortedEnd;
            for (q = sortedEnd; q != begin && *(q - 1) < tmp; --q)
                *q = *(q - 1);
            *q = tmp;
        }
}

}  // namespace


// Constructors of the MovePicker class. As arguments, we pass information
// to decide which class of moves to emit, to help sorting the (presumably)
// good moves first, and how important move ordering is at the current node.

// MovePicker constructor for the main search and for the quiescence search
MovePicker::MovePicker(const Position&              p,
                       Move                         ttm,
                       Depth depth) :
    pos(p),
    ttMove(ttm) {

    if (pos.checkers())
        stage = EVASION_TT + !(ttm && pos.pseudo_legal(ttm));

    else
        stage = (depth > 0 ? MAIN_TT : QSEARCH_TT) + !(ttm && pos.pseudo_legal(ttm));
}

// MovePicker constructor for ProbCut: we generate captures with Static Exchange
// Evaluation (SEE) greater than or equal to the given threshold.
/*
MovePicker::MovePicker(const Position& p, Move ttm, int th) :
    pos(p),
    ttMove(ttm),
    threshold(th) {
    assert(!pos.checkers());

    stage = PROBCUT_TT
          + !(ttm && pos.capture_stage(ttm) && pos.pseudo_legal(ttm) && pos.see_ge(ttm, threshold));
}*/

// Assigns a numerical value to each move in a list, used for sorting.
// Captures are ordered by Most Valuable Victim (MVV), preferring captures
// with a good history. Quiets moves are ordered using the history tables.
template<GenType Type>
void MovePicker::score() {

    static_assert(Type == CAPTURES || Type == QUIETS || Type == EVASIONS, "Wrong type");

    [[maybe_unused]] Bitboard threatenedByPawn, threatenedByMinor, threatenedByRook,
      threatenedPieces;
    if constexpr (Type == QUIETS)
    {
        Color us = pos.side_to_move();

        threatenedByPawn = pos.attacks_by<PAWN>(~us);
        threatenedByMinor =
          pos.attacks_by<KNIGHT>(~us) | pos.attacks_by<BISHOP>(~us) | threatenedByPawn;
        threatenedByRook = pos.attacks_by<ROOK>(~us) | threatenedByMinor;

        // Pieces threatened by pieces of lesser material value
        threatenedPieces = (pos.pieces(us, QUEEN) & threatenedByRook)
                         | (pos.pieces(us, ROOK) & threatenedByMinor)
                         | (pos.pieces(us, KNIGHT, BISHOP) & threatenedByPawn);
    }

    for (auto& m : *this)
        if constexpr (Type == CAPTURES)
            m.value =
              7 * int(PieceValue[pos.piece_on(m.to_sq())]);

        else if constexpr (Type == QUIETS)
        {
            Piece     pc   = pos.moved_piece(m);
            PieceType pt   = type_of(pc);
            Square    from = m.from_sq();
            Square    to   = m.to_sq();

            // bonus for checks
            m.value += bool(pos.check_squares(pt) & to) * 16384;

            // bonus for escaping from capture
            m.value += threatenedPieces & from ? (pt == QUEEN && !(to & threatenedByRook)   ? 51700
                                                  : pt == ROOK && !(to & threatenedByMinor) ? 25600
                                                  : !(to & threatenedByPawn)                ? 14450
                                                                                            : 0)
                                               : 0;

            // malus for putting piece en prise
            m.value -= (pt == QUEEN ? bool(to & threatenedByRook) * 49000
                        : pt == ROOK && bool(to & threatenedByMinor) ? 24335
                                                                     : 0);
        }

        else  // Type == EVASIONS
        {
            if (pos.capture_stage(m))
                m.value =
                  PieceValue[pos.piece_on(m.to_sq())] - type_of(pos.moved_piece(m)) + (1 << 28);
        }
}

// Returns the next move satisfying a predicate function.
// This never returns the TT move, as it was emitted before.
template<typename Pred>
Move MovePicker::select(Pred filter) {

    for (; cur < endMoves; ++cur)
        if (*cur != ttMove && filter())
            return *cur++;

    return Move::none();
}

// This is the most important method of the MovePicker class. We emit one
// new pseudo-legal move on every call until there are no more moves left,
// picking the move with the highest score from a list of generated moves.
Move MovePicker::next_move() {

    auto quiet_threshold = [](Depth d) { return -3560 * d; };

top:
    switch (stage)
    {

    case MAIN_TT :
    case EVASION_TT :
    case QSEARCH_TT :
    case PROBCUT_TT :
        ++stage;
        return ttMove;

    case CAPTURE_INIT :
    case PROBCUT_INIT :
    case QCAPTURE_INIT :
        cur = endBadCaptures = moves;
        endMoves             = generate<CAPTURES>(pos, cur);

        score<CAPTURES>();
        partial_insertion_sort(cur, endMoves, std::numeric_limits<int>::min());
        ++stage;
        goto top;

    case GOOD_CAPTURE :
        if (select([&]() {
                // Move losing capture to endBadCaptures to be tried later
                return pos.see_ge(*cur, -cur->value / 18) ? true
                                                          : (*endBadCaptures++ = *cur, false);
            }))
            return *(cur - 1);

        ++stage;
        [[fallthrough]];

    case QUIET_INIT :
        if (!skipQuiets)
        {
            cur      = endBadCaptures;
            endMoves = beginBadQuiets = endBadQuiets = generate<QUIETS>(pos, cur);

            score<QUIETS>();
            partial_insertion_sort(cur, endMoves, quiet_threshold(depth));
        }

        ++stage;
        [[fallthrough]];

    case GOOD_QUIET :
        if (!skipQuiets && select([]() { return true; }))
        {
            if ((cur - 1)->value > -7998 || (cur - 1)->value <= quiet_threshold(depth))
                return *(cur - 1);

            // Remaining quiets are bad
            beginBadQuiets = cur - 1;
        }

        // Prepare the pointers to loop over the bad captures
        cur      = moves;
        endMoves = endBadCaptures;

        ++stage;
        [[fallthrough]];

    case BAD_CAPTURE :
        if (select([]() { return true; }))
            return *(cur - 1);

        // Prepare the pointers to loop over the bad quiets
        cur      = beginBadQuiets;
        endMoves = endBadQuiets;

        ++stage;
        [[fallthrough]];

    case BAD_QUIET :
        if (!skipQuiets)
            return select([]() { return true; });

        return Move::none();

    case EVASION_INIT :
        cur      = moves;
        endMoves = generate<EVASIONS>(pos, cur);

        score<EVASIONS>();
        partial_insertion_sort(cur, endMoves, std::numeric_limits<int>::min());
        ++stage;
        [[fallthrough]];

    case EVASION :
    case QCAPTURE :
        return select([]() { return true; });

    case PROBCUT :
        return select([&]() { return pos.see_ge(*cur, threshold); });
    }

    assert(false);
    return Move::none();  // Silence warning
}

void MovePicker::skip_quiet_moves() { skipQuiets = true; }

}  // namespace Stockfish