# avr-remote-alarm

[![Lang](https://badgen.net/badge/Language/Embedded C/orange)](https://badgen.net/badge/Language/Embedded C/orange) [![Contr](https://badgen.net/badge/Controller/ATmega 328P/F96854)](https://badgen.net/badge/Controller/ATmega 328P/blue) [![license](https://badgen.net/badge/licence/MIT/blue)](./LICENSE)

<img src="https://raw.githubusercontent.com/tasoskakour/avr-remote-alarm/master/demo.gif">

### Description
Use your AVR mictrocontroller to set an alarm from your remote.

### Prerequisites

- AVR ATmega MicroController (I used 328P)
- A board to plug the microcontroller (I used Arduino UNO)
- A digit 7-Segment LED display (I used [![Display](http://thomas.bibby.ie/wp-content/uploads/2015/10/KYX-5461AS.jpg)](this) model)
- A display driver to control the LED display (I used MAX7219)
- A buzzer to be triggered for alarm
- A IR Receiver (like this  [![Display](https://www.modmypi.com/raspberry-pi/communication-1068/ir-infrared-1074/ir-infrared-receiver-tsop38238)](this))
- A IR remote controller that supports the NEC protocol (like [![Display](https://encrypted-tbn0.gstatic.com/images?q=tbn:ANd9GcTkDIgX6B70ryKA7WtmAHMzpprQgqfT-gmI3B6vkDbIh9fFAExP)](this))

And finally you're going to need a tool like Atmel Studio to compile and produce the .hex which you will load to the AVR with a program like XLoader.