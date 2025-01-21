#include "bitboard_.h"
#include "position_.h"
#include "types.h"
#include <iostream>


constexpr int KingAttackWeights[Stockfish::PIECE_TYPE_NB] = { 0, 0, 81, 52, 44, 10 };

#define S(mg, eg) Stockfish::make_score(mg, eg)


constexpr Stockfish::Score MobilityBonus[][32] = {
    { S(-62,-81), S(-53,-56), S(-12,-30), S( -4,-14), S(  3,  8), S( 13, 15), // Knight
      S( 22, 23), S( 28, 27), S( 33, 33) },
    { S(-48,-59), S(-20,-23), S( 16, -3), S( 26, 13), S( 38, 24), S( 51, 42), // Bishop
      S( 55, 54), S( 63, 57), S( 63, 65), S( 68, 73), S( 81, 78), S( 81, 86),
      S( 91, 88), S( 98, 97) },
    { S(-58,-76), S(-27,-18), S(-15, 28), S(-10, 55), S( -5, 69), S( -2, 82), // Rook
      S(  9,112), S( 16,118), S( 30,132), S( 29,142), S( 32,155), S( 38,165),
      S( 46,166), S( 48,169), S( 58,171) },
    { S(-39,-36), S(-21,-15), S(  3,  8), S(  3, 18), S( 14, 34), S( 22, 54), // Queen
      S( 28, 61), S( 41, 73), S( 43, 79), S( 48, 92), S( 56, 94), S( 60,104),
      S( 60,113), S( 66,120), S( 67,123), S( 70,126), S( 71,133), S( 73,136),
      S( 79,140), S( 88,143), S( 88,148), S( 99,166), S(102,170), S(102,175),
      S(106,184), S(109,191), S(113,206), S(116,212) }
  };

  // RookOnFile[semiopen/open] contains bonuses for each rook when there is
  // no (friendly) pawn on the rook file.
  constexpr Stockfish::Score RookOnFile[] = { S(21, 4), S(47, 25) };

  // ThreatByMinor/ByRook[attacked PieceType] contains bonuses according to
  // which piece type attacks which one. Attacks on lesser pieces which are
  // pawn-defended are not considered.
  constexpr Stockfish::Score ThreatByMinor[Stockfish::PIECE_TYPE_NB] = {
    S(0, 0), S(5, 32), S(57, 41), S(77, 56), S(88, 119), S(79, 161)
  };

  constexpr Stockfish::Score ThreatByRook[Stockfish::PIECE_TYPE_NB] = {
    S(0, 0), S(2, 44), S(36, 71), S(36, 61), S(0, 38), S(51, 38)
  };

  // PassedRank[Rank] contains a bonus according to the rank of a passed pawn
  constexpr Stockfish::Score PassedRank[Stockfish::RANK_NB] = {
    S(0, 0), S(10, 28), S(17, 33), S(15, 41), S(62, 72), S(168, 177), S(276, 260)
  };

  // Assorted bonuses and penalties
  constexpr Stockfish::Score BishopPawns         = S(  3,  7);
  constexpr Stockfish::Score CorneredBishop      = S( 50, 50);
  constexpr Stockfish::Score FlankAttacks        = S(  8,  0);
  constexpr Stockfish::Score Hanging             = S( 69, 36);
  constexpr Stockfish::Score KingProtector       = S(  7,  8);
  constexpr Stockfish::Score KnightOnQueen       = S( 16, 12);
  constexpr Stockfish::Score LongDiagonalBishop  = S( 45,  0);
  constexpr Stockfish::Score MinorBehindPawn     = S( 18,  3);
  constexpr Stockfish::Score Outpost             = S( 30, 21);
  constexpr Stockfish::Score PassedFile          = S( 11,  8);
  constexpr Stockfish::Score PawnlessFlank       = S( 17, 95);
  constexpr Stockfish::Score RestrictedPiece     = S(  7,  7);
  constexpr Stockfish::Score RookOnQueenFile     = S(  7,  6);
  constexpr Stockfish::Score SliderOnQueen       = S( 59, 18);
  constexpr Stockfish::Score ThreatByKing        = S( 24, 89);
  constexpr Stockfish::Score ThreatByPawnPush    = S( 48, 39);
  constexpr Stockfish::Score ThreatBySafePawn    = S(173, 94);
  constexpr Stockfish::Score TrappedRook         = S( 52, 10);
  constexpr Stockfish::Score WeakQueen           = S( 49, 15);
  constexpr Stockfish::Score WeakQueenProtection = S( 14,  0);


Stockfish::Bitboard mobilityArea[Stockfish::COLOR_NB];
Stockfish::Score mobility[Stockfish::COLOR_NB] = { Stockfish::SCORE_ZERO, Stockfish::SCORE_ZERO };

    // attackedBy[color][piece type] is a bitboard representing all squares
    // attacked by a given color and piece type. Special "piece types" which
    // is also calculated is ALL_PIECES.
Stockfish::Bitboard attackedBy[Stockfish::COLOR_NB][Stockfish::PIECE_TYPE_NB];

    // attackedBy2[color] are the squares attacked by at least 2 units of a given
    // color, including x-rays. But diagonal x-rays through pawns are not computed.
Stockfish::Bitboard attackedBy2[Stockfish::COLOR_NB];

    // kingRing[color] are the squares adjacent to the king plus some other
    // very near squares, depending on king position.
Stockfish::Bitboard kingRing[Stockfish::COLOR_NB];

    // kingAttackersCount[color] is the number of pieces of the given color
    // which attack a square in the kingRing of the enemy king.
int kingAttackersCount[Stockfish::COLOR_NB];

    // kingAttackersWeight[color] is the sum of the "weights" of the pieces of
    // the given color which attack a square in the kingRing of the enemy king.
    // The weights of the individual piece types are given by the elements in
    // the KingAttackWeights array.
int kingAttackersWeight[Stockfish::COLOR_NB];

    // kingAttacksCount[color] is the number of attacks by the given color to
    // squares directly adjacent to the enemy king. Pieces which attack more
    // than one square are counted multiple times. For instance, if there is
    // a white knight on g5 and black's king is on g8, this white knight adds 2
    // to kingAttacksCount[WHITE].
int kingAttacksCount[Stockfish::COLOR_NB];


template<Stockfish::Color Us>
int evaluate_mobility(Stockfish::Position& pos) {
    int mobilityScore = 0;
    Stockfish::Bitboard mobilityArea[Stockfish::COLOR_NB];
    Stockfish::Bitboard attackedBy[Stockfish::COLOR_NB][Stockfish::PIECE_TYPE_NB] = {};

    // Инициализация зоны мобильности
    mobilityArea[Us] = ~(pos.pieces(Us, Stockfish::PAWN) | pos.pieces(Us, Stockfish::KING, Stockfish::QUEEN) | pos.blockers_for_king(Us)); // | pos.pawn_attacks(~Us));

    // Перебор всех фигур, кроме пешек и короля
    for (Stockfish::PieceType pt : {Stockfish::KNIGHT, Stockfish::BISHOP, Stockfish::ROOK, Stockfish::QUEEN}) {
        Stockfish::Bitboard pieces = pos.pieces(Us, pt);
        while (pieces) {
            Stockfish::Square s = Stockfish::pop_lsb(pieces);

            // Вычисляем атакованные поля
            Stockfish::Bitboard attacks = Stockfish::attacks_bb(pt, s, pos.pieces());

            // Учитываем блокировку короля
            if (pos.blockers_for_king(Us) & s) {
                attacks &= Stockfish::LineBB[pos.square<Stockfish::KING>(Us)][s];
            }

            // Подсчитываем количество атакованных полей в зоне мобильности
            int mob = Stockfish::popcount(attacks & mobilityArea[Us]);
            // int mob = Stockfish::popcount(attacks);

            // Добавляем бонус за мобильность
            mobilityScore += Stockfish::mg_value(MobilityBonus[pt - 2][mob]);

            // Обновляем общие атакованные поля
            attackedBy[Us][pt] |= attacks;
            attackedBy[Us][Stockfish::ALL_PIECES] |= attacks;
        }
    }

    return mobilityScore;
}
