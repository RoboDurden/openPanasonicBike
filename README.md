# openPanasonicBike
open source Panasonic/Flyer eBike/Pedelec Motor Controller

complete replacement of the Panasonic/Flyer mid-motor controller.
Arduino Mini Pro 5V, one inside the motor, one for the display.

watch: https://www.youtube.com/watch?v=UOGolEJIfsc

read: 
https://www.pedelecforum.de/forum/index.php?threads/open-source-panasonic-flyer-ebike-pedelec-controller.63354/

The panasonicDrive.h is for both Arduinos and needs an absolute path or to be put in libraires/panasonicDrive/

twi.c and twi.h from the Arduino Wire library at Arduino\hardware\arduino\avr\libraries\Wire\src needs to be modified as the Arduino IDE programers were so stupid to omit timeouts.

GNU General Public License v3.0

ideas welcome :-)
