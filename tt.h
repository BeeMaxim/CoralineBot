#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>

#include "types.h"

// Типы границ для оценки
enum Bound {
    BOUND_NONE,   // Нет границы
    BOUND_LOWER,  // Нижняя граница (оценка >= beta)
    BOUND_UPPER,  // Верхняя граница (оценка <= alpha)
    BOUND_EXACT   // Точная оценка (alpha < оценка < beta)
};

// Структура для хранения данных в transposition table
struct TTEntry {
    uint64_t key;      // Ключ позиции (часть Zobrist hash)
    int depth;         // Глубина поиска
    int value;         // Оценка позиции
    Stockfish::Move move;      // Лучший ход (например, закодированный как short)
    Bound bound;       // Тип границы
};

// Упрощённая transposition table
class TranspositionTable {
public:
    TranspositionTable(size_t size) : table(size) {}

    // Сохранение данных в таблицу
    void save(uint64_t key, int value, Bound bound, int depth, Stockfish::Move move) {
        size_t index = key % table.size();
        table[index] = {key, depth, value, move, bound};
    }

    // Поиск данных в таблице
    TTEntry* probe(uint64_t key) {
        size_t index = key % table.size();
        if (table[index].key == key) {
            return &table[index];
        }
        return nullptr; // Запись не найдена
    }

    // Очистка таблицы
    void clear() {
        std::memset(table.data(), 0, table.size() * sizeof(TTEntry));
    }

private:
    std::vector<TTEntry> table; // Хеш-таблица
};
