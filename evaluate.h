#include "bitboard_.h"
#include "position_.h"


constexpr int MobilityBonus[][32] = {
    { -62, -53, -12,  -4,   3,  13, // Knight
       22,  28,  33 },
    { -48, -20,  16,  26,  38,  51, // Bishop
       55,  63,  63,  68,  81,  81,
       91,  98 },
    { -58, -27, -15, -10,  -5,  -2, // Rook
        9,  16,  30,  29,  32,  38,
       46,  48,  58 },
    { -39, -21,   3,   3,  14,  22, // Queen
       28,  41,  43,  48,  56,  60,
       60,  66,  67,  70,  71,  73,
       79,  88,  88,  99, 102, 102,
      106, 109, 113, 116 }
};


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
            mobilityScore += MobilityBonus[pt - 2][mob];

            // Обновляем общие атакованные поля
            attackedBy[Us][pt] |= attacks;
            attackedBy[Us][Stockfish::ALL_PIECES] |= attacks;
        }
    }

    return mobilityScore;
}