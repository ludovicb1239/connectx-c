#include <stdlib.h>
#include "connectx.h"

int connectx_move(const connectx_board_t board, char player) {
    (void)(player);

    int count = 0;
    for (int i = 0; i < connectx_WIDTH; i++) {
        if (connectx_is_column_full(board, i) == 0) {
            count++;
        }
    }

    if (count == 0) {
        return -1;
    }

    int column = rand() % count;
    for (int i = 0; i < connectx_WIDTH; i++) {
        if (connectx_is_column_full(board, i) == 0) {
            if (column == 0) {
                return i;
            }
            column--;
        }
    }

    return -1;
}
