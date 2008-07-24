#include "board.h"

#ifndef ANSI
#define BOLD    ""
#define NORMAL  ""
#define RED     ""
#define GREEN   ""
#define YELLOW  ""
#define BLUE    ""
#else
#define BOLD    "\033[1m"
#define NORMAL  "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#endif

board_t initial_board = { { (((1LL<<21) - 1LL)<<28) | (((1LL<<3) - 1LL)<<25),
                            (((1LL<<21) - 1LL)<< 0) | (((1LL<<3) - 1LL)<<21) } };

void print_board(FILE *fp, const board_t *board, bool flipped)
{
    int r, c;

    for (r = 0; r < 7; ++r)
    {
        if (r > 0)
        {
            fprintf(fp, "   |");
            for (c = 0; c < 6; ++c)
                fprintf(fp, "%c|", (r + c)%2 ? '\\' : '/');
            fprintf(fp, "\n");
        }

        fprintf(fp, " %s%d%s", BOLD, flipped ? 1 + r : 7 - r, NORMAL);
        for (c = 0; c < 7; ++c)
        {
            int f = 7*(flipped ? 6 - r : r) + (flipped ? 6 - c : c);
            fprintf(fp, "%c%s%s%c%s",
                c > 0 ? '-' : ' ',
                BOLD,
                (board->stones[ flipped]&BIT(f)) ? RED :
                (board->stones[!flipped]&BIT(f)) ? BLUE : NORMAL,
                (board->stones[ flipped]&BIT(f)) ? 'X' :
                (board->stones[!flipped]&BIT(f)) ? 'O' : ' ',
                NORMAL );
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n  %s", BOLD);
    for (c = 0; c < 7; ++c)
        fprintf(fp, " %c", flipped ? 'g' - c : 'a' + c);
    fprintf(fp, "%s\n\n", NORMAL);
}

void print_bitboard(FILE *fp, const bitboard_t *bitboard)
{
    int r, c;

    for (r = 0; r < 7; ++r)
    {
        for (c = 0; c < 7; ++c)
            fputc((*bitboard & BIT(7*r + c)) ? '1' : '0', fp);
        fputc('\n', fp);
    }
}
