#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>

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

enum Player { None = 0, White, Black };

const int dr[8] = {  +1,  0, -1,  0, +1, +1, -1, -1 };
const int dc[8] = {   0, +1,  0, -1, +1, -1, +1, -1 };

enum Player board[7][7] = {
    { White, White, White, White, White, White, White },
    { White, White, White, White, White, White, White },
    { White, White, White, White, White, White, White },
    { Black, Black, Black, None,  White, White, White },
    { Black, Black, Black, Black, Black, Black, Black },
    { Black, Black, Black, Black, Black, Black, Black },
    { Black, Black, Black, Black, Black, Black, Black } };

enum Player player = White;

char line_buf[100];
char move_buf[100];

struct Move
{
    int r1, c1, r2, c2;
} last_move[2];

struct MoveList
{
    struct MoveList *next;
    char str[1];
} *moves;

enum Player other(enum Player player)
{
    return player == White ? Black : player == Black ? White : None;
}

#define dirs(r,c) (((r) + (c))%2 ? 4 : 8)

void free_moves(struct MoveList *cur)
{
    if(cur)
    {
        free_moves(cur->next);
        free(cur);
    }
}

void gen_capture_moves(int r1, int c1, int first)
{
    bool caps = false;
    int d, l = strlen(move_buf);

    for (d = 0; d < ((r1 + c1)%2 ? 4 : 8); ++d)
    {
        int r2 = r1 + dr[d], c2 = c1 + dc[d],
            r3 = r2 + dr[d], c3 = c2 + dc[d];
        if (r3 >= 0 && r3 < 7 && c3 >= 0 && c3 < 7 &&
            board[r3][c3] == None && board[r2][c2] == other(player))
        {
            sprintf(move_buf + l, "*%c%d", 'a' + c3, 1 + r3);
            board[r2][c2] = None;
            gen_capture_moves(r3, c3, 0);
            board[r2][c2] = other(player);
            move_buf[l] = '\0';
            caps = true;
        }
    }

    if (!caps && !first)
    {
        struct MoveList *move = malloc(sizeof(struct MoveList) + l);
        strcpy(move->str, move_buf);
        move->next = moves;
        moves = move;
    }
}

void gen_moves()
{
    int r1, c1, r2, c2, d;

    free_moves(moves);
    moves = 0;

    for (r1 = 0; r1 < 7; ++r1)
        for (c1 = 0; c1 < 7; ++c1)
            if (board[r1][c1] == player)
            {
                sprintf(move_buf, "%c%d", 'a' + c1, 1 + r1);
                board[r1][c1] = None;
                gen_capture_moves(r1, c1, 1);
                board[r1][c1] = player;
            }

    if (moves)
        return;

    for (r1 = 0; r1 < 7; ++r1)
        for (c1 = 0; c1 < 7; ++c1)
            if (board[r1][c1] == player)
            {
                for (d = 0; d < dirs(r1, c1); ++d)
                {
                    r2 = r1 + dr[d], c2 = c1 + dc[d];
                    if (r2 >= 0 && r2 < 7 && c2 >= 0 && c2 < 7 &&
                        board[r2][c2] == None &&
                        (last_move[0].r1 != r2 || last_move[0].c1 != c2 ||
                         last_move[0].r2 != r1 || last_move[0].c2 != c1))
                    {
                        struct MoveList *move = malloc(sizeof(struct MoveList) + 5);
                        sprintf(move->str, "%c%d-%c%d",
                            'a' + c1, 1 + r1, 'a' + c2, 1 + r2);
                        move->next = moves;
                        moves = move;
                    }
                }
            }
}

void print(FILE *fp)
{
    int r, c;
    struct MoveList *move;

    for (r = 0; r < 7; ++r)
    {
        if (r > 0)
        {
            fprintf(fp, "   |");
            for (c = 0; c < 6; ++c)
                fprintf(fp, "%c|", (r + c)%2 ? '\\' : '/');
            fprintf(fp, "\n");
        }

        fprintf(fp, " %s%d%s", BOLD, 7 - r, NORMAL);
        for (c = 0; c < 7; ++c)
        {
            fprintf(fp, "%c%s%s%c%s",
                c > 0 ? '-' : ' ',
                BOLD,
                board[6 - r][c] == White ? RED :
                board[6 - r][c] == Black ? BLUE : NORMAL,
                board[6 - r][c] == White ? 'X' :
                board[6 - r][c] == Black ? 'O' : ' ',
                NORMAL );
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n  %s", BOLD);
    for (c = 0; c < 7; ++c)
        fprintf(fp, " %c", 'a' + c);
    fprintf(fp, "%s\n\n", NORMAL);

    gen_moves();
    fprintf(stderr, "Moves for %s:%s",
        player == White ? BOLD RED "red" NORMAL : BOLD BLUE "blue" NORMAL, BOLD YELLOW);
    for (move = moves; move; move = move->next)
        fprintf(fp, " %s", move->str);
    fprintf(fp, "%s\n", NORMAL);
}

void execute(char *move)
{
    last_move[0] = last_move[1];

    if (move[2] == '-')
    {
        struct Move m = { move[1] - '1', move[0] - 'a',
                          move[4] - '1', move[3] - 'a' };
        last_move[1] = m;
        board[m.r1][m.c1] = None;
        board[m.r2][m.c2] = player;
    }
    else
    {
        last_move[1].r1 = -1;
        while (move[2] == '*')
        {
            int r1 = move[1] - '1', c1 = move[0] - 'a',
                r2 = move[4] - '1', c2 = move[3] - 'a';
            board[r1][c1] = None;
            board[(r1 + r2)/2][(c1 + c2)/2] = None;
            board[r2][c2] = player;
            move += 3;
        }
    }

    player = other(player);
}

void play_random_move()
{
    struct MoveList *move;
    int count;

    gen_moves();

    count = 0;
    for (move = moves; move; move = move->next)
        ++count;

    count = rand()%count;
    move = moves;
    while(count--)
        move = move->next;

    fprintf(stderr, "I play: %s%s%s\n", BOLD GREEN, move->str, NORMAL);
    print(stderr);
    fflush(stderr);

    fprintf(stdout, "%s\n", move->str);
    fflush(stdout);

    execute(move->str);
}

char *read_line()
{
    int l;

    if (!fgets(line_buf, sizeof(line_buf), stdin))
    {
        fprintf(stderr, "Unexpected end of input!\n");
        exit(1);
    }

    l = strlen(line_buf);
    while (l > 0 && line_buf[l - 1] <= ' ')
        line_buf[--l] = '\0';

    if(strcmp(line_buf, "Quit") == 0)
    {
        fprintf(stderr, "Received Quit.\n");
        exit(0);
    }

    return line_buf;
}

int main(int argc, char *argv[])
{
    int moves = 100, n;
    int seed = (int)time(NULL) ^ (((int)getpid())<<16);

    for (n = 1; n < argc - 1; ++n)
        if (strcmp(argv[n], "--seed") == 0 || strcmp(argv[n], "-s") == 0)
            sscanf(argv[n + 1], "%d", &seed);

    fprintf(stderr, "Using random seed %d.\n", seed);


    srand(seed);
    print(stderr);

    if (strcmp(read_line(), "Start") == 0)
    {
        play_random_move();
        read_line();
        --moves;
    }

    while(moves--)
    {
        fprintf(stderr, "You play: %s%s%s\n", BOLD GREEN, line_buf, NORMAL);
        execute(line_buf);
        print(stderr);

        play_random_move();
        gen_moves();
        if (!moves)
            break;

        read_line();
    }

    return 0;
}
