/*
    Long term:
        bitboards
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>


#define INL __inline__

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


/* Players (also used for occupation of fields) */
#define NONE        (0)
#define WHITE       (1)
#define BLACK       (2)
#define INVALID     (3)

/* Move representation

    Normal move:    start, end, -1
    Capture move:   start, captured, .. captured, end, -1
*/

#define MAX_DEPTH       (50)        /* Maximum search depth */
#define MAX_STATES      (50000)     /* Maximum number of states to evaluate */
#define MAX_MOVE_LEN    (20)        /* Est. Maximum number of steps in a move (FIXME) */
#define MAX_MOVES       (200)       /* Est. maximum number of moves in one state (FIXME) */
#define MAX_GAME_LEN    (200)       /* Maximum number of moves in a game */

#define INF             (999999999) /* "Infinite" value */

typedef unsigned char Occupation;
typedef signed char Field;

typedef signed int Value;

typedef Field (Move)[MAX_MOVE_LEN];

/* I/O */
int line_read = 0;
char line_buf[1024];


/*
 *      GLOBAL CONSTANTS
 */
static const int dir_moves[8] = { -8, -1, +1, +8, -9, -7, +7, +9 };



/*
 *      GLOBAL VARIABLES
 */


static Occupation real_board[] = {
    INVALID,INVALID,INVALID,INVALID,INVALID,INVALID,INVALID,INVALID,
    INVALID,WHITE,  WHITE,  WHITE,  WHITE,  WHITE,  WHITE,  WHITE,
    INVALID,WHITE,  WHITE,  WHITE,  WHITE,  WHITE,  WHITE,  WHITE,
    INVALID,WHITE,  WHITE,  WHITE,  WHITE,  WHITE,  WHITE,  WHITE,
    INVALID,BLACK,  BLACK,  BLACK,  NONE,   WHITE,  WHITE,  WHITE,
    INVALID,BLACK,  BLACK,  BLACK,  BLACK,  BLACK,  BLACK,  BLACK,
    INVALID,BLACK,  BLACK,  BLACK,  BLACK,  BLACK,  BLACK,  BLACK,
    INVALID,BLACK,  BLACK,  BLACK,  BLACK,  BLACK,  BLACK,  BLACK,
    INVALID,INVALID,INVALID,INVALID,INVALID,INVALID,INVALID,INVALID,
    INVALID };

/* Board is described as a 7x8 array (in row major order) */
static Occupation *board = real_board + 9;

/* Available moves */
Move real_moves[MAX_DEPTH*MAX_MOVES];
Move *moves_begin = real_moves, *moves_end = real_moves;
long long bitmasks[MAX_DEPTH*MAX_MOVES];

/* History of all moves made */
int history_size = 0;
Field real_history[2 + MAX_GAME_LEN + MAX_DEPTH][MAX_MOVE_LEN];
Field (*history)[MAX_MOVE_LEN] = &real_history[2];

/* The current player */
int player = WHITE;

/* TODO: document */
int my_player, max_depth;
Move best_move;
long long states_evaluated;

/*
 *      IMPLEMENTATION
 */

#define assert_field(field) assert(field >= 0 && field < 7*8 && field%8 < 7)

static INL int other(int player)
{
    assert(player == WHITE || player == BLACK);

    return player^3;
}

static INL int fld_row(int field)
{
    assert_field(field);

    return field>>3;
}

static INL int fld_col(int field)
{
    assert_field(field);

    return field&7;
}

static INL int dirs(int field)
{
    assert_field(field);

    return ((fld_col(field) + fld_row(field))&1) ? 4 : 8;
}

static void do_move(Field *fields)
{
    Field *hist;

    assert(history_size < MAX_GAME_LEN);
    hist = history[history_size++];

    do {
        assert_field(*fields);
        board[(int)*fields] = NONE;
        *hist++ = *fields++;
    } while (*(fields + 1) >= 0);
    assert_field(*fields);
    board[(int)*fields] = player;
    player = other(player);
    *hist++ = *fields++;
    *hist++ = *fields++;
}

static void undo_move()
{
    Field *f;

    assert(history_size > 0);
    --history_size;
    for (f = &history[history_size][1]; *(f + 1) >= 0; ++f)
    {
        assert_field(*f);
        board[(int)*f] = player;
    }
    board[(int)*f] = NONE;

    assert_field(history[history_size][0]);
    board[history[history_size][0]] = player = other(player);
}

void gen_capture_moves(int f)
{
    static Field fields[MAX_MOVE_LEN];
    static long long bitmask;
    static int pos = 0;

    int d, g, h, caps;

    assert_field(f);
    assert(board[f] == NONE);

    if (pos == 0)
    {
        fields[pos++] = f;
        bitmask = (1LL<<f);
    }

    caps = 0;
    for (d = 0; d < dirs(f); ++d)
    {
        if (board[g = f + dir_moves[d]] == other(player) &&
            board[h = g + dir_moves[d]] == NONE)
        {
            fields[pos++] = g;
            bitmask ^= (1LL<<g);
            board[g] = NONE;

            gen_capture_moves(h);

            board[g] = other(player);
            bitmask ^= (1LL<<g);
            --pos;

            caps = 1;
        }
    }

    if (pos == 1)
    {
        pos = 0;
        bitmask = 0;
    }
    else
    if (!caps)
    {
        Move *m;
/*
        for (m = moves_begin; m != moves_end; ++m)
            if (bitmasks[moves_end - real_moves] == bitmask)
                return;
*/
        assert(moves_end - moves_begin < sizeof(real_moves)/sizeof(*real_moves));
        fields[pos + 0] = f;
        fields[pos + 1] = -1;
        memcpy(moves_end, fields, pos + 2);
        bitmasks[moves_end - real_moves] = bitmask; 
        ++moves_end;
    }
}

void generate_moves()
{
    int f, g, d;

    for (f = 0; f < 7*8; ++f)
        if (board[f] == player)
        {
            board[f] = NONE;
            gen_capture_moves(f);
            board[f] = player;
        }

    if (moves_begin != moves_end)
        return;

    for (f = 0; f < 7*8; ++f)
        if (board[f] == player)
        {
            for (d = 0; d < dirs(f); ++d)
            {
                if (board[g = f + dir_moves[d]] == NONE &&
                    (g != history[history_size - 2][0] ||
                     f != history[history_size - 2][1]) )
                {
                    assert(moves_end - moves_begin < sizeof(real_moves)/sizeof(*real_moves));
                    (*moves_end)[0] = f;
                    (*moves_end)[1] = g;
                    (*moves_end)[2] = -1;
                    ++moves_end;
                }
            }
        }
}

const char *move_to_str(Field *fields)
{
    static char buffer[3*MAX_MOVE_LEN];

    char *p = buffer;
    int d, f;

    if (fields[2] < 0)
    {
        *p++ = 'a' + fld_col(fields[0]);
        *p++ = '1' + fld_row(fields[0]);
        *p++ = '-';
        *p++ = 'a' + fld_col(fields[1]);
        *p++ = '1' + fld_row(fields[1]);
    }
    else
    {
        f = *fields++;
        *p++ = 'a' + fld_col(f);
        *p++ = '1' + fld_row(f);
        while (*(fields + 1) >= 0)
        {
            for(d = 0; d < 8; ++d)
                if (f + dir_moves[d] == *fields)
                {
                    f = *fields + dir_moves[d];
                    *p++ = '*';
                    *p++ = 'a' + fld_col(f);
                    *p++ = '1' + fld_row(f);
                    break;
                }
            ++fields;
        }
    }
    *p++ = '\0';

    return buffer;
}

void print_moves()
{
    Move *old_begin, *m;

    old_begin = moves_begin;
    moves_begin = moves_end;
    generate_moves();
    for (m = moves_begin; m != moves_end; ++m)
    {
        fputs(move_to_str(m), stderr);
        if (m + 1 != moves_end)
            fputc(' ', stderr);
    }
    fputc('\n', stderr);
    moves_end   = moves_begin;
    moves_begin = old_begin;
}

void print_board()
{
    FILE *fp = stderr;
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

        fprintf(fp, " %s%d%s", BOLD, 7 - r, NORMAL);
        for (c = 0; c < 7; ++c)
        {
            fprintf(fp, "%c%s%s%c%s",
                c > 0 ? '-' : ' ',
                BOLD,
                board[8*(6 - r) + c] == WHITE ? RED :
                board[8*(6 - r) + c] == BLACK ? BLUE : NORMAL,
                board[8*(6 - r) + c] == WHITE ? 'X' :
                board[8*(6 - r) + c] == BLACK ? 'O' :
                owner(8*(6 - r) + c) == WHITE ? 'x' :
                owner(8*(6 - r) + c) == BLACK ? 'o' : ' ',
                NORMAL );
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n  %s", BOLD);
    for (c = 0; c < 7; ++c)
        fprintf(fp, " %c", 'a' + c);
    fprintf(fp, "%s\n\n", NORMAL);
}

char *peek_line()
{
    char *line;

    if (!line_read)
    {
        for (;;)
        {
            line = fgets(line_buf, sizeof(line_buf), stdin);
            assert(line);
            while (*line) ++line;
            while (line - line_buf > 0 && *(line - 1) <= ' ') --line;
            if (line == line_buf)
                continue;
            *line = '\0';
            break;
        }
        line_read = 1;
    }

    return line_buf;
}

char *read_line()
{
    char *line = peek_line();
    line_read = 0;
    return line;
}

int find_owner(int f, int depth)
{
    int g, d, r;

    r = board[f];
    if (depth > 0)
    {
        for (d = 0; d < dirs(f); ++d)
        {
            g = f + dir_moves[d];
            if (board[g] != INVALID)
                r |= find_owner(g, depth - 1);
        }
    }
    return r;
}

int owner(int f)
{
    int depth, v;

    depth = 0;
    while((v = find_owner(f, ++depth)) == 0);
    return v;
}

int evaluate()
{
    int count[4] = { }, tcount[4] = { }, f;

    ++states_evaluated;

    for(f = 0; f < 8*7; ++f)
    {
        switch (board[f])
        {
        case NONE:
            ++tcount[owner(f)];
            break;

        case WHITE:
        case BLACK:
            ++count[board[f]];
            break;
        }
    }

    return 10000*(count[player] - count[other(player)])/(count[player] + count[other(player)])
           + tcount[player] - tcount[other(player)];
}

int negamax(int depth, int alpha, int beta)
{
    int v;
    Move *m, *old_begin;

    if (depth == max_depth)
        return evaluate();

    old_begin = moves_begin;
    moves_begin = moves_end;

    generate_moves();

    if(moves_begin == moves_end)
    {
        moves_begin = old_begin;
        return -INF;
    }

    for (m = moves_begin; m != moves_end; ++m)
    {
        do_move(*m);
        v = -negamax(depth + 1, -beta, -alpha);
        if (v > alpha)
        {
            alpha = v;
            if (depth == 0)
            {
                memcpy(best_move, *m, sizeof(Move));
            }
        }
        undo_move();
        if (v >= beta)
            break;
    }

    moves_end   = moves_begin;
    moves_begin = old_begin;

    return alpha;
}

Move *find_best_move()
{
    long long total_states = 1;
    int v;

    for (max_depth = 4; max_depth <= MAX_DEPTH; ++max_depth)
    {
        states_evaluated = 0;
        v = negamax(0, -INF, +INF);
        total_states += states_evaluated;
        fprintf(stderr, "max_depth: %d;  value: %d;  evaluated: %lld; total: %lld\n",
            max_depth, v, states_evaluated, total_states );
        if (total_states*total_states/(1 + states_evaluated) > MAX_STATES)
            break;
    }

    return &best_move;
}

char line[1024];

int search(int depth)
{
    Move *m, *old_begin;
    int res, l;

    if (depth == 0)
    {
        printf("%s\n", line + 1);
        return 1;
    }
    l = strlen(line);
    line[l++] = '\t';

    old_begin = moves_begin;
    moves_begin = moves_end;

    generate_moves();

    res = 0;
    for (m = moves_begin; m != moves_end; ++m)
    {
        do_move(*m);
        strcpy(line + l, move_to_str(m));
        res += search(depth - 1);
        undo_move();
    }

    moves_end   = moves_begin;
    moves_begin = old_begin;
    return res;
}

int main()
{
    printf("%d\n", search(6));
}

#if 0
int main(int argc, char *argv[])
{
    if (argc > 1 && strcmp(argv[1], "-t") == 0)
    {
        max_depth = 18;
        printf("%d\n", negamax(0, -INF, +INF));
        printf("%lld\n", states_evaluated);
        return 0;
    }


    if (strcmp(peek_line(), "Start") == 0)
    {
        read_line();
        my_player = WHITE;
    }
    else
    {
        my_player = BLACK;
    }

    while (history_size < MAX_GAME_LEN)
    {
        char *line;
        Move *chosen;

#ifdef PRINTSTATE
        print_board();
        print_moves();
#endif

        moves_begin = moves_end = real_moves;
        generate_moves();
        if (moves_begin == moves_end)
            break;

        if (player == my_player)
        {
            if (moves_end - moves_begin == 1)
                chosen = moves_begin;
            else
                chosen = find_best_move();
            fflush(stderr);
            printf("%s\n", move_to_str(*chosen));
            fflush(stdout);
        }
        else
        {
            line = read_line();
            if (strcmp(line, "Quit") == 0)
                return 0;

            for (chosen = moves_begin; chosen != moves_end; ++chosen)
                if (strcmp(move_to_str(*chosen), line) == 0)
                    break;
            assert(chosen != moves_end);
        }
        do_move(*chosen);
    }

    return 0;
}
#endif
