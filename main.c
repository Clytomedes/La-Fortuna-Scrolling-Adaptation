/* La Fortuna Scrolling Test */


#include <avr/io.h>
#include <util/delay.h>
#include "lcd.h"

void init(void);

void main(void) {
    init();

    int i = 0;
    char str[15];
    /* Display the auto scrolling when hitting the bottom of the screen */
    for (i = 0; i < 40; i++) {
      sprintf(str, "%d\n", i);
      display_string(str);
      _delay_ms(100);
    }

    _delay_ms(500);

    /* Display the scrolling back up */
    for (i = 0; i < 5; i++) {
        scroll_up();
        _delay_ms(100);
    }

    _delay_ms(500);

    /* Show printing text after scrolling up */
    display_string("Bottom");
}


void init(void) {
    /* 8MHz clock, no prescaling (DS, p. 48) */
    CLKPR = (1 << CLKPCE);
    CLKPR = 0;

    init_lcd();
}
