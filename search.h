#ifndef SEARCH_H_INCLUDED
#define SEARCH_H_INCLUDED

#include "board.h"

extern long long search_branches;   /* Total branch nodes */
extern long long search_leaves;     /* Total search nodes */
extern long long search_forced;     /* Branch nodes with 1 move */
extern long long search_lost;       /* Branch nodes with 0 moves */

int search_negamax(const board_t *board, int search_depth, int alpha, int beta);
int search_best_move(const board_t *board, move_t *move, double max_time);

#endif /* ndef SEARCH_H_INCLUDED */
