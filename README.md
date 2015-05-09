# La Fortuna Scrolling Adaptation
Adaptation of Steve Gunn's LCD library for the La Fortuna board to implement software scrolling

###Dependencies
* avr-gcc

###How to Build
1. Run the make file that is supplied, which should work fine on Linux and will work on Windows if run through Cygwin or another bash like terminal

###Usage
This is just the library implementation of the software scrolling. By default it is set so that when the text reaches the end of the screen that it will automatically scroll down a line, leaving a gap at the bottom for a new line. There are other methods that are included to turn off the auto scrolling as well as to scroll up and scroll to the bottom if you have scrolled up. If you try to write a new character or string while the screen is scrolled up, it will automatically scroll back down before it prints the new line to the screen.
