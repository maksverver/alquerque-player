#ifndef BOARD_H_INCLUDED
#define BOARD_H_INCLUDED

#include "common.h"

typedef long long bitboard_t;

typedef struct
{
    bitboard_t stones[2];
    bitboard_t last[2];
} board_t;


typedef bitboard_t move_t;

#define FIELDS (49)
#define BIT(n) (1LL<<(n))
#define ALLFLD (BIT(FIELDS)-1)

/* Defined in board.c */
extern board_t initial_board;

void print_board(FILE *fp, const board_t *board, bool flipped);
void print_bitboard(FILE *fp, const bitboard_t *bitboard);  /* for debugging */


/* Defined in moves.c */
__inline__ void execute(board_t *dst, const board_t *src, const move_t *move);
int generate_moves(move_t *moves, const board_t *board);
const char *pos_to_str(int field);
const char *move_to_str(const board_t *board, const move_t *move);


#endif /* ndef BOARD_H_INCLUDED */
