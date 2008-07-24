#include <stdio.h>
/*

    0  1  2  3  4  5  6
    7  8  9 10 11 12 13
   14 15 16 17 18 19 20
   21 22 23 24 25 26 27
   28 29 30 31 32 33 34
   35 36 37 38 39 40 41
   42 43 44 45 46 47 48

*/

typedef long long bitboard_t;
#define BITCOUNT(i) __builtin_popcountll(i)
#define BIT(x) (x >= 0 ? (1LL<<(x)) : 0LL)
#define PERM(n,x) (x >= 0 ? (1LL<<perms[n][x]) : 0LL)

int perms[8][49] = {
    { },
    { },
    { },
    {  42, 35, 28, 21, 14,  7,  0,
       43, 36, 29, 22, 15,  8,  1,
       44, 37, 30, 23, 16,  9,  2,
       45, 38, 31, 24, 17, 10,  3,
       46, 39, 32, 25, 18, 11,  4,
       47, 40, 33, 26, 19, 12,  5,
       48, 41, 34, 27, 20, 13,  6 },
    { },
    { },
    { },
    {   6,  5,  4,  3,  2,  1,  0,
       13, 12, 11, 10,  9,  8,  7,
       20, 19, 18, 17, 16, 15, 14,
       27, 26, 25, 24, 23, 22, 21,
       34, 33, 32, 31, 30, 29, 28,
       41, 40, 39, 38, 37, 36, 35,
       48, 47, 46, 45, 44, 43, 42 },
};

long long counter[10][10];

void calc()
{
    int a, b, c, d, e, f;
    for (a = -1; a < 49; ++a)
    {
    	printf("  %2d%%\r", 2*(a + 1));
	fflush(stdout);

        for (b = -1; b < 49; ++b)
        {
            if (b != -1 && (b <= a))
                continue;
            for (c = -1; c < 49; ++c)
            {
                if (c != -1 && (c <= b))
                    continue;

                for (d = -1; d < 49; ++d)
                {
                    if (d != -1 && (d == a || d == b || d == c))
                        continue;

                    for (e = -1; e < 49; ++e)
		    {
                        if (e != -1 && (e == a || e == b || e == c || e <= d))
                            continue;

                        for (f = -1; f < 49; ++f)
                        {
                            if (f != -1 && (f == a || f == b || f == c || f <= e))
                                continue;

                            {
                                bitboard_t p1 = BIT(a) | BIT(b) | BIT(c),
                                           p2 = BIT(d) | BIT(e) | BIT(f);
                                int n;
                                for (n = 1; n < 8; ++n)
                                {
                                    bitboard_t q1 = PERM(n,a) | PERM(n,b) | PERM(n,c),
                                               q2 = PERM(n,d) | PERM(n,e) | PERM(n,f);
                                    if (q1 < p1 || (q1 == p1 && q2 < p2))
                                        break;
                                }

                                if (n == 8)
                                    ++counter[BITCOUNT(p1)][BITCOUNT(p2)];
                            }

                            if (e == -1)
                                break;
                        }
                        if (d == -1)
                            break;
                    }
                }
                if (b == -1)
                    break;
            }
            if (a == -1)
                break;
        }
    }
}

int main()
{
    int p, n;
    int x, y;

    for (p = 2; p >= 0; --p)
	for (n = 0; n < 49; ++n)
	    perms[p][n] = perms[p + 1][perms[3][n]];
    for (p = 6; p >= 4; --p)
	for (n = 0; n < 49; ++n)
	    perms[p][n] = perms[p - 3][perms[7][n]];
/*
    for (p = 0; p < 8; ++p)
    {
	for (n = 0; n < 49; ++n)
	{
	    printf("%4d", perms[p][n]);
	    if ((n+1)%7 == 0)
		printf("\n");
	}
	printf("\n");
    }
*/

    calc();

    for (x = 0; x <= 3; ++x)
        for (y = 0; y <= 3; ++y)
	    printf("%d vs %d => %8lld\n", x, y, counter[x][y]);

    return 0;
}
