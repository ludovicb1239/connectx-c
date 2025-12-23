#include "connectx.h"

static char connectx_check_winner_pos(const connectx_board_t board, int x, int y){
    /*
     * Check for a winner starting from position (x, y).
     * This function checks in all 4 directions (horizontal, vertical,
     * diagonal down-right, diagonal up-right) for a connect of CONNECTX_TO_WIN.
     */
    const int W = CONNECTX_WIDTH;
    const int H = CONNECTX_HEIGHT;
    const int T = CONNECTX_TO_WIN;
    const int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {1, -1}};

    char p = board[x][y];
    if (p == 0) {
        return 0;
    }

    for (int d = 0; d < 4; ++d) {
        int dx = dirs[d][0];
        int dy = dirs[d][1];

        int endx = x + (T - 1) * dx;
        int endy = y + (T - 1) * dy;
        if (endx < 0 || endx >= W || endy < 0 || endy >= H) {
            continue;
        }

        int k;
        for (k = 1; k < T; ++k) {
            if (board[x + k * dx][y + k * dy] != p) {
                break;
            }
        }

        if (k == T) {
            return p;
        }
    }

    return 0;
}

static char connectx_check_winner(const connectx_board_t board) {
    /*
     * Generic, fast check for a winner using CONNECTX_TO_WIN.
     * For each non-empty cell, we check the 4 directions (right, down,
     * down-right and up-right). We pre-check bounds so the inner loop
     * doesn't need to test boundaries on each step.
     */
    const int W = CONNECTX_WIDTH;
    const int H = CONNECTX_HEIGHT;
    const int T = CONNECTX_TO_WIN;

    for (int x = 0; x < W; ++x) {
        for (int y = 0; y < H; ++y) {
            char winner = connectx_check_winner_pos(board, x, y);
            if (winner != 0) {
                return winner;
            }
        }
    }

    return 0;
}

// Returns 1 if theres a win in the specified column at the last placed piece, 0 otherwise
char connectx_check_win_idx(const connectx_board_t board, int col){
    /*
     * Check if there is a win in the specified column.
     * This is an optimized version that only checks for a win
     * involving the last played piece in the given column.
     */
    for (int row = CONNECTX_HEIGHT - 1; row >= 0; row--) {
        if (board[col][row] != 0) {
            char winner = connectx_check_winner_pos(board, col, row);
            return winner != 0;
        }
    }
    return 0;
}

int connectx_is_board_full(const connectx_board_t board) {
    // Optimized check for full board
    for (int i = 0; i < CONNECTX_WIDTH; i++) {
        if (connectx_is_column_full(board, i) == 0) {
            return 0;
        }
    }
    return 1;
}

char connectx_check_win_or_draw(const connectx_board_t board) {
    char check = connectx_check_winner(board);
    if (check != 0) {
        return check;
    }

    if (connectx_is_board_full(board) == 0) {
        return 0;
    }

    return CONNECTX_DRAW;
}

int connectx_is_column_full(const connectx_board_t board, int column) {
    for (int i = 0; i < CONNECTX_HEIGHT; i++) {
        if (board[column][i] == 0) {
            return 0;
        }
    }

    return 1;
}

int connectx_update_board(connectx_board_t *board, int column, char player) {
    int result = 0;

    if (column < 0 || column >= CONNECTX_WIDTH) {
        return -1;
    }

    if (connectx_is_column_full(*board, column) == 1) {
        return -1;
    }

    for (int i = CONNECTX_HEIGHT - 1; i >= 0; i--) {
        if ((*board)[column][i] == 0) {
            (*board)[column][i] = player;
            break;
        }
    }

    return result;
}
