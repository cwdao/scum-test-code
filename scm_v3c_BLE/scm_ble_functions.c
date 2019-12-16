#include <stdio.h>
#include <stdint.h>
#include "Memory_Map.h"
#include "scm3C_hardware_interface.h"
#include "scm3_hardware_interface.h"
#include "bucket_o_functions.h"
#include "scum_radio_bsp.h"

// assorted BLE packet assembly globals
const uint8_t BPREAMBLE = 0x55;
const uint8_t BACCESS_ADDRESS1 = 0x6B; // need to split BACCESS_ADDRESS into bytes to avoid big-/little-endianness issue
const uint8_t BACCESS_ADDRESS2 = 0x7D;
const uint8_t BACCESS_ADDRESS3 = 0x91;
const uint8_t BACCESS_ADDRESS4 = 0x71;

const uint8_t PDU_HEADER1 = 0x40;
const uint8_t PDU_HEADER2 = 0xA4; // pdu is 37 bytes long (6 bytes adv address + 7 bytes name + 4 bytes LC freq + 10 bytes count_32k + 4 bytes temp + 6 bytes data)
const uint8_t NAME_HEADER1 = 0x60; // name is 6 bytes long (1 byte GAP code + 5 bytes data)
const uint8_t NAME_HEADER2 = 0x10;
const uint8_t LC_FREQ_HEADER1 = 0xC0; // LC freq is 3 bytes long (1 byte GAP code + 2 bytes data)
const uint8_t LC_FREQ_HEADER2 = 0x03; // custom GAP code for LC freq (0xC0 LSB first)
const uint8_t COUNTERS_HEADER1 = 0x90; // counters is 9 bytes long (1 byte GAP code + 8 bytes data)
const uint8_t COUNTERS_HEADER2 = 0x43; // custom GAP code for counters (0xC2 LSB first)
const uint8_t TEMP_HEADER1 = 0xC0; // temp is 3 bytes long (1	byte GAP code + 2 bytes data)
const uint8_t TEMP_HEADER2 = 0x83; // custom GAP code for temperature (0xC1 LSB first)
const uint8_t DATA_HEADER1 = 0xA0; // data is 5 bytes long (1 byte GAP code + 4 bytes data)
const uint8_t DATA_HEADER2 = 0x0B; // custom defined for data (0xD0 LSB first)

const uint8_t PDU_LENGTH = 39; // pdu is 39 bytes long
const uint8_t ADVA_LENGTH = 6; // adv address is 6 bytes long
const uint8_t DATA_LENGTH = 4; // data is 4 bytes long
const uint8_t CRC_LENGTH = 3; // crc is 3 bytes long

extern uint32_t ASC[38];

extern unsigned int LC_code;
extern unsigned int IF_coarse;
extern unsigned int IF_fine;

extern unsigned int HF_CLOCK_fine;
extern unsigned int HF_CLOCK_coarse;
extern unsigned int RC2M_coarse;
extern unsigned int RC2M_fine;
extern unsigned int RC2M_superfine;

void gen_ble_packet(uint8_t *packet, uint8_t *AdvA, uint8_t channel, uint16_t LC_freq, double temp, uint32_t count_2M, uint32_t count_32k) {
	
	double k_temp; // Temperature in Kelvin
	uint16_t temp_payload;
	
	int byte1;
	int i;
	int bit2;
	uint8_t common;
	uint8_t crc3 = 0xAA;
	uint8_t crc2 = 0xAA;
	uint8_t crc1 = 0xAA;
	
	uint8_t pdu_crc[PDU_LENGTH + CRC_LENGTH];
	uint8_t *pdu_pointer = pdu_crc;
	uint8_t *crc_pointer = pdu_crc;
	uint8_t *byte_addr = pdu_crc;
	
	uint8_t lfsr = (channel & 0x3F) | (1 << 6); // [1 channel[6]]
		
	*packet = BPREAMBLE;
	packet++;
	
	*packet = BACCESS_ADDRESS1;
	packet++;
	*packet = BACCESS_ADDRESS2;
	packet++;
	*packet = BACCESS_ADDRESS3;
	packet++;
	*packet = BACCESS_ADDRESS4;
	packet++;
	
	*pdu_pointer = PDU_HEADER1;
	pdu_pointer++;
	*pdu_pointer = PDU_HEADER2;
	pdu_pointer++;
	
	for (byte1 = ADVA_LENGTH - 1; byte1 >= 0; --byte1) {
		*pdu_pointer = flipChar(AdvA[byte1]);
		pdu_pointer++;
	}
	
	*pdu_pointer = NAME_HEADER1;
	pdu_pointer++;
	*pdu_pointer = NAME_HEADER2;
	pdu_pointer++;
	
	*pdu_pointer = flipChar(0x53); // S
	pdu_pointer++;
	*pdu_pointer = flipChar(0x43); // C
	pdu_pointer++;
	*pdu_pointer = flipChar(0x55); // U
	pdu_pointer++;
	*pdu_pointer = flipChar(0x4D); // M
	pdu_pointer++;
	*pdu_pointer = flipChar(0x33); // 3
	pdu_pointer++;
	
	*pdu_pointer = LC_FREQ_HEADER1;
	pdu_pointer++;
	*pdu_pointer = LC_FREQ_HEADER2;
	pdu_pointer++;
	*pdu_pointer = flipChar((LC_freq >> 8) & 0xFF); // LC freq MSB
	pdu_pointer++;
	*pdu_pointer = flipChar(LC_freq & 0xFF); // LC freq LSB
	pdu_pointer++;

	*pdu_pointer = COUNTERS_HEADER1;
	pdu_pointer++;
	*pdu_pointer = COUNTERS_HEADER2;
	pdu_pointer++;
	*pdu_pointer = flipChar((count_2M >> 24) & 0xFF); // count_2M MSB
	pdu_pointer++;
	*pdu_pointer = flipChar((count_2M >> 16) & 0xFF);
	pdu_pointer++;
	*pdu_pointer = flipChar((count_2M >> 8) & 0xFF);
	pdu_pointer++;
	*pdu_pointer = flipChar(count_2M & 0xFF); // count_2M LSB
	pdu_pointer++;
	*pdu_pointer = flipChar((count_32k >> 24) & 0xFF); // count_32k MSB
	pdu_pointer++;
	*pdu_pointer = flipChar((count_32k >> 16) & 0xFF);
	pdu_pointer++;
	*pdu_pointer = flipChar((count_32k >> 8) & 0xFF);
	pdu_pointer++;
	*pdu_pointer = flipChar(count_32k & 0xFF); // count_32k LSB
	pdu_pointer++;

	*pdu_pointer = TEMP_HEADER1;
	pdu_pointer++;
	*pdu_pointer = TEMP_HEADER2;
	pdu_pointer++;

	k_temp = temp + 273.15; // Temperature in Kelvin
	temp_payload = 100 * k_temp + 1; // Floating point error

	*pdu_pointer = flipChar((temp_payload >> 8) & 0xFF); // Temperature MSB
	pdu_pointer++;
	*pdu_pointer = flipChar(temp_payload & 0xFF); // Temperature LSB
	pdu_pointer++;
	
	*pdu_pointer = DATA_HEADER1;
	pdu_pointer++;
	*pdu_pointer = DATA_HEADER2;
	pdu_pointer++;
	
	for (i = 0; i < DATA_LENGTH; ++i) {
		// Fill with zeros for now
		*pdu_pointer = 0x00;
		pdu_pointer++;
	}

	crc_pointer = pdu_pointer;
	byte_addr = pdu_crc;
	
	for (i = 0; i < PDU_LENGTH; ++byte_addr, ++i) {
		for (bit2 = 7; bit2 >= 0; --bit2) {
			common = (crc1 & 1) ^ ((*byte_addr & (1 << bit2)) >> bit2);
			crc1 = (crc1 >> 1) | ((crc2 & 1) << 7);
			crc2 = ((crc2 >> 1) | ((crc3 & 1) << 7)) ^ (common << 6) ^ (common << 5);
			crc3 = ((crc3 >> 1) | (common << 7)) ^ (common << 6) ^ (common << 4) ^ (common << 3) ^ (common << 1);
		}
	}
	
	crc1 = flipChar(crc1);
	crc2 = flipChar(crc2);
	crc3 = flipChar(crc3);
	
	*(crc_pointer) = crc1;
	*(crc_pointer + 1) = crc2;
	*(crc_pointer + 2) = crc3;
	
	byte_addr = pdu_crc;
	
	for (i = 0; i < PDU_LENGTH + CRC_LENGTH; ++byte_addr, ++i) {
		for (bit2 = 7; bit2 >= 0; --bit2) {
			*byte_addr = (*byte_addr & ~(1 << bit2)) | ((*byte_addr & (1 << bit2)) ^ ((lfsr & 1) << bit2));
			lfsr = ((lfsr >> 1) | ((lfsr & 1) << 6)) ^ ((lfsr & 1) << 2);
		}
	}

	byte_addr = pdu_crc;
	for (i = 0; i < PDU_LENGTH + CRC_LENGTH; ++byte_addr, ++i) {
		*packet = *byte_addr;
		packet++;
	}
}

void radio_init_tx_BLE(){
	// Set up BLE modulation source
	// ----
	// mod_logic<3:0> = ASC<996:999>
	// The two LSBs change the mux from cortex mod (00) source to pad (11)
	// They can also be used to set the modulation to 0 (10) or 1 (01)
	// The other bits are used for inverting the modulation bitstream, and can be cleared
	// The goal is to remove the 15.4 DAC from the modulation path
	/*
	set_asc_bit(997);
	clear_asc_bit(996);
	clear_asc_bit(998);
	clear_asc_bit(999);
	*/
	clear_asc_bit(997);
	set_asc_bit(996);
	clear_asc_bit(998);
	clear_asc_bit(999);
	
	clear_asc_bit(1000);
	clear_asc_bit(1001);
	clear_asc_bit(1002);
	clear_asc_bit(1003);
	
	// Make sure the BLE modulation mux is set
	// Bit 1013 sets the BLE mod dac to cortex control
	// The BLE tone spacing cannot be set, it is a fixed DAC
	set_asc_bit(1013);
	// ----
	
	// Set the bypass scan bits of the BLE FIFO and GFSK modules
	set_asc_bit(1041);
	set_asc_bit(1042);
	
	// Ensure cortex control of LO
	clear_asc_bit(964);
	
	// Ensure cortex control of divider
	clear_asc_bit(1081);

	// Need to set analog_cfg<183> to 0 to select BLE for chips out
	ANALOG_CFG_REG__11 = 0x0000;
	
	// Set current in LC tank
	set_LC_current(127);
	
	// Set LDO voltages for PA and LO
	set_PA_supply(63);
	set_LO_supply(127,0);
	
	// Ensure cortex control of LO
	clear_asc_bit(964);
	
	// Ensure cortex control of divider
	clear_asc_bit(1081);
}

/*
void gen_test_ble_packet(uint8_t *packet) {
	*packet = 0x1D;
	packet++;
	*packet = 0x55;
	packet++;
	*packet = 0x6B;
	packet++;
  *packet = 0x7D;
	packet++;
	*packet = 0x91;
	packet++;
	*packet = 0x71;
	packet++;
	*packet = 0xFE;
	packet++;
	*packet = 0x2B;
	packet++;
	*packet = 0x89;
	packet++;
	*packet = 0x84;
	packet++;
	*packet = 0xF0;
	packet++;
	*packet = 0xAB;
	packet++;
	*packet = 0x26;
	packet++;
	*packet = 0x0D;
	packet++;
	*packet = 0x25;
	packet++;
	*packet = 0x1C;
	packet++;
	*packet = 0xD2;
}
*/

void gen_test_ble_packet(uint8_t *packet) {
	*packet = 0x1D;
	packet++;
	*packet = 0x55;
	packet++;
	*packet = 0xAD;
	packet++;
	*packet = 0xF6;
	packet++;
	*packet = 0x45;
	packet++;
	*packet = 0xC7;
	packet++;
	*packet = 0xC5;
	packet++;
	*packet = 0x0E;
	packet++;
	*packet = 0x26;
	packet++;
	*packet = 0x13;
	packet++;
	*packet = 0xC2;
	packet++;
	*packet = 0xAC;
	packet++;
	*packet = 0x98;
	packet++;
	*packet = 0x37;
	packet++;
	*packet = 0xB8;
	packet++;
	*packet = 0x30;
	packet++;
	*packet = 0xA1;
	packet++;
	*packet = 0xC9;
	packet++;
	*packet = 0xE4;
	packet++;
	*packet = 0x93;
	packet++;
	*packet = 0x75;
	packet++;
	*packet = 0xB7;
	packet++;
	*packet = 0x41;
	packet++;
	*packet = 0x6C;
	packet++;
	*packet = 0xD1;
	packet++;
	*packet = 0x2D;
	packet++;
	*packet = 0xB8;
}

void load_tx_arb_fifo(uint8_t *packet) {
	
	// Initialize Variables
	int i;												// used to loop through the bytes
	int j;												// used to loop through the 8 bits of each individual byte

	uint8_t current_byte;   // temporary variable used to get through each byte at a time
	uint8_t current_bit;		// temporary variable to put the current bit into the GPIO
	uint32_t fifo_ctrl_reg = 0x00000000; // local storage for analog CFG register
	
	// Prepare FIFO for loading
	fifo_ctrl_reg |= 0x00000001;  // raise LSB - data in valid
	fifo_ctrl_reg &= 0xFFFFFFFB;  // lower 3rd bit - data out ready
	fifo_ctrl_reg &= 0xFFFFFFDF;  // lower clock select bit to clock in from Cortex
	
	ANALOG_CFG_REG__11 = fifo_ctrl_reg; // load in the configuration settings
	
	// Load packet into FIFO
	for (i = 0; i < 64; ++i) {
		current_byte = *packet;     // put a byte into the temporary storage
		packet++;										// go to the next byte (for next time)
		
		for (j = 7; j >= 0; --j) {
			//current_bit = ((current_byte<<(j-1))&0x80)>>7;
			current_bit = (current_byte >> j) & 0x1;
			
			if (current_bit == 0) {
				fifo_ctrl_reg &= 0xFFFFFFFD;
				ANALOG_CFG_REG__11 = fifo_ctrl_reg;
			}
			if (current_bit == 1) {
				fifo_ctrl_reg |= 0x00000002;
				ANALOG_CFG_REG__11 = fifo_ctrl_reg;
			}
			
			fifo_ctrl_reg |= 0x00000008;
			ANALOG_CFG_REG__11 = fifo_ctrl_reg;
			fifo_ctrl_reg &= 0xFFFFFFF7;
			ANALOG_CFG_REG__11 = fifo_ctrl_reg; // toggl the clock!
		}
	}
}

void transmit_tx_arb_fifo() {
	
	uint32_t fifo_ctrl_reg = 0x00000000; // local storage for analog CFG register 

	// Release data from FIFO
	fifo_ctrl_reg |= 0x00000010; // enable div-by-2
	fifo_ctrl_reg &= 0xFFFFFFFE; // lower data in valid (set FIFO to output mode)
	fifo_ctrl_reg |= 0x00000004; // raise data out ready (set FIFO to output mode)
	fifo_ctrl_reg |= 0x00000020; // set choose clk to 1
	
	ANALOG_CFG_REG__11 = fifo_ctrl_reg;

}

// Must set IF clock frequency AFTER calling this function
void radio_init_rx_ZCC_BLE(){
	
	uint32_t mask1, mask2;
	uint32_t correlation_threshold;
	
	// IF uses ASC<271:500>, mask off outside that range
	mask1 = 0xFFFE0000;
	mask2 = 0x000007FF;	
	ASC[8] &= mask1;
	ASC[15] &= mask2;
	
	ASC[8] |= (0x0000FFF0 & ~mask1);   //256-287
	ASC[9] = 0x00422188;   //288-319
	ASC[10] = 0x88040071;   //320-351
	ASC[11] = 0x100C4081;   //352-383
	ASC[12] = 0x00188102;   //384-415
	ASC[13] = 0x017FC844;   //416-447
	ASC[14] = 0x70010001;   //448-479
	ASC[15] |= (0xFFE00800 & ~mask2);   //480-511
	
	// Set clock mux to internal RC oscillator
 	clear_asc_bit(424);
	set_asc_bit(425);
	
	// Need to turn on ADC clock generation to get /4 output for clock calibration
	set_asc_bit(422);
	
	// Set gain for I and Q
	set_IF_gain_ASC(63,63);
	
	// Set gm for stg3 ADC drivers
	set_IF_stg3gm_ASC(7, 7); //(I, Q)

	// Set comparator trims
	//set_IF_comparator_trim_I(0, 7); //(p,n)
	//set_IF_comparator_trim_Q(15, 0); //(p,n)
	
	// Set zcc demod parameters
	
	// Set clk/data mux to zcc
	// ASC<0:1> = [1 1]
	set_asc_bit(0);
	set_asc_bit(1);
	
	// Set counter threshold 122:107 MSB:LSB
	// for 76MHz, use 13
	set_zcc_demod_threshold(13);

	// Set clock divider value for zcc
	// The IF clock divided by this value must equal 1 MHz for BLE 1M PHY
	set_IF_ZCC_clkdiv(76);
	
	// Set early decision margin to a large number to essentially disable it
	set_IF_ZCC_early(80);
	
	// Mux select bits to choose I branch as input to zcc demod
	set_asc_bit(238);
	set_asc_bit(239);
	
	// Mux select bits to choose internal demod or external clk/data from gpio
	// '0' = on chip, '1' = external from GPIO
	clear_asc_bit(269);
	clear_asc_bit(270);
	
	// Enable ZCC demod
	set_asc_bit(132);
	
	// Disable feedback
	clear_asc_bit(123);
	
	// Threshold used for packet detection
	correlation_threshold = 5;
	ANALOG_CFG_REG__9 = correlation_threshold;

	// Set LDO reference voltage
	set_IF_LDO_voltage(0);

	// Set RST_B to analog_cfg[75]
	set_asc_bit(240);
	
	// Set RST_B to ASC<241>
	//clear_asc_bit(240);
	
	// Leave baseband held in reset until RX activated
	// RST_B = 0 (it is active low)
	ANALOG_CFG_REG__4 = 0x2000;	
	ANALOG_CFG_REG__4 = 0x2800;

	// Mixer and polyphase control settings can be driven from either ASC or memory mapped I/O
	// Mixers and polyphase should both be enabled for RX and both disabled for TX
	// --
	// For polyphase (1=enabled), 
	// 	mux select signal ASC<746>=0 gives control to ASC<971>
	//	mux select signal ASC<746>=1 gives control to analog_cfg<256> (bit 0 of ANALOG_CFG_REG__16)
	// --
	// For mixers (0=enabled), both I and Q should be enabled for matched filter mode
	// 	mux select signals ASC<744>=0 and ASC<745>=0 give control to ASC<298> and ASC<307>
	//	mux select signals ASC<744>=1 and ASC<745>=1 give control to analog_cfg<257> analog_cfg<258> (bits 1 and 2 in ANALOG_CFG_REG__16)	
	
	// Set mixer and polyphase control signals to memory mapped I/O
	set_asc_bit(744);
	set_asc_bit(745);
	set_asc_bit(746);
	
	// Enable both polyphase and mixers via memory mapped IO (...001 = 0x1)
	// To disable both you would invert these values (...110 = 0x6)
	ANALOG_CFG_REG__16 = 0x1;
}


void initialize_mote_ble(){

	uint8_t t;
	
	// Disable interrupts for the radio FSM
	radio_disable_interrupts();
	
	// Disable RF timer interrupts
	rftimer_disable_interrupts();
	
	//--------------------------------------------------------
	// SCM3C Analog Scan Chain Initialization
	//--------------------------------------------------------
	// Init LDO control
	init_ldo_control();

	// Set LDO reference voltages
	//set_VDDD_LDO_voltage(0);
	//set_AUX_LDO_voltage(0);
	//set_ALWAYSON_LDO_voltage(0);
		
	// Select banks for GPIO inputs
	GPI_control(0,0,0,0);
	
	// Select banks for GPIO outputs
	GPO_control(4,6,6,10);
	
	// Set all GPIOs as outputs
	GPI_enables(0x0000);	
	GPO_enables(0xFFFF);

	// Set HCLK source as HF_CLOCK
	set_asc_bit(1147);
	
	// Set initial coarse/fine on HF_CLOCK
	//coarse 0:4 = 860 861 875b 876b 877b
	//fine 0:4 870 871 872 873 874b
	set_sys_clk_secondary_freq(HF_CLOCK_coarse, HF_CLOCK_fine);	
	
	// Set RFTimer source as HF_CLOCK
	set_asc_bit(1151);

	// Disable LF_CLOCK
	set_asc_bit(553);
	
	// HF_CLOCK will be trimmed to 20MHz, so set RFTimer div value to 40 to get 500kHz (inverted, so 1101 0111)
	set_asc_bit(49);
	set_asc_bit(48);
	clear_asc_bit(47);
	set_asc_bit(46);
	clear_asc_bit(45);
	set_asc_bit(44);
	set_asc_bit(43);
	set_asc_bit(42);
	
	// Set 2M RC as source for chip CLK
	set_asc_bit(1156);
	
	// Enable 32k for cal
	set_asc_bit(623);
	
	// Enable passthrough on chip CLK divider
	set_asc_bit(41);
	
	// Init counter setup - set all to analog_cfg control
	// ASC[0] is leftmost
	//ASC[0] |= 0x6F800000; 
	for(t=2; t<9; ++t) set_asc_bit(t);	
	
//radio_init_rx_MF();
	// Init RX
	radio_init_rx_ZCC_BLE();

	// Init TX for BLE
	radio_init_tx_BLE();
	// radio_init_tx();
	
	// Set initial IF ADC clock frequency
	set_IF_clock_frequency(IF_coarse, IF_fine, 0);

	// Set initial TX clock frequency
	set_2M_RC_frequency(31, 31, RC2M_coarse, RC2M_fine, RC2M_superfine);

	// Turn on RC 2M for cal
	set_asc_bit(1114);
		
	// Set initial LO frequency
	// LC_monotonic(LC_code);
	
	// Init divider settings
	radio_init_divider(2000);
	
	// Program analog scan chain
	analog_scan_chain_write(&ASC[0]);
	analog_scan_chain_load();
	//--------------------------------------------------------
	
}
