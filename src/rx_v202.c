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
#include "lib_timers.h"
#include "nrf24l01.h"
//#include "lib_digitalio.h"
//#include "lib_serial.h"

// when adding new receivers, the following functions must be included:
// initrx()   // initializes the r/c receiver
// readrx()   // loads global.rxvalues with r/c values as fixedpointnum's from -1 to 1 (0 is the center).

unsigned char channelindex[] = { ROLLINDEX,PITCHINDEX,THROTTLEINDEX,YAWINDEX,AUX1INDEX,AUX2INDEX,AUX3INDEX,AUX4INDEX,8,9,10,11 };

extern globalstruct global;

#define BV(x) (1 << (x))

#define V2x2_PAYLOAD_SIZE 16
#define V2x2_NFREQCHANNELS 16

// This is frequency hopping table for V202 protocol
// The table is the first 4 rows of 32 frequency hopping
// patterns, all other rows are derived from the first 4.
// For some reason the protocol avoids channels, dividing
// by 16 and replaces them by subtracting 3 from the channel
// number in this case.
// The pattern is defined by 5 least significant bits of
// sum of 3 bytes comprising TX id
static const uint8_t freq_hopping[][V2x2_NFREQCHANNELS] = {
 { 0x27, 0x1B, 0x39, 0x28, 0x24, 0x22, 0x2E, 0x36,
   0x19, 0x21, 0x29, 0x14, 0x1E, 0x12, 0x2D, 0x18 }, //  00
 { 0x2E, 0x33, 0x25, 0x38, 0x19, 0x12, 0x18, 0x16,
   0x2A, 0x1C, 0x1F, 0x37, 0x2F, 0x23, 0x34, 0x10 }, //  01
 { 0x11, 0x1A, 0x35, 0x24, 0x28, 0x18, 0x25, 0x2A,
   0x32, 0x2C, 0x14, 0x27, 0x36, 0x34, 0x1C, 0x17 }, //  02
 { 0x22, 0x27, 0x17, 0x39, 0x34, 0x28, 0x2B, 0x1D,
   0x18, 0x2A, 0x21, 0x38, 0x10, 0x26, 0x20, 0x1F }  //  03
};

static uint8_t rf_channels[V2x2_NFREQCHANNELS];
static uint8_t packet[V2x2_PAYLOAD_SIZE];
static uint8_t packet_sent;
static uint8_t tx_id[3];
static uint8_t rf_ch_num;
static uint32_t bind_timer;


static void initialize_beken(void)
{
    // Check for Beken BK2421/BK2423 chip
    // It is done by using Beken specific activate code, 0x53
    // and checking that status register changed appropriately
    // There is no harm to run it on nRF24L01 because following
    // closing activate command changes state back even if it
    // does something on nRF24L01
    // For detailed description of what's happening here see 
    //   http://www.inhaos.com/uploadfile/otherpic/AN0008-BK2423%20Communication%20In%20250Kbps%20Air%20Rate.pdf
    NRF24L01_Activate(0x53); // magic for BK2421/BK2423 bank switch
    printf("Trying to switch banks\n");
    if (NRF24L01_ReadReg(NRF24L01_07_STATUS) & 0x80) {
        printf("BK2421 detected\n");
        uint32_t nul = 0;
        // Beken registers don't have such nice names, so we just mention
        // them by their numbers
        // It's all magic, eavesdropped from real transfer and not even from the
        // data sheet - it has slightly different values
        NRF24L01_WriteRegisterMulti(0x00, (uint8_t *) "\x40\x4B\x01\xE2", 4);
        NRF24L01_WriteRegisterMulti(0x01, (uint8_t *) "\xC0\x4B\x00\x00", 4);
        NRF24L01_WriteRegisterMulti(0x02, (uint8_t *) "\xD0\xFC\x8C\x02", 4);
        NRF24L01_WriteRegisterMulti(0x03, (uint8_t *) "\xF9\x00\x39\x21", 4);
        NRF24L01_WriteRegisterMulti(0x04, (uint8_t *) "\xC1\x96\x9A\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x05, (uint8_t *) "\x24\x06\x7F\xA6", 4);
        NRF24L01_WriteRegisterMulti(0x06, (uint8_t *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x07, (uint8_t *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x08, (uint8_t *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x09, (uint8_t *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x0A, (uint8_t *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x0B, (uint8_t *) &nul, 4);
        NRF24L01_WriteRegisterMulti(0x0C, (uint8_t *) "\x00\x12\x73\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0D, (uint8_t *) "\x46\xB4\x80\x00", 4);
        NRF24L01_WriteRegisterMulti(0x0E, (uint8_t *) "\x41\x10\x04\x82\x20\x08\x08\xF2\x7D\xEF\xFF", 11);
        NRF24L01_WriteRegisterMulti(0x04, (uint8_t *) "\xC7\x96\x9A\x1B", 4);
        NRF24L01_WriteRegisterMulti(0x04, (uint8_t *) "\xC1\x96\x9A\x1B", 4);
    } else {
        printf("nRF24L01 detected\n");
    }
    NRF24L01_Activate(0x53); // switch bank back
}    

void initrx(void)
{
    NRF24L01_Initialize();

    // 2-bytes CRC, radio off
    uint8_t config = BV(NRF24L01_00_EN_CRC) | BV(NRF24L01_00_CRCO) | BV(NRF24L01_00_PRIM_RX);

    NRF24L01_WriteReg(NRF24L01_00_CONFIG, config); 
    NRF24L01_WriteReg(NRF24L01_01_EN_AA, 0x00);      // No Auto Acknoledgement
    NRF24L01_WriteReg(NRF24L01_02_EN_RXADDR, 0x01);  // Enable data pipe 0
    NRF24L01_WriteReg(NRF24L01_03_SETUP_AW, 0x03);   // 5-byte RX/TX address
    NRF24L01_WriteReg(NRF24L01_04_SETUP_RETR, 0xFF); // 4ms retransmit t/o, 15 tries
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, 0x08);      // Channel 8 - bind
    NRF24L01_SetBitrate(NRF24L01_BR_1M);                          // 1Mbps
    NRF24L01_SetPower(TXPOWER_100mW);
    NRF24L01_WriteReg(NRF24L01_07_STATUS, 0x70);     // Clear data ready, data sent, and retransmit
    NRF24L01_WriteReg(NRF24L01_11_RX_PW_P0, V2x2_PAYLOAD_SIZE);  // bytes of data payload for pipe 0
    NRF24L01_WriteReg(NRF24L01_17_FIFO_STATUS, 0x00); // Just in case, no real bits to write here
    uint8_t rx_tx_addr[] = {0x66, 0x88, 0x68, 0x68, 0x68};
//    uint8_t rx_p1_addr[] = {0x88, 0x66, 0x86, 0x86, 0x86};
    NRF24L01_WriteRegisterMulti(NRF24L01_0A_RX_ADDR_P0, rx_tx_addr, 5);
//    NRF24L01_WriteRegisterMulti(NRF24L01_0B_RX_ADDR_P1, rx_p1_addr, 5);
    NRF24L01_WriteRegisterMulti(NRF24L01_10_TX_ADDR, rx_tx_addr, 5);
    
    initialize_beken();

    lib_timers_delaymilliseconds(50);

    NRF24L01_FlushTx();
    NRF24L01_FlushRx();

    packet_sent = 0;
    rf_ch_num = 0;

    // Turn radio power on
    config |= BV(NRF24L01_00_PWR_UP);
    NRF24L01_WriteReg(NRF24L01_00_CONFIG, config);
    // Implicit delay in callback
    // delayMicroseconds(150);
    bind_timer = lib_timers_getcurrentmicroseconds();
    for (int i = 0; i < V2x2_NFREQCHANNELS; ++i) {
        rf_channels[i] = freq_hopping[0][i];
    }
}

static void switch_channel(void)
{
    NRF24L01_WriteReg(NRF24L01_05_RF_CH, rf_channels[rf_ch_num]);
    if (++rf_ch_num >= V2x2_NFREQCHANNELS) rf_ch_num = 0;
}

void readrx(void)
{
    int chan;
    uint16_t data;

    if (!(NRF24L01_ReadReg(NRF24L01_07_STATUS) & BV(NRF24L01_07_RX_DR))) {
        uint32_t t = lib_timers_gettimermicroseconds(bind_timer);
        if (t > 1000L) {
            switch_channel();
        }
        return;
    }
    NRF24L01_ReadPayload(packet, V2x2_PAYLOAD_SIZE);
    NRF24L01_FlushRx();
    switch_channel();
    
    for (chan = 0; chan < 8; ++chan) {
//        data = pwmRead(chan);
//    if (data < 750 || data > 2250)
        data = 1500;

        // convert from 1000-2000 range to -1 to 1 fixedpointnum range and low pass filter to remove glitches
        lib_fp_lowpassfilter(&global.rxvalues[channelindex[chan]], ((fixedpointnum) data - 1500) * 131L, global.timesliver, FIXEDPOINTONEOVERONESIXTYITH, TIMESLIVEREXTRASHIFT);
    }
    // reset the failsafe timer
    global.failsafetimer = lib_timers_starttimer();
}
