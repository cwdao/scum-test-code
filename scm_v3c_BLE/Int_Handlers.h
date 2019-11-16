#include "Memory_Map.h"
#include "rf_global_vars.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "scm3C_hardware_interface.h"
#include "scm3_hardware_interface.h"
#include "scum_radio_bsp.h"
#include "bucket_o_functions.h"
#include "fixed-point.h"

extern uint8_t send_packet[127];
extern uint8_t recv_packet[130];

unsigned int chips[100];
unsigned int chip_index = 0;
int raw_chips;
int jj;
unsigned int acfg3_val;

extern unsigned int LC_target;
extern unsigned int LC_code;
extern unsigned int IF_clk_target;
extern unsigned int IF_coarse;
extern unsigned int IF_fine;
extern unsigned int cal_iteration;
extern uint32_t ASC[38];

unsigned int num_packets_received;
unsigned int num_crc_errors;
unsigned int wrong_lengths;
unsigned int LQI_chip_errors;
unsigned int num_valid_packets_received;
unsigned int IF_estimate;

unsigned int current_thresh = 0;
extern unsigned int run_test_flag;
extern unsigned int num_packets_to_test;
signed short cdr_tau_value;

extern unsigned int HF_CLOCK_fine;
extern unsigned int HF_CLOCK_coarse;
extern unsigned int RC2M_superfine;
extern unsigned int RC2M_fine;
extern unsigned int RC2M_coarse;

unsigned int num_32k_ticks_in_100ms;
unsigned int num_2MRC_ticks_in_100ms;
unsigned int num_IFclk_ticks_in_100ms;
unsigned int num_LC_ch11_ticks_in_100ms;
unsigned int num_HFclock_ticks_in_100ms;

extern unsigned int RX_channel_codes[16];
extern unsigned int TX_channel_codes[16];
extern uint32_t optical_cal_iteration;
extern bool optical_cal_finished;

signed int SFD_timestamp = 0;
signed int SFD_timestamp_n_1 = 0;
signed int timing_correction = 0;
extern unsigned short doing_initial_packet_search;

// Timer parameters 
extern unsigned int packet_interval;
extern unsigned int radio_startup_time;
extern unsigned int expected_RX_arrival;
extern unsigned int ack_turnaround_time;
extern unsigned int guard_time;
extern unsigned short current_RF_channel;

extern unsigned short do_debug_print;

extern double temp;
extern uint8_t temp_iteration;
extern uint8_t num_temp_iterations;
extern uint32_t cumulative_count_2M, cumulative_count_32k;
uint32_t count_2M, count_LC, count_32k;
extern uint32_t count_2M_tx, count_32k_tx;
double ratio;
unsigned int code;

extern bool calibrate_LC_optical;
extern bool rftimer_LC_cal_finished;
extern uint8_t rftimer_LC_cal_iteration;
extern uint8_t num_rftimer_LC_cal_iterations;
extern uint32_t cumulative_count_LC;
extern uint16_t LC_sweep_code;
extern uint32_t LC_min_diff;

extern uint8_t coarse_code;
extern uint8_t mid_code;

int char_to_int(char c) {
	switch (c) {
		case '0':
			return 0;
		case '1':
			return 1;
		case '2':
			return 2;
		case '3':
			return 3;
		case '4':
			return 4;
		case '5':
			return 5;
		case '6':
			return 6;
		case '7':
			return 7;
		case '8':
			return 8;
		case '9':
			return 9;
		default:
			return 0;
	}
}

void UART_ISR(){	
	static char i=0;
	static char buff[4] = {0x0, 0x0, 0x0, 0x0};
	static char waiting_for_end_of_copy = 0;
	char inChar;
	int t;
	
	inChar = UART_REG__RX_DATA;
  buff[3] = buff[2];
	buff[2] = buff[1];
	buff[1] = buff[0];
	buff[0] = inChar;
	
	// If we are still waiting for the end of a load command
	if (waiting_for_end_of_copy) {
		if (inChar=='\n'){
				int j=0;
				printf("copying string of size %u to send_packet: ", i);
				for (j=0; j < i; j++) {
					printf("%c",send_packet[j]);
				}
				printf("\n");
				RFCONTROLLER_REG__TX_PACK_LEN = i;
				i = 0;
				waiting_for_end_of_copy = 0;
		} else if (i < 127) {		
			send_packet[i] = inChar;			
			i++;
		}	else {
			printf("Input exceeds maximum packet size\n");
		}
	} else { //If waiting for a command
		// Copies string from UART to send_packet
		if ( (buff[3]=='c') && (buff[2]=='p') && (buff[1]=='y') && (buff[0]==' ') ) {
			waiting_for_end_of_copy = 1;
		// Sends TX_LOAD signal to radio controller
		} else if ( (buff[3]=='l') && (buff[2]=='o') && (buff[1]=='d') && (buff[0]=='\n') ) {
		  RFCONTROLLER_REG__CONTROL = 0x1;
			printf("TX LOAD\n");
		// Sends TX_SEND signal to radio controller
	  } else if ( (buff[3]=='s') && (buff[2]=='n') && (buff[1]=='d') && (buff[0]=='\n') ) {
		  RFCONTROLLER_REG__CONTROL = 0x2;
		  printf("TX SEND\n");
		// Sends RX_START signal to radio controller
		} else if ( (buff[3]=='r') && (buff[2]=='c') && (buff[1]=='v') && (buff[0]=='\n') ) {
			printf("Recieving\n");
	    DMA_REG__RF_RX_ADDR = &recv_packet[0];
		  RFCONTROLLER_REG__CONTROL = 0x4;
		// Sends RX_STOP signal to radio controller
	  } else if ( (buff[3]=='e') && (buff[2]=='n') && (buff[1]=='d') && (buff[0]=='\n') ) {
			RFCONTROLLER_REG__CONTROL = 0x8;
			printf("RX STOP\n");
		// Sends RF_RESET signal to radio controller
		} else if ( (buff[3]=='r') && (buff[2]=='s') && (buff[1]=='t') && (buff[0]=='\n') ) {
		  RFCONTROLLER_REG__CONTROL = 0x10;
			printf("RF RESET\n");	
		// Returns the status register of the radio controller
		} else if ( (buff[3]=='s') && (buff[2]=='t') && (buff[1]=='a') && (buff[0]=='\n') ) {
		  int status = RFCONTROLLER_REG__STATUS;
			printf("status register is 0x%x\n", status);	
		
			printf("power=%d, reset=%d, %d\n",ANALOG_CFG_REG__10,ANALOG_CFG_REG__4,doing_initial_packet_search);
		
		// Initiates an ADC conversion
	  } else if ( (buff[3]=='a') && (buff[2]=='d') && (buff[1]=='c') && (buff[0]=='\n') ) {
		  ADC_REG__START = 0x1;
		  printf("starting ADC conversion\n");
		// Uses the radio timer to send TX_LOAD in 0.5s, TX_SEND in 1s, capture when SFD is sent and capture when packet is sent
		} else if ( (buff[3]=='a') && (buff[2]=='t') && (buff[1]=='x') && (buff[0]=='\n') ) {
			unsigned int t = RFTIMER_REG__COUNTER + 0x3D090;
			RFTIMER_REG__COMPARE0 = t;
		  RFTIMER_REG__COMPARE1 = t + 0x3D090;
		  printf("%x\n", RFTIMER_REG__COMPARE0);
		  printf("%x\n", RFTIMER_REG__COMPARE1);
		  RFTIMER_REG__COMPARE0_CONTROL = 0x5;
		  RFTIMER_REG__COMPARE1_CONTROL = 0x9;
		  RFTIMER_REG__CAPTURE0_CONTROL = 0x9;
		  RFTIMER_REG__CAPTURE1_CONTROL = 0x11;
		  printf("Auto TX\n");
		// Uses the radio timer to send RX_START in 0.5s, capture when SFD is received and capture when packet is received
		} else if ( (buff[3]=='a') && (buff[2]=='r') && (buff[1]=='x') && (buff[0]=='\n') ) {
			RFTIMER_REG__COMPARE0 = RFTIMER_REG__COUNTER + 0x3D090;
		  RFTIMER_REG__COMPARE0_CONTROL = 0x11;
		  RFTIMER_REG__CAPTURE0_CONTROL = 0x21;
		  RFTIMER_REG__CAPTURE1_CONTROL = 0x41;
			DMA_REG__RF_RX_ADDR = &recv_packet[0];
			printf("Auto RX\n");
		// Reset the radio timer compare and capture units
		} else if ( (buff[3]=='r') && (buff[2]=='r') && (buff[1]=='t') && (buff[0]=='\n') ) {
			RFTIMER_REG__COMPARE0_CONTROL = 0x0;
			RFTIMER_REG__COMPARE1_CONTROL = 0x0;
			RFTIMER_REG__CAPTURE0_CONTROL = 0x0;
			RFTIMER_REG__CAPTURE1_CONTROL = 0x0;
		  printf("Radio timer reset\n");				
			
		// Attempt to recover if stuck in unprogrammable mode
		} else if ( (buff[3]=='x') && (buff[2]=='x') && (buff[1]=='1') && (buff[0]=='\n') ) {
		
			//for(t=0;t<=38;t++){
			//	ASC_FPGA[t] = 0;	
			//}
	
			for(t=0;t<=38;t++){
				ASC[t] = 0;	
			}
			
			// Program analog scan chain
			analog_scan_chain_write(&ASC[0]);
			analog_scan_chain_load();
			
			// Program analog scan chain on SCM3B
			//analog_scan_chain_write_3B_fromFPGA(&ASC[0]);
			//analog_scan_chain_load_3B_fromFPGA();
			
			printf("Mote re-initialized to default\n");
			
			radio_disable_interrupts();
			
			rftimer_disable_interrupts();
			
			
		} else if ( (buff[3]=='x') && (buff[2]=='x') && (buff[1]=='2') && (buff[0]=='\n') ) {

			do_debug_print = 1;
					
		
		// Unknown command
		} else if ( (buff[3]=='d') && (buff[2]=='o') && (buff[1]=='n') && (buff[0]=='\n') ) {
			// Turn on divider
			divProgram(480, 1, 1);
		  printf("Divider on\n");
	  } else if ( (buff[3]=='d') && (buff[2]=='o') && (buff[1]=='f') && (buff[0]=='\n') ) {
			// Turn off divider
			divProgram(480, 0, 0);
		  printf("Divider off\n");
		} else if (buff[3]=='c') {
			// Change coarse code
			code = char_to_int(buff[2]) * 10 + char_to_int(buff[1]);
			LC_FREQCHANGE(code, 0, 0);
			printf("Changed coarse code to %d\n", code);
		}  else if (buff[3]=='m') {
			// Change coarse code
			code = char_to_int(buff[2]) * 10 + char_to_int(buff[1]);
			LC_FREQCHANGE(0, code, 0);
			printf("Changed mid code to %d\n", code);
		} 
		 else if (buff[3]=='f') {
			// Change coarse code
			code = char_to_int(buff[2]) * 10 + char_to_int(buff[1]);
			LC_FREQCHANGE(0, 0, code);
			printf("Changed fine code to %d\n", code);
		} else if (inChar=='\n'){
			code = char_to_int(buff[3]) * 100 + char_to_int(buff[2]) * 10 + char_to_int(buff[1]);
			printf("New code: %d\n", code);
			LC_monotonic(code);
		}
	}
}

void ADC_ISR() {
	printf("Conversion complete: 0x%x\n", ADC_REG__DATA);
}

void RF_ISR() {
	
	unsigned int interrupt = RFCONTROLLER_REG__INT;
	unsigned int error     = RFCONTROLLER_REG__ERROR;
	int t;
	
  //if (interrupt & 0x00000001) printf("TX LOAD DONE\n");
	//if (interrupt & 0x00000002) printf("TX SFD DONE\n");
	if (interrupt & 0x00000004){ //printf("TX SEND DONE\n");
		
		//printf("TXDONE\n");
		
		// Lower GPIO3 to indicate TX is off
		GPIO_REG__OUTPUT &= ~(0x8);
		
		// Packet sent; turn transmitter off
		radio_rfOff();
		
		//GPIO_REG__OUTPUT = 0xFFFF;
		//GPIO_REG__OUTPUT = 0xFFFF;
		//GPIO_REG__OUTPUT = 0x0;
		
		//printf("%d\n",RFTIMER_REG__COUNTER);
		
		// Apply frequency corrections
		radio_frequency_housekeeping();
		
		//printf("TX DONE\n");
		//printf("IF=%d, LQI=%d, CDR=%d, len=%d, SFD=%d, LC=%d\n",IF_estimate,LQI_chip_errors,cdr_tau_value,recv_packet[0],packet_interval,LC_code);
		
	}
	if (interrupt & 0x00000008){// printf("RX SFD DONE\n");
		SFD_timestamp_n_1 = SFD_timestamp;
		SFD_timestamp = RFTIMER_REG__COUNTER;
		
		//printf("%d --",SFD_timestamp);
		
		// Toggle GPIO5 to indicate a SFD was found
		GPIO_REG__OUTPUT |= 0x20;
		GPIO_REG__OUTPUT |= 0x20;
		GPIO_REG__OUTPUT &= ~(0x20);
		
		// Disable watchdog if a SFD has been found
		RFTIMER_REG__COMPARE3_CONTROL = 0x0;
		
		if(doing_initial_packet_search==1){
			// Sync next RX turn-on to expected arrival time
			RFTIMER_REG__COUNTER = expected_RX_arrival;
		}
		
	}
	if (interrupt & 0x00000010) {
		
		//int i;
		unsigned int RX_DONE_timestamp;
		char num_bytes_rec = recv_packet[0];
		//char *current_byte_rec = recv_packet+1;
		//printf("RX DONE\n");
		//printf("RX'd %d: \n", num_bytes_rec);
		//for (i=0; i < num_bytes_rec; i++) {
		//	printf("%c",current_byte_rec[i]);
		//}
		//printf("\n");
		
		// Set GPIO1 low to indicate RX is now done/off
		GPIO_REG__OUTPUT &= ~(0x2);
		
		RX_DONE_timestamp = RFTIMER_REG__COUNTER;
		
		num_packets_received++;
		
		// continuous rx for debug
		//printf("IF=%d, LQI=%d, CDR=%d, %d\n",read_IF_estimate(),read_LQI(),ANALOG_CFG_REG__25,SFD_timestamp);
		//radio_rxEnable();
		//radio_rxNow();		
			
		// Check if the packet length is as expected (20 payload bytes + 2 for CRC)	
		if(num_bytes_rec != 22){
			wrong_lengths++;
			
			// If packet was a failure, turn the radio off
			if(doing_initial_packet_search == 0){
				
				radio_rfOff();
			}
			else{
				
				// Keep listening if doing initial search
				radio_rxEnable();
				radio_rxNow();
			}
			
		}
		
		// Length was right but CRC was wrong
		else if (error == 0x00000008){
			num_crc_errors++;
			RFCONTROLLER_REG__ERROR_CLEAR = error;

			// If packet was a failure, turn the radio off
			if(doing_initial_packet_search == 0){
				
				radio_rfOff();
			}
			else{
				
				// Keep listening if doing initial search
				radio_rxEnable();
				radio_rxNow();
			}
		
		// Packet has good CRC value and is the correct length
		} else {
			
			// If this is the first valid packet received, start timers for next expected packet
			if(doing_initial_packet_search == 1){
				
				// Clear this flag
				doing_initial_packet_search = 0;
			
				// Enable compare interrupts for RX
				RFTIMER_REG__COMPARE0_CONTROL = 0x3;
				RFTIMER_REG__COMPARE1_CONTROL = 0x3;
				RFTIMER_REG__COMPARE2_CONTROL = 0x3;
				
				// Enable timer ISRs in the NVIC
				rftimer_enable_interrupts();
								
				radio_rfOff();
			}	
			// Already locked onto packet rate
			else{
			
				//printf("y\n");
				
				// Only record IF estimate, LQI, and CDR tau for valid packets
				IF_estimate = read_IF_estimate();
				LQI_chip_errors	= ANALOG_CFG_REG__21 & 0xFF; //read_LQI();	
				
				// Read the value of tau debug at end of packet
				// Do this later in the ISR to make sure this register has settled before trying to read it
				// (the register is on the adc clock domain)
				cdr_tau_value = ANALOG_CFG_REG__25;
					
				num_valid_packets_received++;
				
				// Prepare ack	
				// Data is stored in send_packet[]
				//send_packet[0] = 't';
				//send_packet[1] = 'e';
				//send_packet[2] = 's';
				//send_packet[3] = 't';
				radio_loadPacket(20);
		
				// Transmit packet at this time
				RFTIMER_REG__COMPARE5 = RX_DONE_timestamp + ack_turnaround_time;	
				
				// Turn on transmitter 
				RFTIMER_REG__COMPARE4 = RFTIMER_REG__COMPARE5 - radio_startup_time;
					
				// Enable TX timers
				RFTIMER_REG__COMPARE4_CONTROL = 0x3;
				RFTIMER_REG__COMPARE5_CONTROL = 0x3;
				
				radio_rfOff();
				
			//	printf("%d\n",RX_DONE_timestamp);
			}
			
	// Debug print output		
	//		printf("IF=%d, LQI=%d, CDR=%d, len=%d, SFD=%d, LC=%d\n",IF_estimate,LQI_chip_errors,cdr_tau_value,recv_packet[0],packet_interval,LC_code);
//			radio_rxEnable();
//			radio_rxNow();
//			rftimer_disable_interrupts();

		}
	}
	
	//if (error != 0) {
	//	if (error & 0x00000001) printf("TX OVERFLOW ERROR\n");
	//	if (error & 0x00000002) printf("TX CUTOFF ERROR\n");
	//	if (error & 0x00000004) printf("RX OVERFLOW ERROR\n");
	//	if (error & 0x00000008) printf("RX CRC ERROR\n");
	//	if (error & 0x00000010) printf("RX CUTOFF ERROR\n");
		RFCONTROLLER_REG__ERROR_CLEAR = error;
	//}
	
	RFCONTROLLER_REG__INT_CLEAR = interrupt;
}

void RFTIMER_ISR() {

// COMPARE0 @ t=0
// setFrequencyRX(channel);
// 
// Disable TX interrupts (turn them on only if RX is successful)
// RFTIMER_REG__COMPARE3_CONTROL = 0x0;
// RFTIMER_REG__COMPARE4_CONTROL = 0x0;

// Enable RX watchdog
// RFTIMER_REG__COMPARE2_CONTROL = 0x3;

// COMPARE1 (t1) = Turn on the RX 
// t0 = (expected - guard_time - radio_startup_time);
// rxEnable();

// COMPARE2 (t2) = Time to start listening for packet 
// t1 = (expected_arrival - guard_time);
// rxNow(); 

// COMPARE3 (t3) = RX watchdog 
// t2 = (t1 + 2 * guard_time);
// radio_rfOff();

// COMPARE4 (t4) = Turn on transmitter 
// t3 = (expected_RX_arrival + ack_turnaround_time - radio_startup_time);
// txEnable();

// COMPARE5 (t5) = Transmit packet 
// t4 = (expected_RX_arrival + ack_turnaround_time - radio_startup_time);
// txNow();	
  
	unsigned int interrupt = RFTIMER_REG__INT;
	
	unsigned int rdata_lsb, rdata_msb; 
	unsigned int count_HFclock, count_IF;
			
	if (interrupt & 0x00000001){ //printf("COMPARE0 MATCH\n");
		
		// Setup LO for reception
		setFrequencyRX(current_RF_channel);

		// Enable RX watchdog
		RFTIMER_REG__COMPARE3_CONTROL = 0x3;
		
		// Disable TX interrupts (turn them on only if RX is successful)
		RFTIMER_REG__COMPARE4_CONTROL = 0x0;
		RFTIMER_REG__COMPARE5_CONTROL = 0x0;
		
		// Toggle GPIO7 to indicate start of packet interval (t=0)
		GPIO_REG__OUTPUT |= 0x80;
		GPIO_REG__OUTPUT |= 0x80;
		GPIO_REG__OUTPUT &= ~(0x80);
		
	}
	if (interrupt & 0x00000002){// printf("COMPARE1 MATCH\n");
		
	// Raise GPIO1 to indicate RX is on
		GPIO_REG__OUTPUT |= 0x2;

		// Turn on the analog part of RX
		radio_rxEnable();
		
	}
	if (interrupt & 0x00000004){// printf("COMPARE2 MATCH\n");
		
//printf("rxnow-");
		
		// Start listening for packet
		radio_rxNow();
				
	}
	// Watchdog has expired - no packet received
	if (interrupt & 0x00000008){// printf("COMPARE3 MATCH\n");
					
		//printf("wd\n");
		
		GPIO_REG__OUTPUT &= ~(0x2);
		
		// Turn off the radio
		radio_rfOff();
		
	}
	// Turn on transmitter to allow frequency to settle
	if (interrupt & 0x00000010){// printf("COMPARE4 MATCH\n");
		
		
		// Raise GPIO3 to indicate TX is on
		GPIO_REG__OUTPUT |= 0x8;
		
		// Setup LO for transmit
		setFrequencyTX(current_RF_channel);
		
		// Turn on RF for TX
		radio_txEnable();
		
	}
	// LC calibration
	if (interrupt & 0x00000020){// printf("COMPARE5 MATCH\n");
		RFTIMER_REG__COMPARE5_CONTROL = 0x00;

		// Disable all counters
		ANALOG_CFG_REG__0 = 0x007F;

		// Read LC_div counter (via counter4)
		rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x280000);
		rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x2C0000);
		count_LC = rdata_lsb + (rdata_msb << 16);

		// Reset all counters
		ANALOG_CFG_REG__0 = 0x0000;

		// Enable all counters
		ANALOG_CFG_REG__0 = 0x3FFF;

		if (rftimer_LC_cal_iteration == num_rftimer_LC_cal_iterations) {
			count_LC = cumulative_count_LC / num_rftimer_LC_cal_iterations;
			rftimer_LC_cal_iteration = 0;
			cumulative_count_LC = 0;

			if ((count_LC <= LC_target) && (LC_target - count_LC < LC_min_diff)) {
				LC_min_diff = LC_target - count_LC;
				coarse_code = (LC_sweep_code >> 10) & 0x1F;
				mid_code = (LC_sweep_code >> 5) & 0x1F;
			} else if ((count_LC > LC_target) && (count_LC - LC_target < LC_min_diff)) {
				LC_min_diff = count_LC - LC_target;
				coarse_code = (LC_sweep_code >> 10) & 0x1F;
				mid_code = (LC_sweep_code >> 5) & 0x1F;
			}

			printf("count_LC: %u, LC_target: %u, LC_min_diff: %u\n", count_LC, LC_target, LC_min_diff);

			LC_sweep_code += (1 << 5);
			LC_FREQCHANGE((LC_sweep_code >> 10) & 0x1F,
										(LC_sweep_code >> 5) & 0x1F,
										LC_sweep_code & 0x1F);

			if (LC_sweep_code >= (25U << 10)) {
				printf("coarse code: %u, mid code: %u\n", coarse_code, mid_code);

				// Disable this ISR
				ICER = 0x0800;
				rftimer_LC_cal_finished = true;

				// Halt all counters
				ANALOG_CFG_REG__0 = 0x0000;
			} else {
				// Reset the interrupt
				RFTIMER_REG__CONTROL = 0x7;
				RFTIMER_REG__MAX_COUNT = 0x0000C350;
				RFTIMER_REG__COMPARE5 = 0x0000C350;
				RFTIMER_REG__COMPARE5_CONTROL = 0x03;

				// Reset all counters
				ANALOG_CFG_REG__0 = 0x0000;

				// Enable all counters
				ANALOG_CFG_REG__0 = 0x3FFF;
			}
		} else {
			++rftimer_LC_cal_iteration;
			cumulative_count_LC += count_LC;

			// Reset the interrupt
			RFTIMER_REG__CONTROL = 0x7;
			RFTIMER_REG__MAX_COUNT = 0x0000C350;
			RFTIMER_REG__COMPARE5 = 0x0000C350;
			RFTIMER_REG__COMPARE5_CONTROL = 0x03;

			// Reset all counters
			ANALOG_CFG_REG__0 = 0x0000;

			// Enable all counters
			ANALOG_CFG_REG__0 = 0x3FFF;
		}
	}
	if (interrupt & 0x00000040) {//printf("COMPARE6 MATCH\n");
		RFTIMER_REG__COMPARE6_CONTROL = 0x00;
		
		read_counters(&count_2M, &count_LC, &count_32k);
		
		printf("count_2M: %u, count_32k: %u, temp_iteration: %u\n", count_2M, count_32k, temp_iteration);
		
		if (temp_iteration == num_temp_iterations) {
			// Disable this interrupt
			ICER = 0x0080;
			
			ratio = fix_double(fix_div(fix_init(cumulative_count_2M), fix_init(cumulative_count_32k)));
			
			// Calculate temperature based on average ratio
			temp = -30.715 * ratio + 1915.142;
			printf("Temp: %d\n", (int)(temp * 100));
			
			// Transmit 2M and 32k counters
			count_2M_tx = cumulative_count_2M / num_temp_iterations;
			count_32k_tx = cumulative_count_32k / num_temp_iterations;
			printf("count_2M: %d, count_32k: %d\n", count_2M_tx, count_32k_tx);

			temp_iteration = 0;
			cumulative_count_2M = 0;
			cumulative_count_32k = 0;
		} else {
			++temp_iteration;
			cumulative_count_2M += count_2M;
			cumulative_count_32k += count_32k;
			
			RFTIMER_REG__CONTROL = 0x7;
			RFTIMER_REG__MAX_COUNT = 0x0000C350;
			RFTIMER_REG__COMPARE6 = 0x0000C350;
			RFTIMER_REG__COMPARE6_CONTROL = 0x03;

			// Reset all counters
			ANALOG_CFG_REG__0 = 0x0000;

			// Enable all counters
			ANALOG_CFG_REG__0 = 0x3FFF;
		}
	}
	if (interrupt & 0x00000080) {//printf("COMPARE7 MATCH\n");
		// Disable this interrupt
		ICER = 0x0080;
		
		RFTIMER_REG__COMPARE7_CONTROL = 0x00;
	
		// Disable all counters
		ANALOG_CFG_REG__0 = 0x007F;
		
		// Read 32k counter
		rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x000000);
		rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x040000);
		count_32k = rdata_lsb + (rdata_msb << 16);

		// Read 2M counter
		rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x180000);
		rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x1C0000);
		count_2M = rdata_lsb + (rdata_msb << 16);
		
		printf("count_32k: %d, count_2M: %d\n", count_32k, count_2M);
	}
	//if (interrupt & 0x00000100) printf("CAPTURE0 TRIGGERED AT: 0x%x\n", RFTIMER_REG__CAPTURE0);
	//if (interrupt & 0x00000200) printf("CAPTURE1 TRIGGERED AT: 0x%x\n", RFTIMER_REG__CAPTURE1);
	//if (interrupt & 0x00000400) printf("CAPTURE2 TRIGGERED AT: 0x%x\n", RFTIMER_REG__CAPTURE2);
	//if (interrupt & 0x00000800) printf("CAPTURE3 TRIGGERED AT: 0x%x\n", RFTIMER_REG__CAPTURE3);
	//if (interrupt & 0x00001000) printf("CAPTURE0 OVERFLOW AT: 0x%x\n", RFTIMER_REG__CAPTURE0);
	//if (interrupt & 0x00002000) printf("CAPTURE1 OVERFLOW AT: 0x%x\n", RFTIMER_REG__CAPTURE1);
	//if (interrupt & 0x00004000) printf("CAPTURE2 OVERFLOW AT: 0x%x\n", RFTIMER_REG__CAPTURE2);
	//if (interrupt & 0x00008000) printf("CAPTURE3 OVERFLOW AT: 0x%x\n", RFTIMER_REG__CAPTURE3);
	
	RFTIMER_REG__INT_CLEAR = interrupt;
}


// This ISR goes off when the raw chip shift register gets 32 new bits shifted in
void RAWCHIPS_32_ISR() {
	
	unsigned int jj;
	unsigned int rdata_lsb, rdata_msb;
	
	// Read 32bit val
	rdata_lsb = ANALOG_CFG_REG__17;
	rdata_msb = ANALOG_CFG_REG__18;
	chips[chip_index] = rdata_lsb + (rdata_msb << 16);	
		
	
	//printf("0x%X\n",chips[chip_index]);
	
	chip_index++;

	if(chip_index == 7){	

	// Target packet - 214 bits		
	// As transmitted
	//0x1D55ADF6
	//0x45C7C50E
	//0x2613C2AC
	//0x9837B830
	//0xA1C9E493
	//0x75B7416C
	//0xD12DB8XX
	
	// Stripping off the preamble (the first 14 bits in the above)
	//0x6B7D9171
	//0xF1438984
	//0xF0AB260D
	//0xEE0C2872
	//0x7924DD6D
	//0xD05B344B
	//0x6EXXXXXX
				
	// Check that there were no bit errors in packet
	if((chips[0] == 0x6B7D9171) && (chips[1] == 0xF1438984) && (chips[2] == 0xF0AB260D) &&
		(chips[3] == 0xEE0C2872) && (chips[4] == 0x7924DD6D) && (chips[5] == 0xD05B344B) &&
	((chips[6] & 0xFF000000) == 0x6E000000)){
		num_valid_packets_received++;
		
		printf("BLE PACKET RX!\n");
	}
		
		
	// Set NVIC back up to receive another packet
	ICER = 0x0200;
	ICPR = 0x0300;
	ISER = 0x0100;
	chip_index = 0;
		
	}
	
	// Clear the interrupt
	acfg3_val |= 0x60;
	ANALOG_CFG_REG__3 = acfg3_val;
	acfg3_val &= ~(0x60);
	ANALOG_CFG_REG__3 = acfg3_val;
}



// With HCLK = 5MHz, data rate of 1.25MHz tested OK
// For faster data rate, will need to raise the HCLK frequency
// This ISR goes off when the input register matches the target value
void RAWCHIPS_STARTVAL_ISR() {
	
	unsigned int rdata_lsb, rdata_msb;
	
	int t;
	
	//printf("startval!\n");
	
	//for(t=0;t<10;t++);
	
	// Read 32bit val
	rdata_lsb = ANALOG_CFG_REG__17;
	rdata_msb = ANALOG_CFG_REG__18;
	chips[chip_index] = rdata_lsb + (rdata_msb << 16);
	
	printf("0x%X\n",chips[chip_index]);
	
	chip_index++;
	
	// Clear all interrupts
	acfg3_val |= 0x60;
	ANALOG_CFG_REG__3 = acfg3_val;
	acfg3_val &= ~(0x60);
	ANALOG_CFG_REG__3 = acfg3_val;
	
	// Enable the interrupt for the 32bit 
	ICER = 0x0100;
	ICPR = 0x0300;
	ISER = 0x0200;
	
	// RST_B = 0 (it is active low)
	ANALOG_CFG_REG__4 = 0x2000;	
	ANALOG_CFG_REG__4 = 0x2800;
		
}

// This interrupt goes off every time 32 new bits of data have been shifted into the optical register
// Do not recommend trying to do any CPU intensive actions while trying to receive optical data
// ex, printf will mess up the received data values
void OPTICAL_32_ISR(){
	//printf("Optical 32-bit interrupt triggered\n");
	
	//unsigned int LSBs, MSBs, optical_shiftreg;
	//int t;
	
	// 32-bit register is analog_rdata[335:304]
	//LSBs = ANALOG_CFG_REG__19; //16 LSBs
	//MSBs = ANALOG_CFG_REG__20; //16 MSBs
	//optical_shiftreg = (MSBs << 16) + LSBs;	
	
	// Toggle GPIO 0
	//GPIO_REG__OUTPUT ^= 0x1;
	
}

// This interrupt goes off when the optical register holds the value {221, 176, 231, 47}
// This interrupt can also be used to synchronize to the start of an optical data transfer
// Need to make sure a new bit has been clocked in prior to returning from this ISR, or else it will immediately execute again
void OPTICAL_SFD_ISR(){
	
	unsigned int rdata_lsb, rdata_msb;
	unsigned int count_LC, count_32k, count_2M, count_HFclock, count_IF;
		
	// Disable all counters
	ANALOG_CFG_REG__0 = 0x007F;
	
	// Keep track of how many calibration iterations have been completed
	optical_cal_iteration++;
		
	// Read 32k counter
	rdata_lsb = *(uint32_t*)(APB_ANALOG_CFG_BASE + 0x000000);
	rdata_msb = *(uint32_t*)(APB_ANALOG_CFG_BASE + 0x040000);
	count_32k = rdata_lsb + (rdata_msb << 16);

	// Read HF_CLOCK counter
	rdata_lsb = *(uint32_t*)(APB_ANALOG_CFG_BASE + 0x100000);
	rdata_msb = *(uint32_t*)(APB_ANALOG_CFG_BASE + 0x140000);
	count_HFclock = rdata_lsb + (rdata_msb << 16);

	// Read 2M counter
	rdata_lsb = *(uint32_t*)(APB_ANALOG_CFG_BASE + 0x180000);
	rdata_msb = *(uint32_t*)(APB_ANALOG_CFG_BASE + 0x1C0000);
	count_2M = rdata_lsb + (rdata_msb << 16);
		
	// Read LC_div counter (via counter4)
	rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x280000);
	rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x2C0000);
	count_LC = rdata_lsb + (rdata_msb << 16);
	
	// Read IF ADC_CLK counter
	rdata_lsb = *(uint32_t*)(APB_ANALOG_CFG_BASE + 0x300000);
	rdata_msb = *(uint32_t*)(APB_ANALOG_CFG_BASE + 0x340000);
	count_IF = rdata_lsb + (rdata_msb << 16);
	
	// Reset all counters
	ANALOG_CFG_REG__0 = 0x0000;	
	
	// Enable all counters
	ANALOG_CFG_REG__0 = 0x3FFF;
		
	// Don't make updates on the first two executions of this ISR
	if (optical_cal_iteration > 2) {
		
		// Do correction on HF CLOCK
		// Fine DAC step size is about 6000 counts
		if(count_HFclock < 1997000) HF_CLOCK_fine--;
		if(count_HFclock > 2003000) HF_CLOCK_fine++;
		set_sys_clk_secondary_freq(HF_CLOCK_coarse, HF_CLOCK_fine);
		
		// Do correction on LC
		/*
		if(count_LC > (LC_target + 0)) LC_code -= 1;
		if(count_LC < (LC_target - 0))	LC_code += 1;
		LC_monotonic(LC_code);
		code = LC_code;
		*/
			
		// Do correction on 2M RC
		// Coarse step ~1100 counts, fine ~150 counts, superfine ~25
		// Too fast
		if(count_2M > (200600)) RC2M_coarse += 1;
		else if(count_2M > (200080)) RC2M_fine += 1;
		else if(count_2M > (200015))	RC2M_superfine += 1;
		
		// Too slow
		if(count_2M < (199400))	RC2M_coarse -= 1;
		else if(count_2M < (199920)) RC2M_fine -= 1;
		else if(count_2M < (199985))	RC2M_superfine -= 1;
		set_2M_RC_frequency(31, 31, RC2M_coarse, RC2M_fine, RC2M_superfine);

		// Do correction on IF RC clock
		// Fine DAC step size is ~2800 counts
		if(count_IF > (IF_clk_target+1400)) IF_fine += 1;
		if(count_IF < (IF_clk_target-1400))	IF_fine -= 1;
		set_IF_clock_frequency(IF_coarse, IF_fine, 0);
		
		analog_scan_chain_write(&ASC[0]);
		analog_scan_chain_load();
	}
	
	printf("%u\n", optical_cal_iteration);
	// Debugging output
	// printf("HF=%d-%d   2M=%d-%d,%d,%d   LC=%d-%d   IF=%d-%d\n",count_HFclock,HF_CLOCK_fine,count_2M,RC2M_coarse,RC2M_fine,RC2M_superfine,count_LC,LC_code,count_IF,IF_fine); 

	if (calibrate_LC_optical) {
		if ((count_LC <= LC_target) && (LC_target - count_LC < LC_min_diff)) {			
			LC_min_diff = LC_target - count_LC;
			coarse_code = (LC_sweep_code >> 10) & 0x1F;
			mid_code = (LC_sweep_code >> 5) & 0x1F;
		} else if ((count_LC > LC_target) && (count_LC - LC_target < LC_min_diff)) {
			LC_min_diff = count_LC - LC_target;
			coarse_code = (LC_sweep_code >> 10) & 0x1F;
			mid_code = (LC_sweep_code >> 5) & 0x1F;
		}
		
		printf("count_LC: %u, LC_target: %u, LC_min_diff: %u\n", count_LC, LC_target, LC_min_diff);
		
		LC_sweep_code += (1 << 5);
		LC_FREQCHANGE((LC_sweep_code >> 10) & 0x1F,
		              (LC_sweep_code >> 5) & 0x1F,
		              LC_sweep_code & 0x1F);
	}
	
	if ((!calibrate_LC_optical && optical_cal_iteration >= 20) || (calibrate_LC_optical && LC_sweep_code >= (25U << 10))) {
		if (calibrate_LC_optical) {
			printf("coarse code: %u, mid code: %u\n", coarse_code, mid_code);
		}

		// Disable this ISR
		ICER = 0x0800;
		optical_cal_iteration = 0;
		optical_cal_finished = true;
		
		// Store the last count values
		num_32k_ticks_in_100ms = count_32k;
		num_2MRC_ticks_in_100ms = count_2M;
		num_IFclk_ticks_in_100ms = count_IF;
		num_LC_ch11_ticks_in_100ms = count_LC;
		num_HFclock_ticks_in_100ms = count_HFclock;
		
		// Update the expected packet rate based on the measured HF clock frequency
		// Have an estimate of how many 20MHz clock ticks there are in 100ms
		// But need to know how many 20MHz/40 = 500kHz ticks there are in 125ms (if doing 8 Hz packet rate)
		// (125 / 100) / 40 = 1/32
		packet_interval = num_HFclock_ticks_in_100ms >> 5;
	
		//printf("LC_code=%d\n", LC_code);
		//printf("IF_fine=%d\n", IF_fine);
		
		printf("done\n");
				
		//printf("Building channel table...");
		
		//build_channel_table(LC_code);
		
		//printf("done\n");
		
		//radio_disable_all();
		
		// Halt all counters
		ANALOG_CFG_REG__0 = 0x0000;
				
		// Turn off clocks
		// Disable 32k
		//clear_asc_bit(623);
		//analog_scan_chain_write(&ASC[0]);
		//analog_scan_chain_load();	
		
	}
}
	
	
// ISRs for external interrupts
void INTERRUPT_GPIO3_ISR(){
	printf("External Interrupt GPIO3 triggered\n");
}
void INTERRUPT_GPIO8_ISR(){
	printf("External Interrupt GPIO8 triggered\n");
}
void INTERRUPT_GPIO9_ISR(){
	printf("External Interrupt GPIO9 triggered\n");
}
void INTERRUPT_GPIO10_ISR(){
	printf("External Interrupt GPIO10 triggered\n");
}
