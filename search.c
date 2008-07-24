#include "search.h"
#include "time.h"
#include "log.h"
#include <math.h>

/* For alarm */
#include <sys/types.h>
#include <sys/time.h>
#ifndef __USE_POSIX
#define __USE_POSIX
#endif
#include <signal.h>
#include <unistd.h>


#define INF 999999999

#if !(defined TEST1) && !(defined TEST2)
#define COUNT_LEAF()
#define COUNT_BRANCH()
#define COUNT_FORCED()
#define COUNT_LOST()
#else
#define COUNT_LEAF()        do { ++search_leaves;   } while(0)
#define COUNT_BRANCH()      do { ++search_branches; } while(0)
#define COUNT_FORCED()      do { ++search_forced;   } while(0)
#define COUNT_LOST()        do { ++search_lost;     } while(0)
#endif

long long       search_branches,
                search_leaves,
                search_forced,
                search_lost;

static bool     search_abort;
static int      max_depth;
static move_t   movelist[MAX_DEPTH*MAX_MOVES];
static int      values[MAX_DEPTH*MAX_MOVES];

static __inline__ bitboard_t neighbours(bitboard_t bb)
{
    bitboard_t bd = bb&375299968947541LL;  /* even fields; ie. those that move diagonally */
    return (ALLFLD^bb) & ((bb << 7) | (bb << 1) | (bb >> 1) | (bb >> 7) |
                          (bd << 8) | (bd << 6) | (bd >> 6) | (bd >> 8));
}


static __inline__ int evaluate_simple(const board_t *board)
{
    bitboard_t sa = board->stones[0],
               sb = board->stones[1];
    int        xa = bitcount(sa),
               xb = bitcount(sb);

    return 100000*(xa - xb)/(xa + xb);
}

static int evaluate_old(const board_t *board)
{
    bitboard_t sa = board->stones[0],
               sb = board->stones[1];
    int xa = bitcount(sa), xb = bitcount(sb);
    int score = 0;

    /* Value material */
    score += 100000*(xa - xb)/(xa + xb);

    /* Value influence */
    while ((sa|sb) != ALLFLD)
    {
        bitboard_t na = neighbours(sa), nb = neighbours(sb);
        sa |= na, sb |= nb;
        score += bitcount(na&~sb);
        score -= bitcount(nb&~sa);
    }

    return score;
}

/* Defined in tables.h */
extern int moves_normal[49][9];
extern int moves_jumps[49][9];

static double evaluate_influence(const board_t *board)
{
    bitboard_t occ = board->stones[0] | board->stones[1];
    double score = 0;

    int dists[FIELDS], todo[FIELDS], pos, len;
    int f, g, *m;

    for (f = 0; f < FIELDS; ++f)
    {
        if (occ&BIT(f))
            continue;
        /* BFS for dists */
        memset(dists, -1, sizeof(dists));
        pos = 0, len = 1;
        todo[0] = 100*f;
        dists[f] = 0;

        while (pos < len)
        {
            int cur = todo[pos]/100, dist = todo[pos]%100;
            ++pos;
            dists[cur] = dist;
            /* This doesn't seem to help much:
            if (occ&BIT(cur))
                continue;
            */
            for (m = moves_normal[cur]; *m >= 0; ++m)
                if (dists[*m] < 0)
                {
                    dists[*m] = dist + 1;
                    todo[len++] = 100*(*m) + (dist + 1);
                }
        }

        for (g = 0; g < FIELDS; ++g)
        {
            if (!(occ&BIT(g)) || dists[g] < 0)
                continue;

            if (board->stones[0]&BIT(g))
                score += 1.0/dists[g]/dists[g];
            else
                score -= 1.0/dists[g]/dists[g];
        }
    }

    return score;
}

static int cmp_move_values(const void *a, const void *b)
{
    return values[(move_t*)a - movelist] - values[(move_t*)b - movelist];
}

static int negamax(const board_t *board, int depth, move_t *moves, int alpha, int beta)
{
    if (depth == max_depth)
    {
        COUNT_LEAF();
        return evaluate_simple(board);
    }
    else
    {
        int moves_size, n, v;
        board_t new_board;

        COUNT_BRANCH();

        if (search_abort)
            return 0;

        moves_size = generate_moves(moves, board);
        if (moves_size == 0)
            COUNT_LOST();

        if (moves_size == 1)
            COUNT_FORCED();

        for (n = 0; n < moves_size; ++n)
        {
            execute(&new_board, board, &moves[n]);
            v = -negamax(&new_board, depth + 1, moves + moves_size, -beta, -alpha);
            if (v > alpha)
                alpha = v;
#ifndef TEST1
            if (v >= beta)
                return v;
#endif
        }
        return alpha;
    }
}

int search_negamax(const board_t *board, int search_depth, int alpha, int beta)
{
    max_depth = search_depth;
    search_branches = search_leaves = search_forced = search_lost = 0;
    return negamax(board, 0, movelist, alpha, beta);
}


static void sig_handler(int signal)
{
    if (signal == SIGALRM)
    {
        info("Aborting search...");
        search_abort = true;
    }
}


int search_best_move(const board_t *board, move_t *move, double max_time)
{
    board_t new_board;
    int moves_size, value, n;
    double *influence;
    double t0 = time_now(), dt, last_dt = 0;

    info("Searching with max_time=%lf (total used: %lf)", max_time, time_used());

    moves_size = generate_moves(movelist, board);
    if (moves_size == 0)
    {
        *move = 0;
        return -INF;
    }

    if (moves_size == 1)
    {
        *move = movelist[0];
        return 0;
    }

    influence = malloc(sizeof(*influence)*moves_size);
    for (n = 0; n < moves_size; ++n)
    {
        execute(&new_board, board, &movelist[n]);
        influence[n] = -evaluate_influence(&new_board);
    }

    /* Set alarm signal handler */
    {
        struct sigaction sa;
        struct itimerval it;

        memset(&sa, 0, sizeof(sa));
        sa.sa_handler = &sig_handler;
        sigaction(SIGALRM, &sa, NULL);

        memset(&it, 0, sizeof(it));
        it.it_value.tv_sec  = floor(max_time);
        it.it_value.tv_usec = 1000000*(max_time - floor(max_time));
        setitimer(ITIMER_REAL, &it, NULL);
    }

    value = -INF;
    search_abort = false;
    for (max_depth = 4; max_depth < 200 && value < INF; max_depth += 2)
    {
        int alpha = -INF, best_n = 0, n;

        for (n = 0; n < moves_size && !search_abort; ++n)
        {
            int v;
            execute(&new_board, board, &movelist[n]);
            v = -negamax(&new_board, 1, movelist + moves_size, -INF, -alpha + 1);
            if (v > alpha || (v == alpha && influence[n] > influence[best_n]))
            {
                best_n = n;
                alpha  = v;
            }
            if (v >= INF)
                break;      /* quickest win */
        }
        if (search_abort)
            break;
        *move = movelist[best_n];
        value = alpha;
        info("Depth: %d  Value: %d  Move: %s",
            max_depth, alpha, move_to_str(board, move));

        /* Estimate if we can do another pass */
        dt = time_now() - t0;
        info("Estimate for next pass: %lf", dt*dt/last_dt);
        if (last_dt > 0.001 && dt*dt/last_dt > max_time)
            break;
        last_dt = dt;
    }

    {
        struct itimerval it;
        memset(&it, 0, sizeof(it));
        setitimer(ITIMER_REAL, &it, NULL);
    }

    free(influence);

    return value;
}

