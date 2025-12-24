#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "connectx.h"
#include <dlfcn.h>
#define DS_AP_IMPLEMENTATION
#define TERMINAL_RED "\033[31m"
#define TERMINAL_YELLOW "\033[33m"
#define TERMINAL_RESET "\033[0m"

typedef int (*move_t)(const connectx_board_t board, char);

move_t move_player1 = NULL;
move_t move_player2 = NULL;

void print_result(char result) {
    if (result == CONNECTX_PLAYER1) {
        printf("Player 1 wins!\n");
    } else if (result == CONNECTX_PLAYER2) {
        printf("Player 2 wins!\n");
    } else if (result == CONNECTX_DRAW) {
        printf("It's a draw!\n");
    }
}

char swap_player(char player) {
    return (player == CONNECTX_PLAYER1) ? CONNECTX_PLAYER2 : CONNECTX_PLAYER1;
}

void connectx_display(const connectx_board_t board) {
    fprintf(stdout, "\n");
    for (int i = 0; i < CONNECTX_HEIGHT; i++) {
        for (int j = 0; j < CONNECTX_WIDTH; j++) {
            fprintf(stdout, "+---");
        }
        fprintf(stdout, "+\n");
        for (int j = 0; j < CONNECTX_WIDTH; j++) {
            fprintf(stdout, "| ");
            if (board[j][i] == CONNECTX_PLAYER1) fprintf(stdout, TERMINAL_RED"🅧"TERMINAL_RESET);
            else if (board[j][i] == CONNECTX_PLAYER2) fprintf(stdout, TERMINAL_YELLOW"🅞"TERMINAL_RESET);
            else fprintf(stdout, " ");
            fprintf(stdout, " ");
        }
        fprintf(stdout, "|\n");
    }
    for (int i = 0; i < CONNECTX_WIDTH; i++) {
        fprintf(stdout, "+---");
    }
    fprintf(stdout, "+\n");
    for (int i = 0; i < CONNECTX_WIDTH; i++) {
        fprintf(stdout, "  %d ", i + 1);
    }
    fprintf(stdout, " \n");
}

void print_board(connectx_board_t board) {
    connectx_display(board);
}

int move_player(connectx_board_t board, char player) {
    if (player == CONNECTX_PLAYER1) {
        return move_player1(board, player);
    } else {
        return move_player2(board, player);
    }
}

typedef struct arguments {
    char *player1;
    char *player2;
} arguments;

void parse_arguments(int argc, char **argv, arguments *args) {
    /* Simple positional parser: expect exactly 2 positional args
       Usage: connectx <player1.so> <player2.so> */
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <player1.so> <player2.so>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    args->player1 = argv[1];
    args->player2 = argv[2];
}

int load_move_function(arguments args) {
    void *strat1 = dlopen(args.player1, RTLD_NOW);
    if (strat1 == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    move_player1 = (move_t)dlsym(strat1, "connectx_move");
    if (move_player1 == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    void *strat2 = dlopen(args.player2, RTLD_NOW);
    if (strat2 == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    move_player2 = (move_t)dlsym(strat2, "connectx_move");
    if (move_player2 == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {
    int move = 0;
    char result = 0;
    char player = CONNECTX_PLAYER1;

    srand(time(NULL));

    arguments args = {0};
    parse_arguments(argc, argv, &args);
    if (load_move_function(args) != 0) {
        fprintf(stderr, "Failed to load move functions.\n");
        return -1;
    }

    connectx_board_t board = {0};
    print_board(board);

    while (1) {
        move = move_player(board, player);
        if (move == -1 || connectx_update_board(board, move, player) == -1) {
            printf("Invalid move for player %d (%d).\n", player, move);
            result = swap_player(player);
            break;
        }
        print_board(board);
        result = connectx_check_win_or_draw(board);
        if (result != 0) {
            break;
        }
        player = swap_player(player);
    }
    print_result(result);

    return 0;
}
