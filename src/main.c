#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "connectx.h"
#include <dlfcn.h>
#define DS_AP_IMPLEMENTATION
#include "ds.h"

typedef int (*move_t)(const connectx_board_t board, char);

move_t move_player1 = NULL;
move_t move_player2 = NULL;

const char *CLEAR_SCREEN_ANSI = "\e[1;1H\e[2J";

void print_result(char result) {
    if (result == connectx_PLAYER1) {
        printf("Player 1 wins!\n");
    } else if (result == connectx_PLAYER2) {
        printf("Player 2 wins!\n");
    } else if (result == connectx_DRAW) {
        printf("It's a draw!\n");
    }
}

char swap_player(char player) {
    return (player == connectx_PLAYER1) ? connectx_PLAYER2 : connectx_PLAYER1;
}

void connectx_display(const connectx_board_t board) {
    fprintf(stdout, "\n");
    for (int i = 0; i < connectx_HEIGHT; i++) {
        for (int j = 0; j < connectx_WIDTH; j++) {
            fprintf(stdout, "+---");
        }
        fprintf(stdout, "+\n");
        for (int j = 0; j < connectx_WIDTH; j++) {
            fprintf(stdout, "| ");
            if (board[j][i] == connectx_PLAYER1) fprintf(stdout, DS_TERMINAL_RED"x"DS_TERMINAL_RESET);
            else if (board[j][i] == connectx_PLAYER2) fprintf(stdout, DS_TERMINAL_BLUE"o"DS_TERMINAL_RESET);
            else fprintf(stdout, " ");
            fprintf(stdout, " ");
        }
        fprintf(stdout, "|\n");
    }
    for (int i = 0; i < connectx_WIDTH; i++) {
        fprintf(stdout, "+---");
    }
    fprintf(stdout, "+\n");
    for (int i = 0; i < connectx_WIDTH; i++) {
        fprintf(stdout, "  %d ", i + 1);
    }
    fprintf(stdout, " \n");
}

void print_board(connectx_board_t board) {
    (void)(system("stty cooked"));
    printf("%s", CLEAR_SCREEN_ANSI);
    connectx_display(board);
    (void)(system("stty raw"));
}

int move_player(connectx_board_t board, char player) {
    if (player == connectx_PLAYER1) {
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
    ds_argparse_parser parser = {0};
    ds_argparse_parser_init(
        &parser,
        "connectx",
        "connect 4 generalized Game",
        "0.1"
    );

    ds_argparse_add_argument(&parser, (ds_argparse_options){
        .short_name = 'a',
        .long_name = "player1",
        .description = "the strategy for player 1, a path to a DLL",
        .type = ARGUMENT_TYPE_POSITIONAL,
        .required = true,
    });

    ds_argparse_add_argument(&parser, (ds_argparse_options){
        .short_name = 'b',
        .long_name = "player2",
        .description = "the strategy for player 2, a path to a DLL",
        .type = ARGUMENT_TYPE_POSITIONAL,
        .required = true,
    });

    DS_UNREACHABLE(ds_argparse_parse(&parser, argc, argv));

    args->player1 = ds_argparse_get_value(&parser, "player1");
    args->player2 = ds_argparse_get_value(&parser, "player2");

    ds_argparse_parser_free(&parser);
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
    char player = connectx_PLAYER1;

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
        if (move == -1 || connectx_update_board(&board, move, player) == -1) {
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
