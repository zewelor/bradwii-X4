Bradwii for Hubsan X4 H107L using GCC and Eclipse
=======

Forked from

https://github.com/hackocopter/bradwii-jd385

The original project also supports JXD JD385, WLToys V202 and clones and was developed using Keil tools.
In this fork I only work on Hubsan H107L hardware. I try to keep the source for the other models intact but I don't try to compile it and I can't test it.
The Keil project files are kept as well but they also don't get updated.

Current status
======
The control parameters habe been optimized in a way that flying feels similar to the original firmware.
It is possible to lift off very slowly and fly close to the ground.
The control parameters have not been tested in acro or semi-acro mode.

Accelerometer calibration can be separated from gyro calibration, like the original firmware does. After executing the manual accelerometer calibration,
the parameters are stored in data flash. At subsequent power-on only the gyro calibration is executed. This way there is no need to find a level surface
for every power-on.

Testing a new flight mode: "yaw hold". In this mode the yaw controller controls the yaw angle instead of the yaw rate.
If the quadcopter gets rotated away from its desired heading (due to wind or high throttle or whatever), it will try to rotate back to the desired heading.
This is similar to compass mode, but a compass is not needed. This mode can be activated using the checkboxes like all the other flight modes.
It is currently activated using AUX2 ("flip"/"no flip" channel of X4).

The X4 board does not have a serial port, so a PC configuration software cannot be connected.

How to use:
 * Switch quadcopter on
 * (If all LEDs slowly blink 4 times, no accelerometer calibration was found in data flash. Accelerometer will be calibrated after binding with transmitter,
so a level surface is needed. Use manual accelerometer calibration to remedy this)
 * All LEDs blink alternating
 * Put quadcopter on steady surface
 * Switch transmitter on
 * LEDs blink in circular pattern. Don't move the quadcopter during this time because the calibration is ongoing.
 * When calibration is done, all LEDs blink short pulses. Quadcopter is not armed yet and will not respond to throttle.
 * Press the lower throttle trim button for one second ("LEDs off" for Hubsan firmware)
 * All LEDs are on and you are ready to fly.
 * When the LEDs start to blink during flight it's time to land because the battery is nearly empty.
With this firmware the LEDs only blink *while* the battery voltage is low, so blinking might be temporary at high throttle.

Disarm by pressing the lower throttle trim button again for one second.

Manual accelerometer calibration:
 * Quadcopter must be on level surface
 * Quadcopter must be in "not armed" state
 * Throttle stick at minimum
 * Move roll stick 3 times left and right
 * LEDs blink in circular pattern to indicate calibration process. When finished, results are stored in data flash.

#### Used development environment
 * Kubuntu 14.04 amd64
 * Eclipse IDE http://www.eclipse.org/
 * GNU ARM Eclipse plugin http://gnuarmeclipse.livius.net/blog/
 * GCC cross compiler for ARM Cortex-A/R/M (gcc-arm-none-eabi)
 * UM232H FTDI development board configured as SWD adapter
 * OpenOCD, built from latest source. Release 0.8.0 does not include SWD driver for FTDI yet. http://openocd.sourceforge.net/

#### Development issues:

When burning a firmware with new PID control parameters, checkboxconfig or anything else from the usersettings struct make sure to erase the data flash.
Otherwise the firmware will continue to use the old data.

Resetting the board using OpenOCD and the UM232H based SWD adapter does not work. Need to check with scope what's going on.

The OpenOCD Mini51 flash driver did not work for me, so I modified it.
Config file and modified source [here](https://gist.github.com/TheLastMutt/d1c1948acaace7444c1c).

#### The H107L uses the following hardware
 * Nuvoton MINI54ZAN ARM Cortex-M0
 * AMICCOM A7105 2.5GHz transceiver
 * mCube MC3210 3-Axis Accelerometer
 * InvenSense MPU-3050 3-Axis MEMS Gyroscope

Credits
======

 * Bradwii was originally coded by Brad Quick for AVR: https://github.com/bradquick/bradwii
 * Trollcop forked and ported to ARM STM32, untested: https://github.com/trollcop/bradwii
 * The Mini54ZAN ARM port to V202/JD385 was done by Victor: https://github.com/victzh/bradwii
 * The Hubsan X4 H107L port was done by Goebish: https://github.com/goebish/bradwii-X4

More information
======

 * http://www.rcgroups.com/forums/showthread.php?t=2174365
 * https://www.mikrocontroller.net/topic/309185 (German)
