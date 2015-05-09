/*  Author: Steve Gunn
 * Licence: This work is licensed under the Creative Commons Attribution License.
 *           View this license at http://creativecommons.org/about/licenses/
 *
 *
 *  - Jan 2015  Modified for LaFortuna (Rev A, black edition) [KPZ]
 *  - Apr 2015  Modified to include software scrolling [cpk1g3/Clytomedes]
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include "font.h"
#include "ili934x.h"
#include "lcd.h"

#define MAX_SCROLLING_LINES 70 /* The number of lines that are stored */

lcd display;
static char lines[MAX_SCROLLING_LINES][53]; /* 30 lines with 53 characters per line */
uint8_t current, beginning, position, keep, scrolling, amountScrolledUp, clearing, canScrollUp, shiftedLines;

void init_lcd()
{
    /* Enable extended memory interface with 10 bit addressing */
    XMCRB = _BV(XMM2) | _BV(XMM1);
    XMCRA = _BV(SRE);
    DDRC |= _BV(RESET);
    DDRB |= _BV(BLC);
    _delay_ms(1);
    PORTC &= ~_BV(RESET);
    _delay_ms(20);
    PORTC |= _BV(RESET);
    _delay_ms(120);
    write_cmd(DISPLAY_OFF);
    write_cmd(SLEEP_OUT);
    _delay_ms(60);
    write_cmd_data(INTERNAL_IC_SETTING,          0x01);
    write_cmd(POWER_CONTROL_1);
        write_data16(0x2608);
    write_cmd_data(POWER_CONTROL_2,              0x10);
    write_cmd(VCOM_CONTROL_1);
        write_data16(0x353E);
    write_cmd_data(VCOM_CONTROL_2, 0xB5);
    write_cmd_data(INTERFACE_CONTROL, 0x01);
        write_data16(0x0000);
    write_cmd_data(PIXEL_FORMAT_SET, 0x55);     /* 16bit/pixel */
    set_orientation(West);
    clear_screen();
    display.x = 0;
    display.y = 0;
    display.background = BLACK;
    display.foreground = WHITE;
    write_cmd(DISPLAY_ON);
    _delay_ms(50);
    write_cmd_data(TEARING_EFFECT_LINE_ON, 0x00);
    /* EICRB |= _BV(ISC61); */
    PORTB |= _BV(BLC);

    current = beginning = position = amountScrolledUp = canScrollUp = shiftedLines = 0;
    scrolling = keep = clearing = 1;
}

void lcd_brightness(uint8_t i)
{
    /* Configure Timer 2 Fast PWM Mode 3 */
    TCCR2A = _BV(COM2A1) | _BV(WGM21) | _BV(WGM20);
    TCCR2B = _BV(CS20);
    OCR2A = i;
}

void set_orientation(orientation o)
{
    display.orient = o;
    write_cmd(MEMORY_ACCESS_CONTROL);
    if (o==North) {
        display.width = LCDWIDTH;
        display.height = LCDHEIGHT;
        write_data(0x48);
    }
    else if (o==West) {
        display.width = LCDHEIGHT;
        display.height = LCDWIDTH;
        write_data(0xE8);
    }
    else if (o==South) {
        display.width = LCDWIDTH;
        display.height = LCDHEIGHT;
        write_data(0x88);
    }
    else if (o==East) {
        display.width = LCDHEIGHT;
        display.height = LCDWIDTH;
        write_data(0x28);
    }
    write_cmd(COLUMN_ADDRESS_SET);
    write_data16(0);
    write_data16(display.width-1);
    write_cmd(PAGE_ADDRESS_SET);
    write_data16(0);
    write_data16(display.height-1);
}



void set_frame_rate_hz(uint8_t f)
{
    uint8_t diva, rtna, period;
    if (f>118)
        f = 118;
    if (f<8)
        f = 8;
    if (f>60) {
        diva = 0x00;
    } else if (f>30) {
        diva = 0x01;
    } else if (f>15) {
        diva = 0x02;
    } else {
        diva = 0x03;
    }
    /*   !!! FIXME !!!  [KPZ-30.01.2015] */
    /*   Check whether this works for diva > 0  */
    /*   See ILI9341 datasheet, page 155  */
    period = 1920.0/f;
    rtna = period >> diva;
    write_cmd(FRAME_CONTROL_IN_NORMAL_MODE);
    write_data(diva);
    write_data(rtna);
}

void fill_rectangle(rectangle r, uint16_t col)
{
    write_cmd(COLUMN_ADDRESS_SET);
    write_data16(r.left);
    write_data16(r.right);
    write_cmd(PAGE_ADDRESS_SET);
    write_data16(r.top);
    write_data16(r.bottom);
    write_cmd(MEMORY_WRITE);
/*  uint16_t x, y;
    for(x=r.left; x<=r.right; x++)
        for(y=r.top; y<=r.bottom; y++)
            write_data16(col);
*/
    uint16_t wpixels = r.right - r.left + 1;
    uint16_t hpixels = r.bottom - r.top + 1;
    uint8_t mod8, div8;
    uint16_t odm8, odd8;
    if (hpixels > wpixels) {
        mod8 = hpixels & 0x07;
        div8 = hpixels >> 3;
        odm8 = wpixels*mod8;
        odd8 = wpixels*div8;
    } else {
        mod8 = wpixels & 0x07;
        div8 = wpixels >> 3;
        odm8 = hpixels*mod8;
        odd8 = hpixels*div8;
    }
    uint8_t pix1 = odm8 & 0x07;
    while(pix1--)
        write_data16(col);

    uint16_t pix8 = odd8 + (odm8 >> 3);
    while(pix8--) {
        write_data16(col);
        write_data16(col);
        write_data16(col);
        write_data16(col);
        write_data16(col);
        write_data16(col);
        write_data16(col);
        write_data16(col);
    }
}

void fill_rectangle_indexed(rectangle r, uint16_t* col)
{
    uint16_t x, y;
    write_cmd(COLUMN_ADDRESS_SET);
    write_data16(r.left);
    write_data16(r.right);
    write_cmd(PAGE_ADDRESS_SET);
    write_data16(r.top);
    write_data16(r.bottom);
    write_cmd(MEMORY_WRITE);
    for(x=r.left; x<=r.right; x++)
        for(y=r.top; y<=r.bottom; y++)
            write_data16(*col++);
}

void set_foreground(uint16_t col)
{
    /* Set the colour of the foreground to be displayed when printing to the screen */
    display.foreground = col;
}

void set_background(uint16_t col)
{
    /* Set the colour of the background to be displayed when printing to the screen */
    display.background = col;
}

void set_clearing(uint8_t val)
{
    /* Turn on or off the screen clearing or scrolling when the cursor hits the bottom of the screen */
    clearing = val;
}

void clear_screen()
{
    display.x = 0;
    display.y = 0;
    rectangle r = {0, display.width-1, 0, display.height-1};
    fill_rectangle(r, display.background);
}

void set_auto_down_scrolling(uint8_t i)
{
    /* Turn the auto scrolling when at the bottom of the screen on or off */
    scrolling = i;
}

void scroll_to_bottom()
{
    /* Scroll smoothly to the bottom of the screen after scrolling up */
    while (amountScrolledUp > 0) {
        scroll_down();
        _delay_ms(50);
    }
}

void jump_to_bottom()
{
    /* Scroll as quickly as possible to the bottom of the screen after scrolling up */
    while (amountScrolledUp > 0) {
        scroll_down();
    }
}

void scroll_up_no_lines(uint8_t noLines) {
    /* Scroll up a specified number of lines, only possible if scrolled down */
    uint8_t i;
    for (i = 0; i < noLines; i++) {
        scroll_up();
    }
}

void scroll_up()
{
    if (canScrollUp > 0) {
        /* Scroll up a single line, only possible if scrolled down */
        scroll(beginning - 1, display.height / 8); /* Start the scrolling from the previous line to go up */

        amountScrolledUp++;
        /* Decrement the start of the circular array */
        if (beginning == 0) {
            beginning = MAX_SCROLLING_LINES;
        } else {
            beginning--;
        }
        canScrollUp--;
    }
}

void scroll_down()
{
    /* Scroll down a single line */
    scroll(beginning + 1, (display.height / 8) - 1); /* Start the scrolling from the next line to go down */

    if (amountScrolledUp > 0) {
        amountScrolledUp--;
    }

    /* Increment the start of the circular array */
    beginning++;
    if (beginning == MAX_SCROLLING_LINES) {
        beginning = 0;
    }

    if (canScrollUp < (MAX_SCROLLING_LINES - (display.height /8))) {
        canScrollUp++;
    }
}

uint8_t can_scroll_up()
{
    /* Has the screen scrolled so there are lines to scroll up to */
    if (canScrollUp > 0) {
        return 1;
    } else {
        return 0;
    }
}

uint8_t amount_scroll_up()
{
    /* The total number of lines that can be scrolled up */
    return canScrollUp;
}

uint8_t lines_from_bottom()
{
    /* Lines until the bottom of the screen is reached */
    if (canScrollUp > 0 || beginning > 0 || current > (display.height / 8)) {
        return 0;
    } else {
        return (display.height / 8) - current;
    }
}

void shift_up_lines(uint8_t noLines)
{
    /* Shift the lines on the screen up by the specified lines */
    scroll(beginning + noLines, (display.height / 8) - noLines);
    shiftedLines += noLines;
}

void shift_back_down(void)
{
    /* Return back to before the screen was shifted */
    if (lines_from_bottom() == 0) {
        scroll(beginning, (display.height / 8) - 1);
    } else {
        scroll(beginning, (display.height / 8) - lines_from_bottom());
    }
    shiftedLines = 0;
}

void scroll(uint8_t startLocation, uint8_t linesToPrint)
{
    /* Implementation of the software scrolling */
    clear_screen();

    keep = 0; /* Stop the display method from recording the printed strings and chars */
    uint8_t i = 0;
    uint8_t j;
    /* Check whether the lines wrap around the end of the circular array */
    if ((MAX_SCROLLING_LINES - startLocation) < linesToPrint) {
        /* Print all of the lines up to the end of the array */
        for (j = startLocation; j < MAX_SCROLLING_LINES; j++) {
            display_string(lines[j]);
            display.x=0;
            display.y+=8;
            i++;
        }
        /* Print the lines that have wrapped around the end of the circular array */
        for (j = 0; j < (linesToPrint - i); j++) {
            display_string(lines[j]);
            display.x=0;
            display.y+=8;
        }
    } else {
        /* Print all of the lines that are need to fill the screen apart from the bottom line */
        for (j = startLocation; j < (startLocation + linesToPrint); j++) {
            display_string(lines[j]);
            display.x=0;
            display.y+=8;
            i++;
        }
    }
    keep = 1;
}

void display_char(char c)
{
    /* If scrolled up, scroll down before printing to the screen */
    if (amountScrolledUp > 0 && keep) {
        jump_to_bottom();
    }

    /* If currently shifted, shift down before printing */
    if (shiftedLines > 0 && keep) {
        shift_back_down();
    }

    uint16_t x, y;
    PGM_P fdata;
    uint8_t bits, mask;
    uint16_t sc=display.x, ec=display.x + 4, sp=display.y, ep=display.y + 7;

    /*   New line starts a new line, or if the end of the
         display has been reached, clears the display.
    */
    if (c == '\n') {
        display.x=0;
        display.y+=8;

        current++;
        position = 0;
        if (current == MAX_SCROLLING_LINES) {
            current = 0;
        }
        if (display.y >= display.height) {
            if (!clearing) {
                /* Allow the cursor to reach the end of the screen and do nothing */
                return;
            } else if (scrolling) {
                scroll_down();
            } else {
                clear_screen();
            }
        }
        return;
    }

    /* Remember the characters that are being displayed to the screen for scrolling */
    if (keep) {
        if (position == 0) {
            memset(lines[current], 0, 54);
        }
        lines[current][position] = c;
        position++;
    }

    if (c < 32 || c > 126) return;
    fdata = (c - ' ')*5 + font5x7;
    write_cmd(PAGE_ADDRESS_SET);
    write_data16(sp);
    write_data16(ep);
    for(x=sc; x<=ec; x++) {
        write_cmd(COLUMN_ADDRESS_SET);
        write_data16(x);
        write_data16(x);
        write_cmd(MEMORY_WRITE);
        bits = pgm_read_byte(fdata++);
        for(y=sp, mask=0x01; y<=ep; y++, mask<<=1)
            write_data16((bits & mask) ? display.foreground : display.background);
    }
    write_cmd(COLUMN_ADDRESS_SET);
    write_data16(x);
    write_data16(x);
    write_cmd(MEMORY_WRITE);
    for(y=sp; y<=ep; y++)
        write_data16(display.background);

    display.x += 6;
    if (display.x + 5 >= display.width) {
        display.x=0;
        display.y+=8;

        current++;
        position = 0;
        if (current == MAX_SCROLLING_LINES) {
            current = 0;
        }
    }
    if (display.y >= display.height) {
        if (!clearing) {
            /* Allow the cursor to reach the end of the screen and do nothing */
            return;
        } else if (scrolling) {
            scroll_down();
        } else {
            clear_screen();
        }
    }
}

void display_string(char *str)
{
    uint8_t i;
    for(i=0; str[i]; i++)
        display_char(str[i]);
}

uint16_t get_x(void)
{
    return display.x;
}

uint16_t get_y(void)
{
    return display.y;
}

void set_xy(uint16_t x, uint16_t y)
{
    display.x = x;
    display.y = y;
}

lcdState get_lcd_state()
{
    /* Get the current state */
    lcdState state = { display, current, beginning, position, keep, scrolling, amountScrolledUp, clearing, canScrollUp, shiftedLines };
    return state;
}

void set_lcd_state(lcdState state)
{
    /* Load a state */
    display = state.displayState;
    current = state.currentState;
    beginning = state.beginningState;
    position = state.positionState;
    keep = state.keepState;
    scrolling = state.scrollingState;
    amountScrolledUp = state.amountScrolledUpState;
    clearing = state.clearingState;
    canScrollUp = state.canScrollUpState;
    shiftedLines = state.shiftedLinesState;
}

void display_char_xy(char c, uint16_t x, uint16_t y)
{
    /* Cannot be implemented with scrolling due to ability of possibly not being on the same lines
       Recommened to turn off scrolling if using this and possibly setting the line clearing off */
    uint16_t oldX = display.x;
    uint16_t oldY = display.y;
    keep = 0;
    display.x = x;
    display.y = y;
    display_char(c);
    keep = 1;
    display.x = oldX;
    display.y = oldY;
}

void display_string_xy(char *str, uint16_t x, uint16_t y)
{
    /* Cannot be implemented with scrolling due to ability of possibly not being on the same lines
       Recommened to turn off scrolling if using this and possibly setting the line clearing off */
    uint16_t oldX = display.x;
    uint16_t oldY = display.y;
    keep = 0;
    uint8_t i;
    display.x = x;
    display.y = y;
    for(i=0; str[i]; i++)
        display_char(str[i]);
    keep = 1;
    display.x = oldX;
    display.y = oldY;
}
