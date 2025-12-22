#define DS_NO_STDLIB
#define DS_DA_INIT_CAPACITY 16
#define DS_LIST_ALLOCATOR_IMPLEMENTATION
#define DS_DA_IMPLEMENTATION
#define DS_NO_TERMINAL_COLORS
#include "ds.h"
#include "wasm.h"
#include "connectx.h"

DS_ALLOCATOR allocator;
connectx_board_t board;
char player = connectx_PLAYER1;

void print_board(connectx_board_t board) {
    int width = js_width();
    int height = js_height();

    int cell_width = width / connectx_WIDTH;
    int cell_height = height / connectx_HEIGHT;

    js_clear_canvas();
    for (unsigned int col = 0; col < connectx_WIDTH; col++) {
        for (unsigned int row = 0; row < connectx_HEIGHT; row++) {
            int x = col * cell_width;
            int y = row * cell_height;

            js_draw_rect(x, y, cell_width, cell_height, 0xA0A0A0);
            if (board[col][row] == connectx_PLAYER1) {
                js_draw_circle(x + cell_width / 2, y + cell_height / 2, cell_width / 2, 0xFF0000);
            } else if (board[col][row] == connectx_PLAYER2) {
                js_draw_circle(x + cell_width / 2, y + cell_height / 2, cell_width / 2, 0x0000FF);
            } else {
                js_draw_circle(x + cell_width / 2, y + cell_height / 2, cell_width / 2, 0x181818);
            }
        }
    }
}

char swap_player(char player) {
    return (player == connectx_PLAYER1) ? connectx_PLAYER2 : connectx_PLAYER1;
}

void init(void *memory, unsigned long size) {
    DS_INIT_ALLOCATOR(&allocator, memory, size);

    print_board(board);
}

void tick(float deltaTime) {
    int result = 0;
    int move = -1;

    if (player == connectx_PLAYER1) {
        int x = js_canvas_clicked_x();
        if (x == -1) return;

        int cell_width = js_width() / connectx_WIDTH;
        move = x / cell_width;
    } else {
        move = connectx_move(board, player);
    }

    if (connectx_update_board(&board, move, player) == -1) {
        js_log_message("Invalid move for player.", 24);
        result = swap_player(player);
        goto result;
    }

    print_board(board);
    result = connectx_check_win_or_draw(board);
    if (result != 0) {
        goto result;
    }

    player = swap_player(player);
    return;

result:
    if (result == connectx_PLAYER1) {
        js_log_message("Player 1 wins!", 14);
    } else if (result == connectx_PLAYER2) {
        js_log_message("Player 2 wins!", 14);
    } else if (result == connectx_DRAW) {
        js_log_message("It's a draw!", 12);
    }

    for (int i = 0; i < connectx_WIDTH; i++) {
        for (int j = 0; j < connectx_HEIGHT; j++) {
            board[i][j] = 0;
        }
    }
    player = connectx_PLAYER1;
    print_board(board);
}
