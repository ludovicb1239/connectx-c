#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "connectx.h"

static void zero_board(connectx_board_t board) {
    memset(board, 0, sizeof(connectx_board_t));
}

static void run_horizontal_test(void) {
    connectx_board_t board;
    zero_board(board);
    /* Place three X in the bottom row at columns 0,1,2. Expect move at 3 */
    board[0][CONNECTX_HEIGHT-1] = CONNECTX_PLAYER1;
    board[1][CONNECTX_HEIGHT-1] = CONNECTX_PLAYER1;
    board[2][CONNECTX_HEIGHT-1] = CONNECTX_PLAYER1;

    int mv = connectx_move(board, CONNECTX_PLAYER1);
    if (mv != 3) {
        fprintf(stderr, "Horizontal test failed: expected 3 got %d\n", mv);
        exit(2);
    }
}

static void run_vertical_test(void) {
    connectx_board_t board;
    zero_board(board);
    /* Place three X in column 2 at rows bottom, one above bottom, two above bottom.
     * Expect move at column 2 to complete the vertical connect.
     */
    board[2][CONNECTX_HEIGHT-1] = CONNECTX_PLAYER1;
    board[2][CONNECTX_HEIGHT-2] = CONNECTX_PLAYER1;
    board[2][CONNECTX_HEIGHT-3] = CONNECTX_PLAYER1;

    int mv = connectx_move(board, CONNECTX_PLAYER1);
    if (mv != 2) {
        fprintf(stderr, "Vertical test failed: expected 2 got %d\n", mv);
        exit(2);
    }
}

int main(void) {
    run_horizontal_test();
    run_vertical_test();
    printf("Immediate-win tests passed\n");
    return 0;
}
