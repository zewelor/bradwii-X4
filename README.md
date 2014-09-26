Bradwii for Hubsan X4 H107L using GCC and Eclipse
=======

Forked from

https://github.com/hackocopter/bradwii-jd385

The original project also supports JXD JD385, WLToys V202 and clones and was developed using Keil tools.
In this fork I only work on Hubsan H107L hardware. I try to keep the source for the other models intact but I don't try to compile it and I can't test it.
The Keil project files are kept as well but they also don't get updated.

### Used development environment
 * Kubuntu 14.04 amd64
 * Eclipse IDE http://www.eclipse.org/
 * GNU ARM Eclipse plugin http://gnuarmeclipse.livius.net/blog/
 * GCC cross compiler for ARM Cortex-A/R/M (gcc-arm-none-eabi)
 * UM232H FTDI development board configured as SWD adapter
 * OpenOCD, built from latest source. Release 0.8.0 does not include SWD driver for FTDI yet. http://openocd.sourceforge.net/

### The H107L uses the following hardware
 * Nuvoton MINI54ZAN ARM Cortex-M0
 * AMICCOM A7105 2.5GHz transceiver
 * mCube MC3210 3-Axis Accelerometer
 * InvenSense MPU-3050 3-Axis MEMS Gyroscope

Current status
======
The quadcopter can at fly and shows good flight stability. No problems with oscillations so far.
Trying to lift off slowly it just flips over. In general, it does not like flying close to the ground.
The original Hubsan firmware had less problems in that area.

Flying feels quite different compared to the original firmware, so it took some time for me to get accustomed to it.
I did not optimize the control parameters and other parameters at all yet, so there is room for improvement.

There is no parameter storage yet. 
All parameters are hard-coded and gyro/accelerometer calibration is executed at every power-on.
The board does not have a serial port, so a PC configuration software cannot be connected.

How to use:
 * Switch quacdopter on: all LEDs blink alternating
 * Put quadcopter on level and steady surface
 * Switch transmitter on
 * LEDs blink in circular pattern. Don't move the quadcopter during this time because the calibration is ongoing.
 * When calibration is done, all LEDs blink short pulses. Quadcopter is not armed yet and will not respond to throttle.
 * Press the lower throttle trim button for one second ("LEDs off" for Hubsan firmware)
 * All LEDs are on and you are ready to fly.
 * When the LEDs start to blink during flight it's time to land because the battery is nearly empty. With this firmware the LEDs only blink *while* the battery voltage is low,
so blinking might be temporary at high throttle.

Disarm by pressing the lower throttle trim button button again for one second.


#### Development issues:

Resetting the board using OpenOCD and the UM232H based SWD adapter does not work. Need to check with scope what's going on.

The OpenOCD Mini51 flash driver did not work for me, so I modified it.
Config file and modified source [here](https://gist.github.com/TheLastMutt/d1c1948acaace7444c1c).

Credits
======

 * Bradwii was originally coded by Brad Quick for AVR: https://github.com/bradquick/bradwii
 * Trollcop forked and ported to ARM STM32, untested: https://github.com/trollcop/bradwii
 * The Mini54ZAN ARM port to V202/JD385 was done by Victor: https://github.com/victzh/bradwii
 * The Hubsan X4 H107L port was done by Goebish: https://github.com/goebish/bradwii-X4

More information
======
http://www.rcgroups.com/forums/showthread.php?t=2174365
https://www.mikrocontroller.net/topic/309185 (German)
