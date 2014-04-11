/* 
Copyright 2014 Victor Joukov

Some of this code is based on Multiwii code by Alexandre Dubus (www.multiwii.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "hal.h"
#include "bradwii.h"
#include "rx.h"
#include "defs.h"
//#include "lib_timers.h"
//#include "lib_digitalio.h"
//#include "lib_serial.h"

// when adding new receivers, the following functions must be included:
// initrx()   // initializes the r/c receiver
// readrx()   // loads global.rxvalues with r/c values as fixedpointnum's from -1 to 1 (0 is the center).

unsigned char channelindex[] = { ROLLINDEX,PITCHINDEX,THROTTLEINDEX,YAWINDEX,AUX1INDEX,AUX2INDEX,AUX3INDEX,AUX4INDEX,8,9,10,11 };

extern globalstruct global;

void initrx(void)
{
}

void readrx(void)
{
    int chan;
    uint16_t data;

    for (chan = 0; chan < 8; ++chan) {
//        data = pwmRead(chan);
//    if (data < 750 || data > 2250)
        data = 1500;

        // convert from 1000-2000 range to -1 to 1 fixedpointnum range and low pass filter to remove glitches
        lib_fp_lowpassfilter(&global.rxvalues[channelindex[chan]], ((fixedpointnum) data - 1500) * 131L, global.timesliver, FIXEDPOINTONEOVERONESIXTYITH, TIMESLIVEREXTRASHIFT);
    }
}
