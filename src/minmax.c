#include "connectx.h"
#include <stdlib.h>
#include <time.h>
#ifdef _OPENMP
#include <omp.h>
#endif
#include <stdint.h>
#include <string.h>

#ifndef MINMAX_DEPTH
#define MINMAX_DEPTH 11
#endif

#define MINMAX_INF 1000000

typedef struct move_score {
    int move;
    int score;
} move_score;

static move_score minmax(const connectx_board_t board, char player, char maxxing, int depth, int alpha, int beta);
static int eval(const connectx_board_t board, char player);

static char swap_player(char player);

int connectx_move(const connectx_board_t board, char player) {
    const int W = CONNECTX_WIDTH;

    /* Scores for each root move (one per column). Initialize to very low.
     * We will run each legal root move search in parallel.
     */
    int scores[CONNECTX_WIDTH];
    for (int i = 0; i < W; ++i) scores[i] = -MINMAX_INF - 1;

    /* Quick serial scan: if any root move immediately wins, return it right away.
     * This saves us from launching the parallel minmax searches when a trivial
     * winning move exists.
     */
    for (int i = 0; i < W; ++i) {
        if (connectx_is_column_full(board, i) == 0) {
            connectx_board_t copy = {0};
            memcpy(copy, board, sizeof(connectx_board_t));
            if (connectx_update_board(&copy, i, player) == 0 && connectx_check_win_idx(copy, i)) {
                return i;
            }
        }
    }

#pragma omp parallel for schedule(dynamic)
    for (int i = 0; i < W; ++i) {
        if (connectx_is_column_full(board, i) == 0) {
            connectx_board_t copy = {0};
            /* Copy the board using memcpy */
            memcpy(copy, board, sizeof(connectx_board_t));

            if (connectx_update_board(&copy, i, player) == 0) {
                /* If this root move immediately wins, record a maximal score. */
                if (connectx_check_win_idx(copy, i)) {
                    scores[i] = MINMAX_INF; /* root `maxxing` is `player` */
                } else if (connectx_is_board_full(copy)) {
                    scores[i] = 0;
                } else {
                    move_score mv = minmax(copy, swap_player(player), player,
                                           MINMAX_DEPTH - 1, -MINMAX_INF, MINMAX_INF);
                    scores[i] = mv.score;
                }
            }
        }
    }

    /* Choose the best score (maximizing for 'player'). Tie-break by lowest column. */
    int best = -1;
    int best_score = -MINMAX_INF - 1;
    for (int i = 0; i < W; ++i) {
        if (scores[i] > best_score) {
            best_score = scores[i];
            best = i;
        }
    }

    return best;
}

static char swap_player(char player) {
    return (player == CONNECTX_PLAYER1) ? CONNECTX_PLAYER2 : CONNECTX_PLAYER1;
}

static int eval(const connectx_board_t board, char player) {
    /* Dynamically generate a weights matrix that favors the center of the board.
     * The formula scales with CONNECTX_WIDTH / CONNECTX_HEIGHT and keeps
     * higher values near the central columns/rows similar to the original
     * hard-coded table.
     */
    connectx_board_t weights = {0};
    const int W = CONNECTX_WIDTH;
    const int H = CONNECTX_HEIGHT;
    const double cx = (W - 1) / 2.0;
    const double cy = (H - 1) / 2.0;

    for (int x = 0; x < W; ++x) {
        for (int y = 0; y < H; ++y) {
            double col_score = cx - (x > cx ? x - cx : cx - x);
            double row_score = cy - (y > cy ? y - cy : cy - y);

            int col_weight = (int)(col_score * 2.0 + 0.5); // columns matter more
            int row_weight = (int)(row_score + 0.5);

            int w = 1 + col_weight + row_weight;
            if (w < 1) w = 1;
            if (w > 127) w = 127; // keep inside signed char range

            weights[x][y] = (char)w;
        }
    }

    int score_1 = 0;
    int score_2 = 0;
    for (int j = 0; j < CONNECTX_HEIGHT; j++) {
        for (int k = 0; k < CONNECTX_WIDTH; k++) {
            score_1 += weights[k][j] * (board[k][j] == CONNECTX_PLAYER1);
            score_2 += weights[k][j] * (board[k][j] == CONNECTX_PLAYER2);
        }
    }

    return (player == CONNECTX_PLAYER1) ? score_1 - score_2 : score_2 - score_1;
}

int random_move(const connectx_board_t board) {
    /* Thread-local seed and rand_r for thread-safety when using OpenMP */
    static _Thread_local unsigned int seed = 0;
    if (seed == 0) seed = (unsigned int)((uintptr_t)&seed ^ (unsigned int)time(NULL));

    int legal[CONNECTX_WIDTH];
    int count = 0;
    for (int i = 0; i < CONNECTX_WIDTH; i++) {
        if (connectx_is_column_full(board, i) == 0) {
            legal[count++] = i;
        }
    }

    if (count == 0) return -1;
    return legal[rand_r(&seed) % count];
}

static move_score minmax(const connectx_board_t board, char player, char maxxing, int depth, int alpha, int beta) {
    /*
     * NOTE: We avoid scanning the whole board for a win at function entry.
     * Instead, we check for a win immediately after applying a move (see loop
     * below) using `connectx_check_win_idx`, which inspects only the last
     * placed piece in the column.
     */

    if (depth == 0) {
        move_score value = {.score = 0, .move = -1};
        value.score = eval(board, player);
        return value;
    }

    move_score best = {.score = 0, .move = -1};
    if (maxxing == player) best.score = -MINMAX_INF;
    else best.score = MINMAX_INF;
    best.move = random_move(board); // default move if all else fails

    for (int i = 0; i < CONNECTX_WIDTH; i++) {
        if (connectx_is_column_full(board, i) == 0) {
            connectx_board_t copy = {0};
            for (int j = 0; j < CONNECTX_HEIGHT; j++) {
                for (int k = 0; k < CONNECTX_WIDTH; k++) {
                    copy[k][j] = board[k][j];
                }
            }
            // Make the move and recurse
            if (connectx_update_board(&copy, i, player) == -1) {
                break;
            }

            /* Check for an immediate terminal state caused by this move. */
            move_score value = {.score = 0, .move = -1};
            if (connectx_check_win_idx(copy, i)) {
                /* The player who just moved is `player` and caused a win. */
                int good = (maxxing == player);
                value.score = good ? MINMAX_INF : -MINMAX_INF;
                value.move = i;
            } else {
                /* If the board is full it's a draw. Otherwise recurse. */
                if (connectx_is_board_full(copy)) {
                    value.score = 0;
                    value.move = i;
                } else {
                    value = minmax(copy, swap_player(player), maxxing, depth - 1, alpha, beta);
                }
            }

            if (maxxing == player) {
                if (value.score > best.score) {
                    best.score = value.score;
                    best.move = i;
                }

                if (value.score > alpha) alpha = value.score;
            } else {
                if (value.score < best.score) {
                    best.score = value.score;
                    best.move = i;
                }

                if (value.score < beta) beta = value.score;
            }

            if (alpha > beta) break;
        }
    }

    return best;
}
