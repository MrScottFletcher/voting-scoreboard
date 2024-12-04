# voting-scoreboard
A voting system with an LED scoreboard, buttons to vote, marquee border LEDs, and sound effects.

Parts list:


Wiring Notes:
- I had to add a 330 ohm resistor to the chaser LED data line to stop the flickering  The resistor absorbs the 'ringing'/reflections effect in the data line.
- The ATMega breakout board I used had a bad 5V power terminal, causin the DFPlayer Mini to not initialize.  Moving the 5V and Ground lines to a different etrminal on the Mega's breakout board fixed that problem.
- I followed the internet's advice and put a 330K resistor on the RX line of the DFPlayer to protect it from the Mega's 5V power.

