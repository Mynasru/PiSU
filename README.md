If your 3D printer houses more than just your controller board, you probably need some sort of power management.

This module will connect to:

3D printer controller board (e.g. RAMPS1.4).
Raspberry pi
ATX PSU
User on/off button
The PiSU works like a intermediator between the three boards, and makes sure power is delivered at the right time to the right module, regulates a gracefull shutdown, and sends the right commands.

The main core of the PiSU is an Atmega328 programmed using Arduino.

Installation:
- sudo wget https://raw.githubusercontent.com/LowPowerLab/ATX-Raspi/master/shutdownchecksetup.sh
- sudo bash shutdownchecksetup.sh
- sudo rm shutdownchecksetup.sh
- sudo reboot