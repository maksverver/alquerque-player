#ifndef COMMON_H_INCLUDED
#define COMMON_H_INCLUDED

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define MAX_DEPTH       (50)        /* Maximum search depth */
#define MAX_STATES      (50000)     /* Maximum number of states to evaluate */
#define MAX_MOVES       (200)       /* Est. maximum number of moves in one state (FIXME) */
#define MAX_GAME_LEN    (200)       /* Maximum number of moves in a game */

/* Derived from HACKMEM 169 -- works only for the lowest 62 bits of i!
   (The topmost two bits should be cleared.) */
static __inline__ int bitcount(long long i)
{
    register long long tmp = i - ((i >> 1) & 0333333333333333333333LL)
                               - ((i >> 2) & 0111111111111111111111LL);
    return ((tmp + (tmp >> 3)) & 0707070707070707070707LL) % 63;
}

#endif /* ndef COMMON_H_INCLUDED */
