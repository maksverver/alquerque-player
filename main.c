#include "time.h"
#include "board.h"
#include "search.h"
#include "log.h"

int line_read = 0;
char line_buf[1024];

__inline__ int parse_pos(char *str)
{
    return 7*('7' - str[1]) + (str[0] - 'a');
}

bitboard_t parse_move(char *str)
{
    bitboard_t move = 0;
    int len = strlen(str);

    if (len < 5 || len%3 != 2)
        return (bitboard_t)-1;

    if (str[2] == '-')
    {
        move = BIT(parse_pos(str)) | BIT(parse_pos(str + 3));
    }
    else
    {
        int f, g;

        f = parse_pos(str);
        move ^= BIT(f);
        while(*(str += 3))
        {
            g = parse_pos(str);
            move |= BIT((f + g)/2);
            f = g;
        }
        move ^= BIT(f);
    }

    return move;
}

char *peek_line()
{
    char *line;

    if (!line_read)
    {
        for (;;)
        {
            line = fgets(line_buf, sizeof(line_buf), stdin);
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

#ifdef TEST1

int main()
{
    int n;
    board_t board = initial_board;

    for (n = 0; n <= 12; ++n)
    {
        search_negamax(&board, n, -999999999, +999999999);
        printf("depth: %4d  branches: %10lld  leaves: %10lld  total:  %10lld\n",
            n, search_branches, search_leaves, search_branches + search_leaves );
    }
    return 0;
}

#elif TEST2


int main()
{
    int n;
    board_t board = initial_board;

    for (n = 0; n <= 16; ++n)
    {
        search_negamax(&board, n, -1, +1);
        printf("depth: %4d  branches: %10lld  leaves: %10lld  total:  %10lld\n",
            n, search_branches, search_leaves, search_branches + search_leaves );
    }
    return 0;
}
#else

int main(int argc, char *argv[])
{
    int current_player = 0, my_player = 0;
    board_t board = initial_board, new_board;
    int moves_played, n;

    if (strcmp(peek_line(), "Start") == 0)
        read_line();
    else
        my_player = 1;

    time_start();

    for (moves_played = 0; moves_played < MAX_GAME_LEN; ++moves_played)
    {
        char *line = "";
        move_t moves[1000], move;
        int moves_size;

        moves_size = generate_moves(moves, &board);

#ifdef PRINTSTATE
        print_board(stderr, &board, current_player);
        info("%d moves generated", moves_size);
#endif
        if (moves_size == 0)
            break;

#ifdef PRINTSTATE
        for (n = 0; n < moves_size; ++n)
            fprintf(stderr, "  %s", move_to_str(&board, &moves[n]));
        fprintf(stderr, "\n");
#endif

        if (current_player == my_player)
        {
            double t = (30.0 - time_used())/25;
            if (t < 0.1)
                t = 0.1;

            /* Select my move */
            info("Value: %d", search_best_move(&board, &move, t));

            /* Write my move */
            fflush(stderr);
            printf("%s\n", move_to_str(&board, &move));
            fflush(stdout);
        }
        else
        {
            time_stop();

            /* Read opponents move */
            line = read_line();
            if (!line)
            {
                info("Unexpected end of input! Quiting.");
                return 1;
            }
            if (strcmp(line, "Quit") == 0)
                return 0;
            move = parse_move(line);

            time_start();
        }

        /* Validate chosen move */
        for (n = 0; n < moves_size; ++n)
            if (move == moves[n])
                break;
        if (n == moves_size)
        {
            info("Invalid move read: %s", line);
            return 1;
        }

        /* Execute chosen move */
        execute(&new_board, &board, &move);
        board = new_board;
        current_player = !current_player;
    }

    return 0;
}
#endif
