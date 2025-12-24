#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <png.h>
#include "connectx.h"

const int pos_top_left_x = 24;
const int pos_bot_right_x = 1055;

#if CONNECT_X == 4
const int pos_top_left_y = 519;
const int pos_bot_right_y = 1405;
#elif CONNECT_X == 5
const int pos_top_left_y = 561;
const int pos_bot_right_y = 1362;
#endif

const int width = CONNECTX_WIDTH;
const int height = CONNECTX_HEIGHT;
const int cell_w = (pos_bot_right_x - pos_top_left_x) / width;
const int cell_h = (pos_bot_right_y - pos_top_left_y) / height;

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

move_t move_player = NULL;

void sleep(int millis) {
    struct timespec req = {0};
    req.tv_sec = millis / 1000;
    req.tv_nsec = (millis % 1000) * 1000000L;
    nanosleep(&req, NULL);
}

// Helper: capture screen from device
void capture_screen(const char *filename) {
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "adb exec-out screencap -p > %s", filename);
    system(cmd);
}

// Example: send a tap
void tap(int x, int y) {
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "adb shell input tap %d %d", x, y);
    system(cmd);
}

// Optional: read a pixel from a PNG file
int read_pixel(const char *filename, int px, int py, unsigned char *r, unsigned char *g, unsigned char *b) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) return 0;

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    if (setjmp(png_jmpbuf(png))) return 0;

    png_init_io(png, fp);
    png_read_info(png, info);

    int width = png_get_image_width(png, info);
    int height = png_get_image_height(png, info);
    png_bytep *row_pointers = malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++) {
        row_pointers[y] = malloc(png_get_rowbytes(png, info));
    }

    png_read_image(png, row_pointers);

    if (px >= width || py >= height) return 0;

    png_bytep px_ptr = &(row_pointers[py][px * 4]); // RGBA
    *r = px_ptr[0];
    *g = px_ptr[1];
    *b = px_ptr[2];

    for (int y = 0; y < height; y++) free(row_pointers[y]);
    free(row_pointers);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    return 1;
}

// Return whos turn it is
int read_board(connectx_board_t board, char* player) {
    const int pos_player_indicator_x = 791;
    const int pos_player_indicator_y = 95;
    
    // Capture
    const char *screenshot = "screen.png";
    capture_screen(screenshot);
    printf("Captured screen.png\n");

    {
        unsigned char r, g, b;
        read_pixel(screenshot, pos_player_indicator_x, pos_player_indicator_y, &r, &g, &b);
        if (g > 80){ // Yellowish
            *player = CONNECTX_PLAYER2;
        } else {
            *player = CONNECTX_PLAYER1;
        }
    }
    int red_count = 0;
    int yellow_count = 0;

    // Read each pixel of the grid, store the hue value
    for (int x = 0; x < CONNECTX_WIDTH; x++) {
        for (int y = 0; y < CONNECTX_HEIGHT; y++) {
            int px = pos_top_left_x + x * cell_w + cell_w / 2;
            int py = pos_top_left_y + y * cell_h + cell_h / 2;
            unsigned char r, g, b;
            if (!read_pixel(screenshot, px, py, &r, &g, &b)) {
                printf("Failed to read pixel at (%d,%d)\n", px, py);
                return 0;
            }
            // Background
            if (r == 37 && g == 34 && b == 51) { // RGB 37 34 51
                board[x][y] = CONNECTX_EMPTY;
            } else if (*player == CONNECTX_PLAYER1 && r > 150 && g < 100 && b < 100) { // Red
                board[x][y] = CONNECTX_PLAYER1;
                red_count++;
            } else if (*player == CONNECTX_PLAYER2 && r > 150 && g > 150 && b < 100) { // Yellow
                board[x][y] = CONNECTX_PLAYER2;
                yellow_count++;
            } else {
                board[x][y] = *player == CONNECTX_PLAYER1 ? CONNECTX_PLAYER2 : CONNECTX_PLAYER1; // Opponent
                red_count += (*player == CONNECTX_PLAYER2);
                yellow_count += (*player == CONNECTX_PLAYER1);
            }
        }
    }
    print_board(board);
    // check if board state is valid
    for (int x = 0; x < CONNECTX_WIDTH; x++) {
        int found_empty = 0;
        for (int y = CONNECTX_HEIGHT - 1; y >= 0; y--) {
            if (board[x][y] == CONNECTX_EMPTY) {
                found_empty = 1;
            } else if (found_empty) {
                printf("Invalid board state: floating piece at column %d, row %d\n", x, y);
                return -1;
            }
        }
    }

    if (red_count == yellow_count) {
        return CONNECTX_PLAYER1;
    } else if (red_count == yellow_count + 1) {
        return CONNECTX_PLAYER2;
    }
    printf("Invalid board state: red=%d yellow=%d\n", red_count, yellow_count);
    return -1;
}

int play_move_board(connectx_board_t board, int column) {
    int x = pos_top_left_x + column * cell_w + cell_w / 2;
    int y = pos_top_left_y + 50;
    tap(x, y);
    printf("Tapped at (%d,%d)\n", x, y);
}




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
            if (board[j][i] == CONNECTX_PLAYER1) fprintf(stdout, TERMINAL_RED"x"TERMINAL_RESET);
            else if (board[j][i] == CONNECTX_PLAYER2) fprintf(stdout, TERMINAL_YELLOW"o"TERMINAL_RESET);
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

typedef struct arguments {
    char *player;
} arguments;

void parse_arguments(int argc, char **argv, arguments *args) {
    /* Simple positional parser: expect exactly 2 positional args
       Usage: connectx <player.so> */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <player.so>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    args->player = argv[1];
}

int load_move_function(arguments args) {
    void *strat = dlopen(args.player, RTLD_NOW);
    if (strat == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    move_player = (move_t)dlsym(strat, "connectx_move");
    if (move_player == NULL) {
        fprintf(stderr, "%s\n", dlerror());
        return -1;
    }

    return 0;
}


int wait_for_opponent(connectx_board_t board, char playing_as) {
    char current_player;
    char empt;
    for (int attempt = 0; attempt < 25; attempt++) {
        current_player = read_board(board, &empt);
        if (current_player == -1) {
            printf("Failed to read board state, retrying...\n");
            continue;
        }
        if (current_player == playing_as) {
            printf("Its our turn.\n");
            print_board(board);
            return 0;
        }
    }
    return -1;
}


int main(int argc, char **argv) {
    int move = 0;
    char result = 0;
    char player;
    char playing_as;

    srand(time(NULL));

    arguments args = {0};
    parse_arguments(argc, argv, &args);
    if (load_move_function(args) != 0) {
        fprintf(stderr, "Failed to load move functions.\n");
        return -1;
    }


    connectx_board_t board;
    player = read_board(board, &playing_as);
    if (player == -1) {
        printf("Failed to read board state.\n");
        return -1;
    }
    printf("Board state read successfully.\n");
    printf("Playing as player %d\n", playing_as);
    printf("Its time to move for player %d\n", player);
    print_board(board);


    while (1) {
        if (player != playing_as) {
            printf("Waiting for opponent's move...\n");
            if (wait_for_opponent(board, playing_as) != 0) {
                printf("Failed while waiting for opponent.\n");
                return -1;
            }
            player = playing_as;
            result = connectx_check_win_or_draw(board);
            if (result != 0) break;
        }
        move = move_player(board, player);
        play_move_board(board, move);
        if (move == -1 || connectx_update_board(board, move, player) == -1) {
            printf("Invalid move for player %d (%d).\n", player, move);
            result = swap_player(player);
            break;
        }
        print_board(board);
        result = connectx_check_win_or_draw(board);
        if (result != 0) break;

        sleep(800); // Wait a bit for the animation
        player = swap_player(player);
    }
    print_result(result);

    return 0;
}