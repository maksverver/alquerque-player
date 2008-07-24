#include "board.h"
#include <stdbool.h>

#define MAX_MOVE_LEN    20      /* FIXME: what's the real maximum? */

/* Defined in tables.h */
extern int moves_normal[49][9];
extern int moves_jumps[49][9];


/* FIXME: inline this everywhere? */
__inline__ void execute(board_t *dst, const board_t *src, const move_t *move)
{
    bitboard_t taken = (*move) & (src->stones[1]);
    dst->stones[0] = src->stones[1] ^ taken;
    dst->stones[1] = src->stones[0] ^ *move ^ taken;
    dst->last[0] = src->last[1];
    dst->last[1] = *move;
}

static int lowbit(long long i)
{
    int res = 0;
    if (i <= 0)
        return -1;
    while ((i&1) == 0)
    {
        i >>= 1;
        ++res;
    }
    return res;
}

/* State used by move generation functions */
static const board_t *current_board;
static bitboard_t occupied;
static move_t *current_move;
static move_t tmp_move;

static char move_buf[3*MAX_MOVE_LEN];

const char *pos_to_str(int field)
{
    static char buf[3] = { };
    buf[0] = 'a' + field%7;
    buf[1] = '7' - field/7;
    return buf;
}

bool format_move(char *buf, int f, bitboard_t todo, bitboard_t occupied)
{
    int n, g, h;

    if (todo == BIT(f))
        return true;

    for (n = 0; moves_jumps[f][n] >= 0; ++n)
    {
        if ( !(occupied & BIT(h = moves_jumps[f][n])) &&
             ((occupied & todo) & BIT(g = (f + h)/2)) )
        {
            *buf = '*';
            strcpy(buf + 1, pos_to_str(h));
            if (format_move(buf + 3, h, todo ^ BIT(g), occupied))
                return true;
        }
    }
    return false;
}

const char *move_to_str(const board_t *board, const move_t *move)
{
    assert((bitcount(*move)) >= 2);

    if (bitcount(*move) == 2)
    {
        int i = lowbit(*move & board->stones[0]),
            j = lowbit(*move & ~board->stones[0]);

        strcpy(move_buf, pos_to_str(i));
        strcat(move_buf, "-");
        strcat(move_buf, pos_to_str(j));
    }
    else
    {
        bitboard_t occupied = board->stones[0] | board->stones[1];
        int f;
        if (*move & board->stones[0])
        {
            f = lowbit(*move & board->stones[0]);
            strcpy(move_buf, pos_to_str(f));
            format_move(move_buf + 2, f, *move ^ BIT(f), occupied ^ BIT(f));
        }
        else
        for(f = 0; f < FIELDS; ++f)
        {
            strcpy(move_buf, pos_to_str(f));
            if (format_move(move_buf + 2, f, *move ^ BIT(f), occupied ^ BIT(f)))
                break;
        }
    }
    return move_buf;
}


static int cmp_moves(const void *a, const void *b)
{
    return *(move_t*)a - *(move_t*)b;
}

static move_t *remove_duplicate_moves(move_t *begin, move_t *end)
{
    move_t *ptr, *new_end;

    qsort(begin, end - begin, sizeof(*begin), cmp_moves);

    for (ptr = new_end = begin + 1; ptr != end; ++ptr)
        if (ptr[0] != ptr[-1])
            *new_end++ = *ptr;

    return new_end;
}


void generate_capture_moves(int f)
{
    int n, g, h;
    bool generated = false;

    for (n = 0; moves_jumps[f][n] >= 0; ++n)
    {
        if ( !(occupied & BIT(h = moves_jumps[f][n])) &&
             ((current_board->stones[1] ^ tmp_move) & BIT(g = (f + h)/2)) )
        {
            tmp_move ^= BIT(g);
            generate_capture_moves(h);
            tmp_move ^= BIT(g);
            generated = true;
        }
    }

    if (!generated && (tmp_move&(tmp_move - 1)))
        *current_move++ = tmp_move ^ BIT(f);
}

int generate_moves(move_t *moves, const board_t *board)
{
    int n, *m;
    bitboard_t b;

    current_board = board;
    occupied = board->stones[0] | board->stones[1];
    current_move = moves;

    /* Generate capture moves */
    for (n = 0, b = 1; n < 49; ++n, b = b+b)
        if (board->stones[0] & b)
        {
            tmp_move = b;
            occupied ^= tmp_move;
            generate_capture_moves(n);
            occupied ^= tmp_move;
        }

    if (current_move == moves)
    {
        /* Generate normal moves */
        for (n = 0, b = 1; n < 49; ++n, b = b+b)
            if (board->stones[0] & b)
            {
                m = moves_normal[n];
                do {
                    if (!(occupied & BIT(*m)))
                    {
                        *current_move = b | BIT(*m);
                        if (*current_move != board->last[0])
                            ++current_move;
                    }
                } while(*++m >= 0);
            }
    }
    else
    {
        /* Eliminate duplicates from capture moves */
#ifndef TEST1
        current_move = remove_duplicate_moves(moves, current_move);
#endif
    }

    return current_move - moves;
}
