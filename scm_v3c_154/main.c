//------------------------------------------------------------------------------
// u-robot Digital Controller Firmware
//------------------------------------------------------------------------------

#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include <rt_misc.h>
#include <stdlib.h>
#include "Memory_map.h"
#include "Int_Handlers.h"
#include "rf_global_vars.h"
#include "scm3C_hardware_interface.h"
#include "scm3_hardware_interface.h"
#include "bucket_o_functions.h"
#include <math.h>
#include "scum_radio_bsp.h"

extern unsigned int current_lfsr;

extern char send_packet[127];
extern unsigned int ASC[38];

// Bootloader will insert length and pre-calculated CRC at these memory addresses	
#define crc_value         (*((unsigned int *) 0x0000FFFC))
#define code_length       (*((unsigned int *) 0x0000FFF8))

// Target radio LO freq = 2.4055G
// Divide ratio is currently 480*2
// Calibration counts for 100ms
unsigned int LC_target = 250572;
unsigned int LC_code = 690;

// HF_CLOCK tuning settings
unsigned int HF_CLOCK_fine = 17;
unsigned int HF_CLOCK_coarse = 3;

// RC 2MHz tuning settings
// This the transmitter chip clock
unsigned int RC2M_coarse = 21;
unsigned int RC2M_fine = 15;
unsigned int RC2M_superfine = 15;

// Receiver clock settings
// The receiver chip clock is derived from this clock
unsigned int IF_clk_target = 1600000;
unsigned int IF_coarse = 22;
unsigned int IF_fine = 18;

unsigned int cal_iteration = 0;
unsigned int run_test_flag = 0;
unsigned int num_packets_to_test = 1;

unsigned short optical_cal_iteration = 0, optical_cal_finished = 0;

unsigned short doing_initial_packet_search;
unsigned short current_RF_channel;
unsigned short do_debug_print = 0;

double temp = 20;
uint8_t temp_iteration = 0;
uint32_t cumulative_count_2M = 0;
uint32_t cumulative_count_32k = 0;

void measure_temperature() {
	// RFCONTROLLER_REG__INT_CONFIG = 0x3FF;   // Enable all interrupts and pulses to radio timer
	// RFCONTROLLER_REG__ERROR_CONFIG = 0x1F;  // Enable all errors
	
	temp_iteration = 0;
	cumulative_count_2M = 0;
	cumulative_count_32k = 0;
	
	// Enable RF timer interrupt
	ISER = 0x0080;

	RFTIMER_REG__CONTROL = 0x7;
	RFTIMER_REG__MAX_COUNT = 0x0000C350;
	RFTIMER_REG__COMPARE6 = 0x0000C350;
	RFTIMER_REG__COMPARE6_CONTROL = 0x03;

	// Reset all counters
	ANALOG_CFG_REG__0 = 0x0000;

	// Enable all counters
	ANALOG_CFG_REG__0 = 0x3FFF;
}

void test_LC_sweep_tx(void) {
	/*
	Inputs:
	Outputs:
	Note:
		Initialization of the scan settings should already have 
		been performed.
	*/

	uint8_t tx_iteration = 0;
	uint8_t iterations = 32;
	uint32_t i, j;

	// Set the LC frequency
	// For Q4 with PA set to 63
	// LC_FREQCHANGE(23&0x1F, 8&0x1F, 11&0x1F); // channel 11
	// LC_FREQCHANGE(23&0x1F, 15&0x1F, 6&0x1F); // channel 12
	// LC_FREQCHANGE(26&0x1F, 10&0x1F, 9&0x1F); // channel 20

	// For Q4 with PA set to 127
	LC_FREQCHANGE(23&0x1F, 8&0x1F, 9&0x1F); // channel 11

	// For QX7 with PA set to 127
	// LC_FREQCHANGE(23&0x1F, 27&0x1F, 23&0x1F); // channel 11

	// For flexboard on paper with PA set to 63
	// LC_FREQCHANGE(23&0x1F, 12&0x1F, 13&0x1F); // channel 11

	// For flexboard on paper with PA set to 127
	// LC_FREQCHANGE(23&0x1F, 12&0x1F, 16&0x1F); // channel 11
	// LC_FREQCHANGE(28&0x1F, 12&0x1F, 14&0x1F); // channel 26

	// Enable the TX. NB: Requires 50us for frequency settling
	// transient.
	radio_txEnable();
	
	while (1) {
		if (tx_iteration == 5) {
			measure_temperature();
			tx_iteration = 0;
		}

		// Send bits out the radio multiple times for redundancy
		for(i = 0; i < iterations; ++i) {

			// LC_FREQCHANGE(23&0x1F, 12&0x1F, i&0x1F); // channel 11

			// TODO: Wait for at least 50us
			for (j = 0; j < 5000; ++j) {}

			// Construct the packet
			// with payload {coarse, mid, fine} in
			// separate bytes
			send_packet[0] = i & 0x1F;
			send_packet[1] = (uint8_t) temp;
			send_packet[2] = (uint8_t) ((uint32_t) (temp * 100) % 100);
			radio_loadPacket(3);

			/*
			for (j=0; j<5; ++j) {
				send_packet[j] = 0x5E;
			}
			radio_loadPacket(5);
			*/

			radio_txNow();
		}

		// Wait before next LC frequency
		for (j=0; j<10000; ++j) {}

		tx_iteration++;
	}
}

//////////////////////////////////////////////////////////////////
// Main Function
//////////////////////////////////////////////////////////////////

int main(void) {
	int t, t2, x;
	unsigned int calc_crc;

	unsigned int rdata_lsb, rdata_msb, count_LC, count_32k, count_2M;
	
	printf("Initializing...");
		
	// Set up mote configuration
	// This function handles all the analog scan chain setup
	initialize_mote();
		
	// Check CRC to ensure there were no errors during optical programming
	printf("\n-------------------\n");
	printf("Validating program integrity..."); 
	
	calc_crc = crc32c(0x0000, code_length);

	if(calc_crc == crc_value){
		printf("CRC OK\n");
	}
	else{
		printf("\nProgramming Error - CRC DOES NOT MATCH - Halting Execution\n");
		while(1);
	}
	
	// Debug output
	//printf("\nCode length is %u bytes",code_length); 
	//printf("\nCRC calculated by SCM is: 0x%X",calc_crc);	
	
	//printf("done\n");
	
	// After bootloading the next thing that happens is frequency calibration using optical
	printf("Calibrating frequencies...\n");
	
	// Initial frequency calibration will tune the frequencies for HCLK, the RX/TX chip clocks, and the LO
	// For the LO, calibration for RX channel 11, so turn on AUX, IF, and LO LDOs
	// Aux is inverted (0 = on)
	// Memory-mapped LDO control
	// ANALOG_CFG_REG__10 = AUX_EN | DIV_EN | PA_EN | IF_EN | LO_EN | PA_MUX | IF_MUX | LO_MUX
	// For MUX signals, '1' = FSM control, '0' = memory mapped control
	// For EN signals, '1' = turn on LDO
	// Turn on LO, DIV, PA
	ANALOG_CFG_REG__10 = 0x68;
	
	// Turn off polyphase and disable mixer
	ANALOG_CFG_REG__16 = 0x6;
	
	// Enable optical SFD interrupt for optical calibration
	ISER = 0x0800;
	
	// Wait for optical cal to finish
	while(optical_cal_finished == 0);
	optical_cal_finished = 0;

	printf("Cal complete\n");
	
	// Disable divider to save power
	divProgram(480, 0, 0);
	
	// Enable UART interrupt
	ISER = 0x0001;
	
	test_LC_sweep_tx();
}
