/*  Author: Steve Gunn
 * Licence: This work is licensed under the Creative Commons Attribution License.
 *           View this license at http://creativecommons.org/about/licenses/
 *
 *
 *  - Apr 2015  Modified to include software scrolling [cpk1g3/Clytomedes]
 */

#include <avr/io.h>
#include <stdint.h>

#define LCDWIDTH	240
#define LCDHEIGHT	320

/* Colour definitions RGB565 */
#define WHITE       0xFFFF
#define BLACK       0x0000
#define BLUE        0x001F
#define GREEN       0x07E0
#define CYAN        0x07FF
#define RED         0xF800
#define MAGENTA     0xF81F
#define YELLOW      0xFFE0

typedef enum {North, West, South, East} orientation;

typedef struct {
	uint16_t width, height;
	orientation orient;
	uint16_t x, y;
	uint16_t foreground, background;
} lcd;

extern lcd display;

typedef struct {
	uint16_t left, right;
	uint16_t top, bottom;
} rectangle;

typedef struct {
	lcd displayState;
	uint8_t currentState, beginningState, positionState, keepState, scrollingState, amountScrolledUpState, clearingState, canScrollUpState, shiftedLinesState;
} lcdState;

void init_lcd();
void lcd_brightness(uint8_t i);
void set_orientation(orientation o);
void set_frame_rate_hz(uint8_t f);
void set_foreground(uint16_t col);
void set_background(uint16_t col);
void set_clearing(uint8_t val);
void clear_screen();
void set_auto_down_scrolling(uint8_t i);
void scroll_to_bottom(void);
void scroll_up_no_lines(uint8_t noLines);
void scroll(uint8_t scrollLocation, uint8_t linesToPrint);
void scroll_up(void);
void scroll_down(void);
uint8_t can_scroll_up(void);
uint8_t amount_scroll_up(void);
uint8_t lines_from_bottom(void);
void shift_up_lines(uint8_t noLines);
void shift_back_down(void);
void return_line(void);
void fill_rectangle(rectangle r, uint16_t col);
void fill_rectangle_indexed(rectangle r, uint16_t* col);
void display_char(char c);
void display_string(char *str);
uint16_t get_x(void);
uint16_t get_y(void);
void set_xy(uint16_t x, uint16_t y);
lcdState get_lcd_state();
void set_lcd_state(lcdState state);
void display_char_xy(char c, uint16_t x, uint16_t y);
void display_string_xy(char *str, uint16_t x, uint16_t y);
