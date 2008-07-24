#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/time.h>
#include <unistd.h>

/* TODO:
    - more sensible evaluation function?
    (- do not increase depth for forced moves?)

    IF NECESSARY:
    - ensure time limit is not exceeded?

    LONG TERM:
    - move re-ordering?
    - transposition tables?
    - defeat horizon effect?
*/

/* Parameters */

/* Macro definitions */
#define VALID(r,c) ((r) >= 0 && (r) < 7 && (c) >= 0 && (c) < 7)
#define POS(r,c) (7*(r)+(c))
#define MOVE(f,g) ((f<<8)|g)
#define OTHER(col) (1 - (col))

#define EMPTY (-1)
#define WHITE ( 0)
#define BLACK ( 1)

#define FIELDS (49)

#define INF (100000000)
#define NO_VALUE (0x80000000)

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

/* The initial game state */
char fld[FIELDS] = {
    /* since the row 0 is the bottom row, this looks upside down */
    WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
    WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
    WHITE, WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
    BLACK, BLACK, BLACK, EMPTY, WHITE, WHITE, WHITE,
    BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
    BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK,
    BLACK, BLACK, BLACK, BLACK, BLACK, BLACK, BLACK };
int cur = WHITE;
int last[2] = { -1, -1 };
int cnt[2] = { 24, 24 };
int played = 0;

char field_str[FIELDS][4];

char move_buf[80], best_move_buf[80], last_best_move_buf[80];

double time_left = 30;
int num_moves, best_value, max_depth;

int *adjacent[FIELDS];
int *adjacent_capture[FIELDS];

/* Function prototypes */
double now();
void dump(FILE *fp);
void initialize();
int search(int depth, int alpha, int beta);
int search_capture(int from, int depth, int alpha, int beta);
int evaluate(int depth, int alpha, int beta);
void execute(char *s);


__inline__ int min(int a, int b)    {   return a < b ? a : b;   }
__inline__ int max(int a, int b)    {   return a > b ? a : b;   }
__inline__ int abs(int a)           {   return a < 0 ? -a : +a; }

double now()
{
    struct timeval tv;

    gettimeofday(&tv, 0);
    return tv.tv_sec + tv.tv_usec/1e6;
}


void dump(FILE *fp)
{
    int r, c, f;

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
            f = POS(6 - r, c);
            fprintf(fp, "%c%s%s%c%s",
                c > 0 ? '-' : ' ',
                BOLD,
                fld[f] == WHITE ? RED : fld[f] == BLACK ? BLUE : NORMAL,
                fld[f] == WHITE ? 'X' : fld[f] == BLACK ? 'O' : ' ',
                NORMAL );
        }
        fprintf(fp, "\n");
    }
    fprintf(fp, "\n  %s", BOLD);
    for (c = 0; c < 7; ++c)
        fprintf(fp, " %c", 'a' + c);
    fprintf(fp, "%s\n\n", NORMAL);
}

void initialize()
{
    static int a_buf[7*FIELDS], *a = a_buf;
    static int ac_buf[7*FIELDS], *ac = ac_buf;
    static const int dr[8] = { -1,  0,  0, +1, -1, -1, +1, +1 };
    static const int dc[8] = {  0, -1, +1,  0, -1, +1, -1, +1 };

    int r1, c1, r2, c2, d;

    for (r1 = 0; r1 < 7; ++r1)
    {
        for (c1 = 0; c1 < 7; ++c1)
        {
            sprintf(field_str[POS(r1,c1)], "%c%c", 'a' + c1, '1' + r1);
            adjacent[POS(r1,c1)] = a;
            adjacent_capture[POS(r1,c1)] = ac;
            for (d = 0; d < ((POS(r1,c1)&1) ? 4 : 8); ++d)
            {
                r2 = r1 + dr[d];
                c2 = c1 + dc[d];
                if (VALID(r2, c2))
                {
                    *a++ = POS(r2,c2);
                    if (VALID(r2 + dr[d], c2 + dc[d]))
                        *ac++ = POS(r2,c2);
                }
            }
            *a++  = -1;
            *ac++ = -1;
        }
    }
}

#if 0
int pos_value()
{
    int res = 0, n, a = 0, b = 0;
    for (n = 0; n < FIELDS; ++n)
    {
        if (fld[n] == WHITE)
            ++a, res += n/7;
        else
        if (fld[n] == BLACK)
            ++b, res -= 6 - n/7;
    }
    /* +1/-1 is to favor capturing */
    return 10000*(((cur == WHITE) ? +1 : -1) + a - b)/(a + b) + res;
}
#endif

int material_value()
{
    int diff = cnt[cur] - cnt[OTHER(cur)], sum = cnt[0] + cnt[1];;
    return 100000*(2*diff + 1)/sum;
}

int pos_value()
{
    int n, val = 0;
    for (n = 0; n < FIELDS; ++n)
        switch(fld[n])
        {
        case WHITE:
            val += n/7;
        case BLACK:
            val -= 6 - n/7;
        }
    return (cur == WHITE) ? val : -val;
}


int pos_fancy_value()
{
    int r, c, val = 0, x[7];

    for (c = 0; c < 7; ++c)
    {
        x[c] = 0;
        for (r = 0; r < 7; ++r)
            switch(fld[7*r + c])
            {
            case BLACK:
                break;
            case WHITE:
                x[c] = 1 + r;
            }
    }
    for (c = 1; c < 7; ++c)
    {
        if (x[c] > x[c - 1] + 1)
            x[c] = x[c - 1] + 1;
        if (x[c] < x[c - 1] - 1)
            x[c] = x[c - 1] - 1;
        val += x[c];
    }
    for (c = 0; c < 7; ++c)
    {
        x[c] = 7;
        for (r = 6; r >= 0; --r)
            switch(fld[7*r + c])
            {
            case WHITE:
                break;
            case BLACK:
                x[c] = r;
            }
    }
    for (c = 1; c < 7; ++c)
    {
        if (x[c] > x[c - 1] + 1)
            x[c] = x[c - 1] + 1;
        if (x[c] < x[c - 1] - 1)
            x[c] = x[c - 1] - 1;
        val -= 7 - x[c];
    }
    return cur == WHITE ? val : -val;
}

int evaluate(int depth, int alpha, int beta)
{
    int value;

    /* assert(alpha < beta); */

    if (depth < max_depth)
        value = search(depth, alpha, beta);
    else
        value = material_value() + pos_value();

    if (depth == 1)
    {
        ++num_moves;
        fprintf(stderr, "%d [%s] => %d\n", num_moves, move_buf, -value);
        if (best_value == NO_VALUE || -value > best_value ||
            (-value == best_value && strcmp(move_buf, last_best_move_buf) == 0))
        {
            best_value = -value;
            strcpy(best_move_buf, move_buf);
        }
    }

    return value;
}

int search_capture(int from, int depth, int alpha, int beta)
{
    int *adj, to, v;
    bool found = false;

    if (depth == 0)
    {
        strcat(move_buf, "*");
        strcat(move_buf, field_str[from]);
    }

    for (adj = adjacent_capture[from]; *adj >= 0; ++adj)
        if (fld[*adj] == OTHER(cur) && fld[to = from + 2*(*adj - from)] == EMPTY)
        {
            fld[*adj] = EMPTY;
            --cnt[OTHER(cur)];
            v = search_capture(to, depth, alpha, beta);
            ++cnt[OTHER(cur)];
            fld[*adj] = OTHER(cur);
            if (v >= beta)
                return v;
            else
            if (v > alpha)
                alpha = v;
            found = true;
        }

    if (!found)
    {
        fld[from] = cur;
        cur = OTHER(cur);
        alpha = -evaluate(depth + 1, -beta, -alpha);
        fld[from] = EMPTY;
        cur = OTHER(cur);
    }

    if (depth == 0)
    {
        move_buf[strlen(move_buf) - 3] = '\0';
    }

    return alpha;
}

int search(int depth, int alpha, int beta)
{
    bool found = false;
    int from, to, *adj, v, bak;

    /* Search for capturing moves */
    bak = last[cur];
    last[cur] = -1;
    for (from = 0; from < FIELDS; ++from)
        if (fld[from] == cur)
        {
            for (adj = adjacent_capture[from]; *adj >= 0; ++adj)
                if (fld[*adj] == OTHER(cur) && fld[to = from + 2*(*adj - from)] == EMPTY)
                {
                    if (depth == 0)
                        strcpy(move_buf, field_str[from]);
                    fld[from] = fld[*adj] = EMPTY;
                    --cnt[OTHER(cur)];
                    v = search_capture(to, depth, alpha, beta);
                    ++cnt[OTHER(cur)];
                    fld[from] = cur;
                    fld[*adj] = OTHER(cur);
                    if (v >= beta)
                        return v;
                    else
                    if (v > alpha)
                        alpha = v;
                    found = true;
                }
        }
    last[cur] = bak;

    if (found)
        return alpha;

    /* Search for non-capturing moves */
    /* FIXME: cache fields occupied by `cur' and use them here? */
    for (from = 0; from < FIELDS; ++from)
        if (fld[from] == cur)
        {
            for (adj = adjacent[from]; *adj >= 0; ++adj)
                if (fld[to = *adj] == EMPTY)
                {
                    if (bak == MOVE(to,from))
                        continue;

                    if (depth == 0)
                    {
                        strcpy(move_buf, field_str[from]);
                        strcat(move_buf, "-");
                        strcat(move_buf, field_str[to]);
                    }

                    fld[from] = EMPTY;
                    fld[to]   = cur;
                    last[cur] = MOVE(from,to);
                    cur = OTHER(cur);
                    v = -evaluate(depth + 1, -beta, -alpha);
                    cur = OTHER(cur);
                    fld[from] = cur;
                    fld[to]   = EMPTY;
                    last[cur] = bak;
                    if (v >= beta)
                        return v;
                    else
                    if (v > alpha)
                        alpha = v;
                }
        }

    return alpha;
}

void execute(char *s)
{
    int f, g;

    f = POS(s[1] - '1', s[0] - 'a');
    s += 2;
    while (*s) {
        g = POS(s[2] - '1', s[1] - 'a');
        fld[f] = EMPTY;
        if (s[0] == '*')
        {
            --cnt[fld[(f + g)/2]];
            fld[(f + g)/2] = EMPTY;
        }
        last[cur] = MOVE(f,g);
        f = g;
        s += 3;
    }
    fld[f] = cur;
    cur = OTHER(cur);
}

void select_move()
{
    double t0, dt, last_dt = 0;
    max_depth = 2;

    t0 = now();
    *last_best_move_buf = *best_move_buf = '\0';
    for (;;)
    {
        best_value = NO_VALUE;
        num_moves = 0;
        evaluate(0, -INF, +INF);
        if (num_moves < 2)
            break;
        dt = now() - t0;
        fprintf(stderr, "%d plies took %.3fs\n", max_depth, dt - last_dt);
        strcpy(last_best_move_buf, best_move_buf);
        if (last_dt > 0.001 && dt*dt/last_dt > time_left/25)
            break;
        last_dt = dt;
        max_depth += 2;
    }

    time_left -= now() - t0 + 0.005;
}

static int opt_time_test, opt_verbose;

void parse_options(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "vtT:")) != -1)
        switch (opt)
        {
        case 't': opt_time_test = 1; break;
        case 'v': opt_verbose   = 1; break;
        case 'T':
            time_left = atof(optarg);
            fprintf(stderr, "Total time set to %.3fs\n", time_left);
            break;
        }
}

int main(int argc, char *argv[])
{
    parse_options(argc, argv);
    initialize();

    if (opt_time_test)
    {
        int v;
        double t0 = now();
        max_depth = 16;
        best_value = NO_VALUE;
        num_moves = 0;
        v = evaluate(0, -INF, +INF);
        printf("max_depth=%d  num_moves=%d  v=%d  best_move=%s\n",
            max_depth, num_moves, v, best_move_buf);
        printf("Execution time: %.3f (CPU: %.3f)\n", now() - t0, clock()/1e6);
        return 0;
    }

    while (played < 100)
    {
        if (!fgets(move_buf, sizeof(move_buf), stdin))
        {
            fprintf(stderr, "ERROR: premature end of input!\n");
            return 1;
        }
        if (strchr(move_buf, '\n'))
            *strchr(move_buf, '\n') = '\0';

        /* Execute opponent's move */
        if (strcmp(move_buf, "Start") != 0)
        {
            execute(move_buf);
            if (opt_verbose)
                dump(stderr);
        }

        /* Pick my move */
        select_move();
        if (best_value == NO_VALUE)
        {
            fprintf(stderr, "No moves available! Exiting.\n");
            break;
        }
        fprintf(stderr, "Best move: [%s]; value: %d\n", best_move_buf, best_value);
        fflush(stderr);
        fprintf(stdout, "%s\n", best_move_buf);
        fflush(stdout);
        execute(best_move_buf);
        if (opt_verbose)
            dump(stderr);
    }
    return 0;
}
