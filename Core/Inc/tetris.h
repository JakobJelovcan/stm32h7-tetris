/*
 * tetris.h
 *
 *  Created on: Dec 23, 2023
 *      Author: Jakob
 */

#ifndef INC_TETRIS_H_
#define INC_TETRIS_H_

#ifndef MIN
#define MIN(a,b) ((a < b) ? a : b)
#else
#error MIN redefinition
#endif // !MIN

#ifndef MAX
#define MAX(a,b) ((a > b) ? a : b)
#else
#error MAX redefinition
#endif // !MAX

#define X_BANNER_DIM LCD_DEFAULT_WIDTH
#define Y_BANNER_DIM 60
#define X_BANNER_START 0
#define Y_BANNER_START (LCD_DEFAULT_WIDTH / 2 - Y_BANNER_DIM / 2)
#define X_DIM 10
#define Y_DIM 20
#define X_BOX 12
#define Y_BOX 12
#define X_BTN 64
#define Y_BTN 64
#define X_BTN_PADDING 10
#define Y_BTN_PADDING 10
#define N_BTN 6
#define MAX_LEVEL 9
#define MIN_LEVEL 0
#define LEVEL_THRESH 1000

#define X_START ((LCD_DEFAULT_WIDTH / 2) - ((X_DIM / 2) * X_BOX))
#define Y_START (LCD_DEFAULT_HEIGHT - Y_BOX * 2)
#define X_BORDER (X_DIM + 1)
#define Y_BORDER (Y_DIM + 1)

#include "polygons.h"
#include "stm32_lcd.h"
#include "stm32h750b_discovery_lcd.h"
#include <stdbool.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

typedef enum {
    MOVE_LEFT,
    MOVE_RIGHT,
    ROTATE_LEFT,
    ROTATE_RIGHT,
    DROP,
    RESET_GAME,
    TICK,
    PLAY_PAUSE
} action_t;

typedef struct {
    uint8_t polygon[2];
    uint8_t count[2];
    uint8_t selected;
} polygon_t;

typedef struct {
    uint8_t dir;
    uint8_t type;
    int8_t x;
    int8_t y;
} tetrimino_t;

typedef struct {
    uint16_t x;
    uint16_t y;
    action_t action;
    uint8_t state;
    polygon_t polygon;
} button_t;

void clear_lines(void);
void perform_action(const action_t action);
void render(void);
void reset_game(void);
bool get_game_over(void);

extern button_t buttons[N_BTN];

#endif /* INC_TETRIS_H_ */
