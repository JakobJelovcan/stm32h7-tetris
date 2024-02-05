/*
 * tetris.c
 *
 *  Created on: Dec 23, 2023
 *      Author: Jakob
 */
#include "tetris.h"

extern RNG_HandleTypeDef rng;

const uint32_t colors[8] = {
        UTIL_LCD_COLOR_DARKGRAY,
        UTIL_LCD_COLOR_CYAN,		//I
        UTIL_LCD_COLOR_YELLOW,		//O
        UTIL_LCD_COLOR_DARKMAGENTA, //T
        UTIL_LCD_COLOR_GREEN,		//S
        UTIL_LCD_COLOR_BLUE,		//J
        UTIL_LCD_COLOR_RED,			//Z
        UTIL_LCD_COLOR_ORANGE };    //L

const uint16_t tetriminos[8][4] = {
        { 0x0000, 0x0000, 0x0000, 0x0000 },
        { 0x0F00, 0x2222, 0x00F0, 0x4444 },   //I
        { 0xCC00, 0xCC00, 0xCC00, 0xCC00 },   //O
        { 0x0E40, 0x4C40, 0x4E00, 0x4640 },   //T
        { 0x06C0, 0x8C40, 0x6C00, 0x4620 },   //S
        { 0x44C0, 0x8E00, 0x6440, 0x0E20 },   //J
        { 0x0C60, 0x4C80, 0xC600, 0x2640 },   //Z
        { 0x4460, 0x0E80, 0xC440, 0x2E00 } }; //L

button_t buttons[N_BTN] = {
    { X_BTN_PADDING, LCD_DEFAULT_HEIGHT - Y_BTN_PADDING - Y_BTN, MOVE_LEFT, 0, { {0, 0}, {1, 1}, 0} },
    { X_BTN_PADDING, LCD_DEFAULT_HEIGHT - Y_BTN_PADDING * 2 - Y_BTN * 2, ROTATE_LEFT, 0, { {1, 1}, {1, 1}, 0} },
    { LCD_DEFAULT_WIDTH - X_BTN_PADDING - X_BTN, LCD_DEFAULT_HEIGHT - Y_BTN_PADDING - Y_BTN, MOVE_RIGHT, 0, { {2, 2}, {1, 1}, 0} },
    { LCD_DEFAULT_WIDTH - X_BTN_PADDING - X_BTN, LCD_DEFAULT_HEIGHT - Y_BTN_PADDING * 2 - Y_BTN * 2, ROTATE_RIGHT, 0, { {3, 3}, {1, 1}, 0} },
    { X_BTN_PADDING, LCD_DEFAULT_HEIGHT - Y_BTN_PADDING * 3 - Y_BTN * 3, DROP, 0, { {4, 4}, {1, 1}, 0} },
    { LCD_DEFAULT_WIDTH - X_BTN_PADDING - X_BTN, LCD_DEFAULT_HEIGHT - Y_BTN_PADDING * 3 - Y_BTN * 3, PLAY_PAUSE, 0, { {5, 6}, {1, 2}, 1} }
};

const uint8_t level_speed[] = { 72, 64, 58, 50, 44, 36, 30, 22, 14, 10, 8, 8, 8, 6, 6, 6, 4, 4, 4, 2 };
const uint16_t lines_score[] = { 0, 100, 300, 500, 800 };
uint32_t top_scores[MMC_BLOCKSIZE / sizeof(uint32_t)];

game_t game;
uint32_t last_save;
bool save_after_reset;

/// <summary>
/// Stores the game to the eMMC memory
/// </summary>
/// <param name=""></param>
static void store_game(void) {
    static uint32_t buf[MMC_BLOCKSIZE / sizeof(uint32_t)];
    memcpy(buf, &game, sizeof(game_t));
    BSP_MMC_WriteBlocks(0, buf, EMMC_START_ADDR + EMMC_GAME_OFFSET, EMMC_BLOCK_COUNT);
    while (BSP_MMC_GetCardState(0) != MMC_TRANSFER_OK);
}

/// <summary>
/// Loads the game from the eMMC memory
/// </summary>
/// <param name=""></param>
void load_game(void) {
    static uint32_t buf[MMC_BLOCKSIZE / sizeof(uint32_t)];
    BSP_MMC_ReadBlocks(0, buf, EMMC_START_ADDR + EMMC_GAME_OFFSET, EMMC_BLOCK_COUNT);
    while (BSP_MMC_GetCardState(0) != MMC_TRANSFER_OK);
    memcpy(&game, buf, sizeof(game_t));

    // Start the game
    game.playing = !game.game_over;
}

/// <summary>
/// Loads the top scores from the EMMC flash
/// </summary>
/// <param name=""></param>
static void load_top_scores(void) {
    BSP_MMC_ReadBlocks(0, top_scores, EMMC_START_ADDR, EMMC_BLOCK_COUNT);
    while (BSP_MMC_GetCardState(0) != MMC_TRANSFER_OK);
}

/// <summary>
/// Stores the top scores to the EMMC flash
/// </summary>
/// <param name=""></param>
static void store_top_scores(void) {
    BSP_MMC_WriteBlocks(0, top_scores, EMMC_START_ADDR, EMMC_BLOCK_COUNT);
    while (BSP_MMC_GetCardState(0) != MMC_TRANSFER_OK);
}

/// <summary>
/// Update top scores and set game_over to true
/// </summary>
/// <param name=""></param>
static void finish_game(void) {
    if (!game.game_over) {
        game.game_over = true;
        game.playing = false;

        size_t i = 0;
        for (; i < N_TOP_SCORES && game.score < top_scores[i]; ++i);

        for (size_t j = N_TOP_SCORES - 1; j > i; --j) {
            top_scores[j] = top_scores[j - 1];
        }

        if (i < N_TOP_SCORES) {
            top_scores[i] = game.score;
            store_top_scores();
        }
    }
}

/// <summary>
/// Gets the x position for the new tetrimino
/// </summary>
/// <param name="type"></param>
/// <returns></returns>
static int8_t get_new_x_position(uint8_t type) {
    static uint16_t masks[] = { 0x8888, 0x4444, 0x2222, 0x1111 };
    uint32_t x;

    uint8_t min = 4, max = 0;
    const uint16_t tetrimino = tetriminos[type][0];
    for (size_t i = 0; i < 4; ++i) {
        if ((tetrimino & masks[i]) != 0) {
            min = MIN(min, i);
            max = MAX(max, i);
        }
    }
    HAL_RNG_GenerateRandomNumber(&rng, &x);
    return min + x % (X_DIM - max - min);
}

/// <summary>
/// Gets the y position for the new tetrimino
/// </summary>
/// <param name="type"></param>
/// <returns></returns>
static int8_t get_new_y_position(uint8_t type) {
    static uint16_t masks[] = { 0x000f, 0x00f0, 0x0f00, 0xf000 };

    const uint16_t tetrimino = tetriminos[type][0];
    for (size_t i = 0; i < 4; ++i) {
        if ((tetrimino & masks[i]) != 0) {
            return Y_DIM - i;
        }
    }

    return Y_DIM;
}

/// <summary>
/// Creates a new tetrimino and stores in the location provided by the argument
/// </summary>
/// <param name="tetrimino"></param>
static void create_tetrimino(tetrimino_t* tetrimino) {
    uint32_t type;
    HAL_RNG_GenerateRandomNumber(&rng, &type);
    tetrimino->dir = 0;
    tetrimino->type = 1 + type % 7;
    tetrimino->x = get_new_x_position(tetrimino->type);
    tetrimino->y = get_new_y_position(tetrimino->type);
}

/// <summary>
/// Draws a banner displaying the game over sign
/// </summary>
/// <param name=""></param>
static void draw_game_over(void) {
    if (game.game_over) {
        UTIL_LCD_SetFont(&UTIL_LCD_DEFAULT_FONT);
        UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_DARKGRAY);
        UTIL_LCD_FillRect(X_BANNER_START, Y_BANNER_START, X_BANNER_DIM, Y_BANNER_DIM, UTIL_LCD_COLOR_DARKGRAY);
        UTIL_LCD_DisplayStringAt(0, Y_BANNER_START + Y_BANNER_DIM / 2, (uint8_t*)"Game over", CENTER_MODE);
        UTIL_LCD_SetFont(&Font12);
    }
}

/// <summary>
/// Draws a banner displaying top 3 scores
/// </summary>
/// <param name=""></param>
static void draw_scores(void) {
    static char buf[32];
    if (!game.game_over && !game.playing) {
        UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_DARKGRAY);
        UTIL_LCD_FillRect(X_BANNER_START, Y_BANNER_START, X_BANNER_DIM, Y_BANNER_DIM, UTIL_LCD_COLOR_DARKGRAY);

        UTIL_LCD_SetFont(&Font20);
        UTIL_LCD_DisplayStringAt(0, Y_BANNER_START + 10, (uint8_t*)"Top scores", CENTER_MODE);

        UTIL_LCD_SetFont(&Font16);
        for (size_t i = 0; i < N_TOP_SCORES; ++i) {
            sprintf(buf, "%d: %d", i + 1, top_scores[i]);
            UTIL_LCD_DisplayStringAt(0, Y_BANNER_START + 40 + 16 * i, (uint8_t*)buf, CENTER_MODE);
        }
        UTIL_LCD_SetFont(&Font12);
    }
}

/// <summary>
/// Draws a box with the selected color
/// </summary>
/// <param name="x">position of the box</param>
/// <param name="y">position of the box</param>
/// <param name="color">of the box</param>
static void draw_box(uint16_t x, uint16_t y, uint8_t color) {
    UTIL_LCD_FillRect(x, y, X_BOX, Y_BOX, colors[color]);
    UTIL_LCD_DrawRect(x, y, X_BOX, Y_BOX, UTIL_LCD_COLOR_GRAY);
}

/// <summary>
/// Draws the current tetrimino
/// </summary>
/// <param name=""></param>
static void draw_tetrimino(void) {
    if (game.tetrimino.type != 0) {
        const uint16_t shape = tetriminos[game.tetrimino.type][game.tetrimino.dir];
        for (size_t i = 0; i < 4; ++i) {
            for (size_t j = 0; j < 4; ++j) {
                if (shape & 1 << (j + i * 4)) {
                    draw_box(X_START + (game.tetrimino.x + 3 - j) * X_BOX, Y_START - (game.tetrimino.y + i) * Y_BOX, game.tetrimino.type);
                }
            }
        }
    }
}

/// <summary>
/// Draws the boxes in the playing field
/// </summary>
/// <param name=""></param>
static void draw_playing_field(void) {
    for (size_t i = 0; i < Y_DIM; ++i) {
        for (size_t j = 0; j < X_DIM; ++j) {
            if (game.playing_field[i][j] != 0) {
                draw_box(X_START + j * X_BOX, Y_START - i * Y_BOX, game.playing_field[i][j]);
            }
        }
    }
}

/// <summary>
/// Draws the border of the playing field
/// </summary>
/// <param name=""></param>
static void draw_border(void) {
    uint32_t x_offset, y_offset;

    //Bottom border
    x_offset = X_START - X_BOX;
    y_offset = Y_START + Y_BOX;
    for (size_t i = 0; i < X_BORDER; ++i) {
        draw_box(x_offset, y_offset, 0);
        x_offset += X_BOX;
    }

    //Left border
    x_offset = X_START - X_BOX;
    y_offset = Y_START + Y_BOX;
    for (size_t i = 0; i < Y_BORDER; ++i) {
        draw_box(x_offset, y_offset, 0);
        y_offset -= Y_BOX;
    }

    //Right border
    x_offset = X_START - X_BOX + X_BORDER * X_BOX;
    y_offset = Y_START + Y_BOX;
    for (size_t i = 0; i < Y_BORDER; ++i) {
        draw_box(x_offset, y_offset, 0);
        y_offset -= Y_BOX;
    }
}

/// <summary>
/// Draws a button
/// </summary>
/// <param name="btn">button to be drawn</param>
static void draw_button(button_t btn) {
    const uint32_t border_color = ((btn.state & 0x1) == 1) ? UTIL_LCD_COLOR_DARKGRAY : UTIL_LCD_COLOR_GRAY;
    const uint8_t selected = btn.polygon.selected;
    UTIL_LCD_FillRect(btn.x, btn.y, X_BTN, Y_BTN, border_color);
    for (size_t i = 0; i < btn.polygon.count[selected]; ++i) {
        uint8_t poly = btn.polygon.polygon[selected] + i;
        UTIL_LCD_DrawPolygon(polygons[poly], polygon_sizes[poly], UTIL_LCD_COLOR_WHITE);
    }
    UTIL_LCD_DrawRect(btn.x, btn.y, X_BTN, Y_BTN, UTIL_LCD_COLOR_LIGHTGRAY);
}

/// <summary>
/// Draws buttons required for the game
/// </summary>
/// <param name=""></param>
static void draw_buttons(void) {
    for (size_t i = 0; i < N_BTN; ++i) {
        draw_button(buttons[i]);
    }
}

/// <summary>
/// Creates a 16bit 4x4 mask where 1 represents invalid fields
/// </summary>
/// <param name="x">position of the bottom left corner</param>
/// <param name="y">position of the bottom left corner</param>
/// <returns>mask</returns>
static uint16_t overlap_mask(int8_t x, int8_t y) {
    uint16_t mask = 0;
    for (int32_t i = 3; i >= 0; --i) {
        for (int32_t j = 0; j < 4; ++j) {
            mask = (mask << 1) | ((x + j < 0) || (y + i < 0) || (x + j >= X_DIM) || ((y + i < Y_DIM) && (game.playing_field[y + i][x + j] != 0)));
        }
    }
    return mask;
}

/// <summary>
/// Checks if the position of the tetrimino is valid
/// </summary>
/// <param name="type">of the tetrimino</param>
/// <param name="dir">orientation of the tetrimino</param>
/// <param name="x">position of the left bottom corner</param>
/// <param name="y">position of the left bottom corner</param>
/// <returns></returns>
static bool valid(uint8_t type, uint8_t dir, int8_t x, int8_t y) {
    return (overlap_mask(x, y) & tetriminos[type][dir]) == 0;
}

/// <summary>
/// Places the tetrimino to the lowest possible position on the playing field
/// </summary>
/// <param name="type">of the tetrimino</param>
/// <param name="dir">orientation of the tetrimino</param>
/// <param name="x">position of the left bottom corner</param>
/// <param name="y">position of the left bottom corner</param>
static void place_on_playing_field(uint8_t type, uint8_t dir, int8_t x, int8_t y) {
    uint32_t drop_count = 0;
    while (valid(type, dir, x, y - 1)) {
        --y;
        ++drop_count;
    }

    game.score += 2 * drop_count;
    const uint16_t tetrimino = tetriminos[type][dir];
    uint16_t mask = 1;
    for (int32_t i = 0; i < 4; ++i) {
        for (int32_t j = 3; j >= 0; --j) {
            if ((mask & tetrimino) != 0) {
                if (x + j >= 0 && x + j < X_DIM && y + i >= 0 && y + i < Y_DIM) {
                    game.playing_field[y + i][x + j] = type;
                } else {
                    finish_game();
                }
            }
            mask <<= 1;
        }
    }
}

/// <summary>
/// Removes full lines from the playing field and increases the score
/// </summary>
/// <param name=""></param>
void clear_lines(void) {
    static bool full[Y_DIM];

    //Find full lines
    for (size_t i = 0; i < Y_DIM; ++i) {
        full[i] = true;
        for (size_t j = 0; j < X_DIM; ++j) {
            full[i] = full[i] && (game.playing_field[i][j] != 0);
        }
    }

    //Remove full lines
    size_t y = 0;
    for (size_t i = 0; i < Y_DIM; ++i) {
        if (!full[i]) {
            if (i != y) {
                memmove(game.playing_field[y++], game.playing_field[i], X_DIM * sizeof(uint8_t));
            } else {
                ++y;
            }
        }
    }

    //Clear old lines
    for (size_t i = y; i < Y_DIM; ++i) {
        memset(game.playing_field[i], 0, X_DIM * sizeof(uint8_t));
    }

    //Calculate score
    uint32_t count = 0;
    for (size_t i = 0; i < Y_DIM; ++i) {
        if (full[i]) {
            ++count;
            ++game.lines_cleared;
        } else {
            game.score += (game.level + 1) * lines_score[count];
            count = 0;
        }
    }
    if (count > 0) {
        game.score += (game.level + 1) * lines_score[count];
    }
    game.level = MIN(game.lines_cleared / LEVEL_THRESH, MAX_LEVEL);
}

/// <summary>
/// Perform the selected action
/// </summary>
/// <param name="action">to be performed</param>
void perform_action(const action_t action) {
    if (action == RESET_GAME) {
        finish_game();
        reset_game();
    } else if (action == PLAY_PAUSE) {
        game.playing = !game.playing;
    } else if (!game.game_over && game.playing) {
        switch (action) {
            case MOVE_LEFT: {
                if (valid(game.tetrimino.type, game.tetrimino.dir, game.tetrimino.x - 1, game.tetrimino.y)) {
                    --game.tetrimino.x;
                }
                break;
            }
            case MOVE_RIGHT: {
                if (valid(game.tetrimino.type, game.tetrimino.dir, game.tetrimino.x + 1, game.tetrimino.y)) {
                    ++game.tetrimino.x;
                }
                break;
            }
            case ROTATE_LEFT: {
                if (valid(game.tetrimino.type, (game.tetrimino.dir + 3) % 4, game.tetrimino.x, game.tetrimino.y)) {
                    game.tetrimino.dir = (game.tetrimino.dir + 3) % 4;
                }
                break;
            }
            case ROTATE_RIGHT: {
                if (valid(game.tetrimino.type, (game.tetrimino.dir + 1) % 4, game.tetrimino.x, game.tetrimino.y)) {
                    game.tetrimino.dir = (game.tetrimino.dir + 1) % 4;
                }
                break;
            }
            case DROP: {
                place_on_playing_field(game.tetrimino.type, game.tetrimino.dir, game.tetrimino.x, game.tetrimino.y);
                create_tetrimino(&game.tetrimino);
                break;
            }
        }
    }
}

/// <summary>
/// Updates the state of the game by moving the current tetrimino down by one block and checking if the game is still in a valid state
/// </summary>
/// <param name=""></param>
void update_state(void) {
    if (!game.game_over && game.playing) {
        if (game.time - game.last_update >= level_speed[game.level]) {
            game.last_update = game.time;
            if (valid(game.tetrimino.type, game.tetrimino.dir, game.tetrimino.x, game.tetrimino.y - 1)) {
                --game.tetrimino.y;
            } else {
                place_on_playing_field(game.tetrimino.type, game.tetrimino.dir, game.tetrimino.x, game.tetrimino.y);
                create_tetrimino(&game.tetrimino);
            }
        }
    }

    // Store the game every second
    if (((game.time - last_save) / TIME_DIV) > 60) {
        last_save = game.time;
        save_after_reset = true;
        store_game();
    }
}

/// <summary>
/// Renders the image onto the screen
/// </summary>
/// <param name=""></param>
void render(void) {
    static char str_buffer[32];

    draw_border();
    draw_playing_field();
    draw_buttons();
    draw_tetrimino();

    UTIL_LCD_SetBackColor(UTIL_LCD_COLOR_BLACK);
    sprintf(str_buffer, "Time: %lds", game.time / TIME_DIV);
    UTIL_LCD_DisplayStringAt(4, 10, (uint8_t*)str_buffer, LEFT_MODE);
    sprintf(str_buffer, "Score: %ld, Level: %ld", game.score, game.level);
    UTIL_LCD_DisplayStringAt(0, 10, (uint8_t*)str_buffer, RIGHT_MODE);

    draw_game_over();
    draw_scores();
}

/// <summary>
/// Resets the game
/// </summary>
/// <param name=""></param>
void reset_game(void) {
    game.level = 0;
    game.lines_cleared = 0;
    game.score = 0;
    game.time = 0;
    game.last_update = 0;
    game.playing = true;
    game.game_over = false;

    last_save = 0;
    create_tetrimino(&game.tetrimino);
    load_top_scores();
    memset(game.playing_field, 0, X_DIM * Y_DIM * sizeof(uint8_t));

    if (save_after_reset) {
        store_game();
    }
    save_after_reset = false;
}

/// <summary>
/// Increase time by one unit
/// </summary>
/// <param name=""></param>
void tick(void) {
    if (!game.game_over && game.playing) {
        ++game.time;
    }
}