#include <stdio.h>
#include "connectx.h"

int connectx_move(const connectx_board_t board, char player) {
    (void)(player);

    while (1) {
        int column = 0;
        if (scanf("%d", &column) < 0) return -1;
        int index = column - 1;
        if (!(index < 0 || index >= connectx_WIDTH) && connectx_is_column_full(board, index) == 0) {
            return index;
        }

        return -1;
    }

    return -1;
}
