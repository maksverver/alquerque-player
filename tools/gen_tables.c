#include <stdio.h>

int main()
{
    int r1, c1, r2, c2, d;
    int dr[8] = { +1, -1,  0,  0, +1, +1, -1, -1 },
        dc[8] = {  0,  0, +1, -1, +1, -1, +1, -1 };

    printf("int moves_normal[49][9] = {\n");
    for (r1 = 0; r1 < 7; ++r1)
        for (c1 = 0; c1 < 7; ++c1)
        {
            printf("    { ");
            for (d = 0; d < ((r1 + c1)%2 ? 4 : 8); ++d)
            {
                r2 = r1 + dr[d];
                c2 = c1 + dc[d];
                if (r2 >= 0 && r2 < 7 && c2 >= 0 && c2 < 7)
                    printf("%2d, ", 7*r2 + c2);
            }
            printf(" -1 }%s\n", (7*r1 + c1 + 1 < 49) ? "," : " };");
        }

    printf("int moves_jumps[49][9] = {\n");
    for (r1 = 0; r1 < 7; ++r1)
        for (c1 = 0; c1 < 7; ++c1)
        {
            printf("    { ");
            for (d = 0; d < ((r1 + c1)%2 ? 4 : 8); ++d)
            {
                r2 = r1 + 2*dr[d];
                c2 = c1 + 2*dc[d];
                if (r2 >= 0 && r2 < 7 && c2 >= 0 && c2 < 7)
                    printf("%2d, ", 7*r2 + c2);
            }
            printf(" -1 }%s\n", (7*r1 + c1 + 1 < 49) ? "," : " };");
        }

    return 0;
}
