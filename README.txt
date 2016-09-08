Author(s): Andrew Domaszek
Hardware Platform: DM164130-9-ND Microchip Low Pin Count Demo Board
Debuggers Used: PICkit3
Programmers Used: PICkit3
MPLAB Version: MPLAB X v3.40
C Compiler Version: XC8 v1.38 
Final Checksum: ????

DESCRIPTION AND CONFIGURATION
=============================
Demonstrate changing pwm period and duty cycle in real time.

By shorting RA2 to ground (or pressing SW1), the timer changes duration to an
alternate setting.

Connect oscilloscope to pins RA2, RA4, RA5, RC0.
RA2 - short to ground to switch modes.
RA4 - mode indicator, trigger oscilloscope on rising and falling edges to catch
      transition behavior.
RA5 - PWM1_out routed to a pin.
RC0 - toggles on unhandled interrupts, see program configuration defines.

PROGRAMMING
===========
Set PICkit3 powers device in the project settings, 5V power.
Program from MPLAB X IDE.

PERIPHERALS USED
================
PWM1
WPUA (not necessary on this platform)
PPS
*MCLR