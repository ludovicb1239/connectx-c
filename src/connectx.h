#ifndef connectx_H
#define connectx_H

#define connectx_WIDTH 7
#define connectx_HEIGHT 6

#define connectx_PLAYER1 1
#define connectx_PLAYER2 2
#define connectx_DRAW 3

typedef char connectx_board_t[connectx_WIDTH][connectx_HEIGHT];

char connectx_check_win_or_draw(const connectx_board_t board);
int connectx_is_column_full(const connectx_board_t board, int column);
int connectx_update_board(connectx_board_t *board, int column, char player);

// This function is used to get the next move for the player.
// This can return -1 if there is no valid move or the AI sucks.
int connectx_move(const connectx_board_t board, char player);

#endif // connectx_H
